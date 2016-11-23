@echo #ifndef BUILD_TIMESTAMP > Src/Inc/build_timestamp.h
@echo #define BUILD_TIMESTAMP "%date% %time%" >> Src/Inc/build_timestamp.h
@echo | set /p HGREVISION="#define HGREV " >> Src/Inc/build_timestamp.h
@hg identify --num >> Src/Inc/build_timestamp.h
@echo #define HGREVSTR(s) stringify_(s) >> Src/Inc/build_timestamp.h
@echo #define stringify_(s) #s >> Src/Inc/build_timestamp.h
@echo #endif >> Src/Inc/build_timestamp.h
@echo Src/Inc/build_timestamp.h file created