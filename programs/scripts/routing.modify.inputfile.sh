# Make routing input file
set Basin = ${argv[1]} 
set WorkFix = ${argv[2]}
set Mode = ${argv[3]}
set PostFix = ${argv[4]}
set RoutPath = ${argv[5]}
set ReservoirFile = ${argv[6]}
set DemandFilePath = ${argv[7]}
set Year = ${argv[8]}
set Reservoirs = ${argv[9]}
set OutFilePath = ${argv[10]}
set Demand = ${argv[11]}
set StartYear = ${argv[12]}
set EndYear = ${argv[13]}
set ScenarioName = ${argv[14]}
set ForcingsOrigin = ${argv[15]}

echo
echo OutFilepath: $OutFilePath
rm -rf rout.tmp.*

set RoutTmpFile = rout.tmp.$$

	awk ' { \
	    if ($1 == "OUT_FILE_PATH") print "OUT_FILE_PATH '${OutFilePath}'/"\
	    else if($1 == "STATION_FILE") print "STATION_FILE  '${Basin}'.sta"\
	    else if($1 == "INPUT_FILE_PATH") print "INPUT_FILE_PATH ../output/daily.ascii/fluxes_"\
	    else if($1 == "FLOW_DIREC_FILE") print "FLOW_DIREC_FILE  '${Basin}'.dir"\
	    else if($1 == "WORK_PATH") print "WORK_PATH '${OutFilePath}'/"\
	    else if($1 == "IRRIGATION") print "IRRIGATION   0"\
	    else if($1 == "RESERVOIR_FILE") print "RESERVOIR_FILE  input/'${ReservoirFile}'"\
	    else if($1 == "ROUTED_FILE") print "ROUTED_FILE  routedcells.txt"\
	    else if($1 == "DEMAND_FILE_PATH") print "DEMAND_FILE_PATH  '${DemandFilePath}'" \
	    else if($1 == "DEMAND") print "DEMAND  '${Demand}'" \
	    else if($1 == "FLUX_PATH")   print "FLUX_PATH  ../'${OutFilePath}'/" \
	    else if($1 == "SIMYEAR")  print "SIMYEAR '${Year}'" \
	    else if($1 == "RESERVOIRS")  print "RESERVOIRS '${Reservoirs}'" \
	    else if($1 == "NAT_PATH") print "NAT_PATH output/'$ScenarioName'/noirrig.wb.24hr/" \
	    else if($1 == "INPUT_DATES") print "INPUT_DATES  '$StartYear' 1 '$EndYear' 12" \
	    else if($1 == "OUTPUT_DATES") print "OUTPUT_DATES  '$StartYear' 1 '$EndYear' 12" \
	    else  print $0;\
	} ' $RoutPath/rout.inp > $RoutTmpFile

mv $RoutTmpFile rout.inp
