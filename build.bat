@echo off
setlocal

set RES_FILE=
if exist cseq.rc (
    echo [INFO] Compiling resources...
    rc /nologo /i headers cseq.rc
    set RES_FILE=cseq.res
)

echo [INFO] Compiling C source files...
:: Added /Iheaders to search the "headers" folder for all .h files
cl /nologo /MP /MD /O2 /Oi /fp:fast /GL /W3 /wd4244 /wd4267 /std:c17 /Iheaders *.c %RES_FILE% user32.lib gdi32.lib shell32.lib comdlg32.lib msimg32.lib ole32.lib winmm.lib /Fe:cseq.exe /link /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF /LTCG

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Compilation failed!
    exit /b %ERRORLEVEL%
)

where upx >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    if exist cseq.exe (
        echo [INFO] Compressing executable with UPX...
        upx --best --lzma cseq.exe
    )
) else (
    echo [WARN] UPX not found in PATH. Skipping compression.
)

echo [SUCCESS] Build complete!
endlocal