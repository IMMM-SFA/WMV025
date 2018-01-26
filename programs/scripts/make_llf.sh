#!/bin/tcsh

if ($#argv > 0) then
    set RoutingInputPath = ${argv[1]}
    shift
endif
if ($#argv > 0) then
    set Basin = ${argv[1]}
    shift
endif

rm -rf fract.tmp.*
set FRAC_TMPFILE = frac.tmp.$$
set FRAC_TMPFILE2 = frac2.tmp.$$
set FRAC_FILE = ${RoutingInputPath}/frac30min/${Basin}.frac
echo ${FRAC_FILE}
if ( ! -e $FRAC_FILE ) then
    echo 'Invalid basin: ' ${Basin}
    exit 1
endif

set NROWS = `awk '/nrows/ {print $2}' ${FRAC_FILE}`
set NCOLS = `awk '/ncols/ {print $2}' ${FRAC_FILE}`
set XLLCORNER = `awk '/xllcorner/ {print $2}' ${FRAC_FILE}`
set YLLCORNER = `awk '/yllcorner/ {print $2}' ${FRAC_FILE}`
set CELLSIZE = `awk '/cellsize/ {print $2}' ${FRAC_FILE}`
set XURCCORNER = `echo ${XLLCORNER} + 0.5 '*' ${CELLSIZE} | bc -l `
set YURCCORNER = `echo ${YLLCORNER} + '('${NROWS}-0.5')' '*' ${CELLSIZE} | bc -l`

awk '{if(NR>6) print $0}'  ${FRAC_FILE} >  ${FRAC_TMPFILE} 

awk ' {\
    for (i = 1; i <= NF; i++) {\
	if ($i > 0.002 ) {\
	    xcenter = '${XURCCORNER}'+(i-1)*'${CELLSIZE}';\
	    ycenter = '${YURCCORNER}'-(NR-1)*'${CELLSIZE}';\
	    printf("%.4f %.4f %.4f\n", xcenter, ycenter,$i);\
	}\
    }\
}' ${FRAC_TMPFILE} | sort -k1,2 | uniq >! ${FRAC_TMPFILE2}

rm ${FRAC_TMPFILE}
mv ${FRAC_TMPFILE2} frac.txt
echo end
exit 0
