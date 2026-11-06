@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

if not exist build mkdir build
set "TRACE=build\trace.log"

if /i "%~1"=="--elevated" goto admin_ok

set "ADMIN=0"
net session >nul 2>&1 && set "ADMIN=1"
if "%ADMIN%"=="0" fsutil dirty query %SystemDrive% >nul 2>&1 && set "ADMIN=1"
if "%ADMIN%"=="0" (
    echo [eac_emu] yonetici olarak yeniden baslatiliyor...
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -ArgumentList '--elevated' -Verb RunAs"
    timeout /t 2 >nul
    exit /b 0
)
goto run

:admin_ok
echo [eac_emu] yonetici modunda calisiyor

:run
type nul > "%TRACE%"
echo [stage] basladi >> "%TRACE%"

where winget >nul 2>&1
if errorlevel 1 (
    echo [eac_emu] winget yok.
    echo Microsoft Store'dan "App Installer" kur, sonra tekrar calistir.
    pause
    exit /b 5
)
echo [stage] winget ok >> "%TRACE%"

set "NEED_RERUN=0"

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSPATH="
if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * ^
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 ^
        -property installationPath`) do set "VSPATH=%%i"
)
if not defined VSPATH (
    echo [eac_emu] Visual Studio C++ araclari yok, kuruluyor...
    echo [stage] vs kurulum >> "%TRACE%"
    winget install -e --id Microsoft.VisualStudio.2022.BuildTools ^
        --accept-package-agreements --accept-source-agreements --disable-interactivity ^
        --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
    set "NEED_RERUN=1"
)
echo [stage] vs ok >> "%TRACE%"

set "SDKROOT=%ProgramFiles(x86)%\Windows Kits\10"
set "HAS_SDK=0"
if exist "%SDKROOT%\Include" (
    for /f "delims=" %%v in ('dir /b /ad /o-n "%SDKROOT%\Include" 2^>nul') do (
        if exist "%SDKROOT%\Include\%%v\um\windows.h" set "HAS_SDK=1"
    )
)
if "%HAS_SDK%"=="0" (
    echo [eac_emu] Windows SDK yok, kuruluyor...
    echo [stage] sdk kurulum >> "%TRACE%"
    winget install -e --id Microsoft.WindowsSDK.10.0.26100 ^
        --accept-package-agreements --accept-source-agreements --disable-interactivity
    set "NEED_RERUN=1"
)
echo [stage] sdk ok >> "%TRACE%"

set "WDKINC="
set "WDKLIB="
if exist "%SDKROOT%\Include" (
    for /f "delims=" %%v in ('dir /b /ad /o-n "%SDKROOT%\Include" 2^>nul') do (
        if exist "%SDKROOT%\Include\%%v\km" (
            set "WDKINC=%SDKROOT%\Include\%%v\km"
            set "WDKLIB=%SDKROOT%\Lib\%%v\km\x64"
        )
    )
)
if not defined WDKINC (
    echo [eac_emu] WDK yok, kuruluyor...
    echo [stage] wdk kurulum >> "%TRACE%"
    winget install -e --id Microsoft.WindowsWDK.10.0.26100 ^
        --accept-package-agreements --accept-source-agreements --disable-interactivity
    set "NEED_RERUN=1"
)
echo [stage] wdk ok >> "%TRACE%"

if "%NEED_RERUN%"=="1" (
    echo.
    echo [eac_emu] Bilesenler kuruldu.
    echo Bu pencereyi kapat, sonra build.bat'i tekrar calistir.
    echo.
    pause
    exit /b 0
)

if not defined VSPATH (
    echo [eac_emu] MSVC bulunamadi.
    pause
    exit /b 3
)
if not defined WDKINC (
    echo [eac_emu] WDK bulunamadi.
    pause
    exit /b 4
)

call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    echo [eac_emu] vcvars64 basarisiz.
    pause
    exit /b 3
)
echo [stage] vcvars ok >> "%TRACE%"

set "LOG=build\build.log"
type nul > "%LOG%"

echo [1/4] dll...
cl /nologo /std:c++17 /EHsc /W3 /O2 /MT /DUNICODE /D_UNICODE ^
   /DWIN32_LEAN_AND_MEAN /DNOMINMAX /I include ^
   src\log.cpp src\dll\dllmain.cpp src\dll\eos_api.cpp src\dll\legacy_api.cpp ^
   /Fo:build\ /Fe:build\EasyAntiCheat_EOS.dll /LD ^
   /link /DEF:src\dll\eac_emu.def /MACHINE:X64 >> "%LOG%" 2>&1
if errorlevel 1 goto fail

echo [2/4] surucu...
cl /nologo /c /kernel /GS- /W3 /D_AMD64_ /D_WIN64 /DUNICODE /D_UNICODE ^
   /DNTDDI_VERSION=0x0A000000 /I include /I "%WDKINC%" ^
   src\sys\driver.cpp /Fo:build\driver.obj >> "%LOG%" 2>&1
if errorlevel 1 goto fail
link /nologo /DRIVER /SUBSYSTEM:NATIVE /ENTRY:DriverEntry /MACHINE:X64 ^
   /OUT:build\eac_emu.sys build\driver.obj ^
   /LIBPATH:"%WDKLIB%" ntoskrnl.lib hal.lib wdm.lib BufferOverflowK.lib >> "%LOG%" 2>&1
if errorlevel 1 goto fail

echo [3/4] ctl...
cl /nologo /std:c++17 /EHsc /W3 /O2 /MT /DUNICODE /D_UNICODE ^
   /DWIN32_LEAN_AND_MEAN /DNOMINMAX /I include ^
   src\log.cpp src\ctl\main.cpp src\ctl\driver_ctl.cpp ^
   /Fo:build\ /Fe:build\eac_emu_ctl.exe /link /MACHINE:X64 advapi32.lib shell32.lib >> "%LOG%" 2>&1
if errorlevel 1 goto fail

echo [4/4] imzalama...
call :sign
if errorlevel 1 (
    echo [eac_emu] imzalama basarisiz, surucu yuklenmeyebilir.
)

copy /Y config.ini build\ >nul 2>&1
echo [stage] basarili >> "%TRACE%"
echo.
echo Cikti: build\eac_emu.sys (imzali), build\EasyAntiCheat_EOS.dll, build\eac_emu_ctl.exe
echo.
pause
exit /b 0

:sign
set "SIGNTOOL="

where signtool >nul 2>&1 && set "SIGNTOOL=signtool"

if not defined SIGNTOOL (
    for /f "delims=" %%d in ('dir /b /ad "%SDKROOT%\bin" 2^>nul') do (
        if not defined SIGNTOOL (
            if exist "%SDKROOT%\bin\%%d\x64\signtool.exe" set "SIGNTOOL=%SDKROOT%\bin\%%d\x64\signtool.exe"
        )
    )
)

if not defined SIGNTOOL (
    echo   signtool bulunamadi.
    echo   winget install -e --id Microsoft.WindowsSDK.10.0.26100
    exit /b 1
)

set "THUMB="
for /f "delims=" %%t in ('powershell -NoProfile -Command ^
    "$c = Get-ChildItem Cert:\CurrentUser\My ^| Where-Object { $_.Subject -eq 'CN=eac_emu_test' } ^| Sort-Object NotAfter -Descending ^| Select-Object -First 1; if (-not $c) { $c = New-SelfSignedCertificate -Type CodeSigningCert -Subject 'CN=eac_emu_test' -CertStoreLocation 'Cert:\CurrentUser\My' }; $c.Thumbprint" 2^>nul') do set "THUMB=%%t"

if not defined THUMB (
    echo   sertifika uretilemedi.
    exit /b 1
)
echo   sertifika: %THUMB%

rem sertifikayi guvenilen depolara ekle (verify icin)
set "CER=%TEMP%\eac_emu_test.cer"
powershell -NoProfile -Command ^
  "Export-Certificate -Cert 'Cert:\CurrentUser\My\%THUMB%' -FilePath '%CER%' -Force" >nul 2>&1
if exist "%CER%" (
    certutil -addstore -f Root "%CER%" >nul 2>&1
    certutil -addstore -f TrustedPublisher "%CER%" >nul 2>&1
    del /q "%CER%" >nul 2>&1
    echo   sertifika guvenilen depolara eklendi.
)

"%SIGNTOOL%" sign /v /fd sha256 /s My /sha1 %THUMB% build\eac_emu.sys >> "%LOG%" 2>&1
if errorlevel 1 (
    echo   imzalama hatasi, log: build\build.log
    exit /b 1
)

"%SIGNTOOL%" verify /v build\eac_emu.sys >> "%LOG%" 2>&1
if errorlevel 1 (
    echo   uyari: verify basarisiz (test modunda yuklemeyi engellemez)
) else (
    echo   imzalandi ve dogrulandi.
)
exit /b 0

:fail
echo.
echo ===== DERLEME BASARISIZ =====
type "%LOG%"
echo.
echo Log: %~dp0build\build.log
pause
exit /b 1