# eac_emu

EasyAntiCheat (EOS) emülatörü. Fortnite odaklı. Sürücü + DLL + kontrol aracı.
EAC yalnızca oyun açıkken çalıştığı için boot durumu yok.

## Derleme
    build.bat

Betik yönetici olarak yeniden başlar; VS Build Tools, Windows SDK ve WDK
eksikse winget ile kurar ve seni yeniden çalıştırmaya yönlendirir.

## Test imzalama (bir kez)
    bcdedit /set testsigning on      (yönetici cmd)
    yeniden başlat

## Kullanım
    build\eac_emu_ctl.exe
    1 -> sürücü kur+başlat
    3 -> durum (cihaz: acilabiliyor olmalı)
    6 -> IOCTL testi (0x226003 -> 0x35)
    4 -> DLL kur (orijinal .eacbak olarak yedeklenir)
    oyunu FortniteClient-Win64-Shipping.exe ile başlat
    5 -> DLL geri al, 2 -> sürücü kaldır

## Sınır
Sunucu tarafı EAC doğrulaması yoktur; oyun içi tespit yapılmaz.