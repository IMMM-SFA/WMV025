#!/bin/tcsh
# Make routing input file
set File = ${argv[1]} 
set GlobalFile = ${argv[2]}
set Irrigation = ${argv[3]}
set IrrFree = ${argv[4]}
set VICSimOutPath = ${argv[5]}
set ForceYear = ${argv[6]}
set StartYearSim = ${argv[7]}
set EndYear = ${argv[8]}
set MetPath = ${argv[9]}

 
if( $Irrigation == 1 ) then
set IrrigationText = TRUE
else
set IrrigationText = FALSE
endif

if( $IrrFree == 1 ) then
set IrrFreeText = TRUE
else
set IrrFreeText = FALSE
endif

rm -rf global.tmp.*
set TmpFile = global.tmp.$$
cp -f $GlobalFile global.txt

awk '{\
if ($1 == "SOIL") {print "SOIL '${File}'"} \
else if ($1 == "IRRIGATION") {print "IRRIGATION '${IrrigationText}'"} \
else if ($1 == "IRR_FREE") {print "IRR_FREE '${IrrFreeText}'"} \
else if ($1 == "RESULT_DIR") {print "RESULT_DIR '${VICSimOutPath}'"} \
else if ($1 == "STARTYEAR") {print "STARTYEAR '${StartYearSim}'"} \
else if ($1 == "ENDYEAR") {print "ENDYEAR '${EndYear}'"} \
else if ($1 == "FORCEYEAR") {print "FORCEYEAR '${StartYearSim}'"} \
else if ($1 == "FORCING1") {print "FORCING1 '${MetPath}\/forcings_'"} \
else if ($1 == "VEGPARAM") {print "VEGPARAM '${MetPath}\/../../../data/veg/global_lai_0.25deg_irri_tian.txt'"} \
else if ($1 == "VEGLIB") {print "VEGLIB '${MetPath}\/../../../data/veg/world.veg.lib'"} \
else {print $0}}' global.txt > $TmpFile
			
cp -f $TmpFile global.txt
