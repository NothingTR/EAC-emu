#pragma once
#include <windows.h>
#include <string>

bool drvInstall(const std::wstring& sysPath, const std::wstring& name);
bool drvStart(const std::wstring& name);
bool drvStop(const std::wstring& name);
bool drvDelete(const std::wstring& name);
bool drvRunning(const std::wstring& name);
HANDLE devOpen();
bool devGetVersion(HANDLE h, unsigned long* ver);
bool devSetup(HANDLE h, unsigned long* status);
bool devCheckNtdll(HANDLE h, unsigned long* status);