#!/bin/tcsh
#

if ($#argv > 0) then
    set InFile = ${argv[1]}
    shift
endif
if ($#argv > 0) then
    set Basin = ${argv[1]}
    shift
endif
#if ($#argv > 0) then
#    set FracFile = ${argv[1]}
#    shift
#endif

set FracFile = frac.txt
echo 'Work on  file - awk "'${InFile}'"'


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
	if ($1 == lat[i] && $2 == lon[i]) {\
	    print $0;\
	    continue;\
	}\
    }\
}' ${InFile} >! irr.$Basin.gmt

