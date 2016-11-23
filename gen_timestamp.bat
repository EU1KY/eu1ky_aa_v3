@echo #ifndef BUILD_TIMESTAMP > Src/Inc/build_timestamp.h
@set hr=%time:~0,2%
@if "%hr:~0,1%" equ " " set hr=0%hr:~1,1%
@echo #define BUILD_TIMESTAMP "%date:~-4,4%-%date:~-10,2%-%date:~-7,2% %hr%:%time:~3,2%:%time:~6,2%" >> Src/Inc/build_timestamp.h
@echo | set /p TMPHG="#define HGREV " >> Src/Inc/build_timestamp.h
@hg identify -ni >> Src/Inc/build_timestamp.h
@echo // >> Src/Inc/build_timestamp.h
@echo #define HGREVSTR(s) stringify_(s) >> Src/Inc/build_timestamp.h
@echo #define stringify_(s) #s >> Src/Inc/build_timestamp.h
@echo #endif >> Src/Inc/build_timestamp.h
@echo Src/Inc/build_timestamp.h file created