#!/bin/tcsh -x
# Extracts lat and lon from fraction file

if ($#argv > 0) then
    set FracFile = ${argv[1]}
    shift
endif
if ($#argv > 0) then
    set FileOut = ${argv[1]}
    shift
endif

echo $FracFile $FileOut

set NRows = `awk '/nrows/ {print $2}' ${FracFile}`
set NCols = `awk '/ncols/ {print $2}' ${FracFile}`
set XllCorner = `awk '/xllcorner/ {print $2}' ${FracFile}`
set YllCorner = `awk '/yllcorner/ {print $2}' ${FracFile}`
set CellSize = `awk '/cellsize/ {print $2}' ${FracFile}`
set XurCorner = `echo ${XllCorner} + 0.5 '*' ${CellSize} | bc -l `
set YurCorner = `echo ${YllCorner} + '('${NRows}-0.5')' '*' ${CellSize} | bc -l`
echo ${XllCorner} ${XurCorner} ${YllCorner} ${YurCorner} ${NRows} ${NCols}

rm -rf frac.tmp.*
set FracTmpFile = frac.tmp.$$

tail -n +7 ${FracFile} | awk ' {\
    for (i = 1; i <= NF; i++) {\
	if ($i > 0.002 ) {\
	    xcenter = '${XurCorner}'+(i-1)*'${CellSize}';\
	    ycenter = '${YurCorner}'-(NR-1)*'${CellSize}';\
	    printf("%.2f %.2f %.4f\n", xcenter, ycenter,$i);\
	}\
    }\
}' | sort -k1,2 | uniq >! ${FracTmpFile}

rm -rf rewrite.tmp.*
set RewriteTmpFile = rewrite.tmp.$$
awk '{ printf ("%.4f %.4f 1\n", $2,$1) }' $FracTmpFile >! $FileOut

