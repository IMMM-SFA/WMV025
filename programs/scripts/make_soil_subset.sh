#!/bin/tcsh
#

if ($#argv > 0) then
    set SoilFile = ${argv[1]}
    shift
endif
if ($#argv > 0) then
    set Basin = ${argv[1]}
    shift
endif
if ($#argv > 0) then
    set FracFile = ${argv[1]}
    shift
endif
if ($#argv > 0) then
    set RunPath = ${argv[1]}
    shift
endif

#echo 'Work on soil file - awk "'${SoilFile}'"'
rm -rf $RunPath/input/soil/soil.current

awk ' \
BEGIN {\
    n = 1;\
    while (getline x < "'${FracFile}'" > 0) {\
	split(x,y);\
	lon[n] = y[1];\
	lat[n] = y[2];\
	n++;\
    }\
}\
{\
    for (i = 1; i < n; i++) {\
	if ($3 == lat[i] && $4 == lon[i]) {\
	    print $0;\
	    continue;\
	}\
    }\
}' ${SoilFile} >! $RunPath/input/soil/soil.current

