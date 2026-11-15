@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 (
    echo [!] cl.exe not found.
    echo     Open "x64 Native Tools Command Prompt for VS" or run vcvars64.bat first.
    exit /b 1
)
if not exist dist mkdir dist
cl /nologo /LD /O2 /MT /GS- /W3 src\minigameappbase.c /link /DEF:src\minigameappbase.def /OUT:dist\minigameappbase.dll
if errorlevel 1 (
    echo [!] Build failed.
    exit /b 1
)
echo [+] Built dist\minigameappbase.dll
endlocal
