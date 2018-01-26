#!/bin/tcsh
# Extract and the appropriate lines from a the soil file

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

awk ' \
BEGIN {\
    n = 1;\
    while (getline x < "'${Mask}'" > 0) {\
	split(x,y);\
	lat[n] = y[1];\
	lon[n] = y[2];\
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
}' ${FileIn} > ${FileOut}
