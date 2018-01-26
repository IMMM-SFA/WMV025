#!/bin/tcsh
# Extract the appropriate line from the upstreamcellfile

if ($#argv > 0) then
    set FileIn = ${argv[1]}
    shift
endif
if ($#argv > 0) then
    set Mask = ${argv[1]}
    shift
endif
if ($#argv > 0) then
    set FileOut = ${argv[1]}
    shift
endif
if ($#argv > 0) then
    set RunPath = ${argv[1]}
    shift
endif


awk ' \
BEGIN {\
    n = 1;\
    while (getline x < "'${Mask}'" > 0) {\
	split(x,y);\
	lat[n] = y[3];\
	lon[n] = y[4];\
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
}' $RunPath/../$FileIn > $FileOut
