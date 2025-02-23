@echo off 

setlocal
call :setESC
cls

echo %ESC%[101;103mStart compilation%ESC%[0m
make
if %errorlevel% equ 0 (
    echo %ESC%[101;102mSuccessful compilation%ESC%[0m
    for %%F in (*.ovl) do (
        echo %ESC%[101;105mSending to switch...%ESC%[0m
        C:\\devkitPro\\tools\\bin\\nxlink.exe "%%F"
        echo %ESC%[0m
        exit /b
    )
) else (
    echo %ESC%[101;101mCompilation failed%ESC%[0m
)

:setESC
for /F "tokens=1,2 delims=#" %%a in ('"prompt #$H#$E# & echo on & for %%b in (1) do rem"') do (
  set ESC=%%b
  exit /B 0
)
exit /B 0