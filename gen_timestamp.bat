@echo off
echo #ifndef BUILD_TIMESTAMP > Src/Inc/build_timestamp.h
set hr=%time:~0,2%
if "%hr:~0,1%" equ " " set hr=0%hr:~1,1%

for /F "skip=1 delims=" %%F in ('
    wmic PATH Win32_LocalTime GET Day^,Month^,Year /FORMAT:TABLE
 ') do (
    for /F "tokens=1-3" %%L in ("%%F") do (
        set CurrDay=0%%L
        set CurrMonth=0%%M
        set CurrYear=%%N ))
set CurrDay=%CurrDay:~-2%
set CurrMonth=%CurrMonth:~-2%

echo #define BUILD_TIMESTAMP "%CurrDay%-%CurrMonth%-%CurrYear%%hr%:%time:~3,2%:%time:~6,2%" >> Src/Inc/build_timestamp.h
echo | set /p TMPHG="#define HGREV " >> Src/Inc/build_timestamp.h
hg identify -ni >> Src/Inc/build_timestamp.h
rem echo // >> Src/Inc/build_timestamp.h
echo #define HGREVSTR(s) stringify_(s) >> Src/Inc/build_timestamp.h
echo #define stringify_(s) #s >> Src/Inc/build_timestamp.h
echo #endif >> Src/Inc/build_timestamp.h
echo Src/Inc/build_timestamp.h file created