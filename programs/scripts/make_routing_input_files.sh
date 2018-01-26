#!/bin/tcsh

set Basin = ${argv[1]} 
set File = ${argv[2]} 
set UpstreamCellsFile = ${argv[3]} 
set RoutPath = ${argv[4]} 
set BinPath = ${argv[5]} 
set ShellPath = ${argv[6]} 
set WorkFix = ${argv[7]} 
set Mode = ${argv[8]} 
set PostFix = ${argv[9]}
set ReservoirFile = ${argv[10]}
set DemandFilePath = ${argv[11]}
set Year = ${argv[12]}
set Reservoirs = ${argv[13]}
set Count =  ${argv[14]}
set MetForcings =  ${argv[15]}
set ScenarioName = ${argv[16]}
set Demand = ${argv[17]}
set CPath = ${argv[18]}
set StartYear = ${argv[19]}
set EndYear = ${argv[20]}
set ForcingsOrigin =  ${argv[21]}

#gcc -lm -o $BinPath/routing.modifystationfile $CPath/routing.modifystationfile.c
$BinPath/routing.modifystationfile $Basin $File $RoutPath/$Basin.sta $RoutPath/../../$UpstreamCellsFile $Reservoirs $ReservoirFile
echo 'Stationfile modified'

cd $RoutPath
rm -rf rout.tmp.*
rm -rf *.uh_s
echo 'Start modifying routing input file'
$ShellPath/routing.modify.inputfile.sh $Basin $WorkFix $Mode $PostFix $RoutPath $ReservoirFile $DemandFilePath $Year $Reservoirs output/$ScenarioName/$PostFix $Demand $StartYear $EndYear $ScenarioName $ForcingsOrigin 
echo 'Finished modifying routing input files (for upstream routing)'


