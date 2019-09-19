#!/bin/bash

set -e

function write_timestamp {
    CurrYear=`date -u +%Y`
    CurrMonth=`date -u +%m`
    CurrDay=`date -u +%d`
    CurrHour=`date -u +%H`
    CurrMinute=`date -u +%M`

    echo \#define BUILD_TIMESTAMP \"${CurrYear}-${CurrMonth}-${CurrDay} ${CurrHour}:${CurrMinute} UT\" >> Src/Inc/build_timestamp.h
    echo \#define HGREVSTR\(s\) stringify_\(s\) >> Src/Inc/build_timestamp.h
    echo \#define stringify_\(s\) \#s >> Src/Inc/build_timestamp.h
    echo \#endif >> Src/Inc/build_timestamp.h
    echo Src/Inc/build_timestamp.h file created at ${CurrYear}-${CurrMonth}-${CurrDay} ${CurrHour}:${CurrMinute} UT
}

function write_git_error {
    echo #warning GIT failed. Repository not found. Firmware revision will not be generated. >> Src/Inc/build_timestamp.h
    echo #define HGREV N/A >> Src/Inc/build_timestamp.h
}

function write_header {
    echo \#ifndef BUILD_TIMESTAMP > Src/Inc/build_timestamp.h
    git status &> /dev/null
    if [ $? -ne 0 ]; then
	echo Failed to execute git status
	return 1
    fi

    HGREV="$(git rev-parse --short=8 HEAD)"
    echo "#define HGREV \"${HGREV}\"" >> Src/Inc/build_timestamp.h
    return 0
}

write_header
if [ $? -eq 0 ]; then
    write_timestamp
else
    echo Failed to execute git status
    write_git_error
    write_timestamp	
fi
