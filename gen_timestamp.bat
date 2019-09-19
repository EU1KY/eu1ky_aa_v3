@echo off
echo #ifndef BUILD_TIMESTAMP > Src/Inc/build_timestamp.h

git status > NUL
if ERRORLEVEL 1 goto GITERR
echo | set /p TMPHG="#define HGREV " >> Src/Inc/build_timestamp.h
git rev-parse --short=8 HEAD >> Src/Inc/build_timestamp.h

:MAKE_TIMESTAMP
for /F "skip=1 delims=" %%F in ('
    wmic PATH win32_utctime GET Day^,Month^,Hour^,Minute^,Year /FORMAT:TABLE
 ') do (
    for /F "tokens=1-5" %%L in ("%%F") do (
        set CurrDay=0%%L
        set CurrHour=0%%M
        set CurrMinute=0%%N
        set CurrMonth=0%%O
        set CurrYear=%%P))
set CurrDay=%CurrDay:~-2%
set CurrMonth=%CurrMonth:~-2%
set CurrHour=%CurrHour:~-2%
set CurrMinute=%CurrMinute:~-2%

echo #define BUILD_TIMESTAMP "%CurrYear%-%CurrMonth%-%CurrDay% %CurrHour%:%CurrMinute% UT">> Src/Inc/build_timestamp.h
echo #define HGREVSTR(s) stringify_(s) >> Src/Inc/build_timestamp.h
echo #define stringify_(s) #s >> Src/Inc/build_timestamp.h
echo #endif >> Src/Inc/build_timestamp.h
echo Src/Inc/build_timestamp.h file created at %CurrYear%-%CurrMonth%-%CurrDay% %CurrHour%:%CurrMinute% UT
goto END

:GITERR
echo #warning GIT failed. Repository not found. Firmware revision will not be generated. >> Src/Inc/build_timestamp.h
echo #define HGREV N/A >> Src/Inc/build_timestamp.h
goto MAKE_TIMESTAMP

:END
