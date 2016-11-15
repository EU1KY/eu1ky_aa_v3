@echo #ifndef BUILD_TIMESTAMP > Src/Inc/build_timestamp.h
@echo #define BUILD_TIMESTAMP "%date%%time%" >> Src/Inc/build_timestamp.h
@echo #endif >> Src/Inc/build_timestamp.h
@echo Src/Inc/build_timestamp.h file created