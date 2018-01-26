#!/bin/tcsh
# Run the VIC model and the routing model 
# Use: ../programs/scripts/run_irrig.sh 
# Flags: 
#        -b extract basin from soilfile (irrigated and non-irrigated cells)
#        -c extract the appropriate lines from irrigation global gmt file (also non-irrigated cells),
#           and make arc-info type ascii file. NB! Must be done before -s!!!
#        -d copy forcing file, vic free run and regular run results to local dir 
#        -e find dams within basin (make *.points)
#        -f find irrigation water demand within basin 
#        -g rewrite binary files to daily ascii files (from ../run/noirr VIC simulations, all cells in basin)    
#        -h rearrange soilfile, take reservoir locations and irrigated cells into account. 
#           Write soilcells w irrigation to input/soil/points (-e and -f and -i must have been run first)
#        -i make file "upstreamcells.txt" (tells you from where you can extract irrigation)     
#        -j run VIC (all the above flags must have been run before running run_vic)

# ./run_irrig.sh  BasinNr 2 -b -c -d -e -f -g -h -i -j
# 

set CurrentPath = `pwd` # this script should be located in the highest level directory, "/BasinRun"
set RunPath = $CurrentPath/run #full path needed!
set NetworkName = ddm30_watch_2009 #Routing network
# set Network = 2  # 1 = Model intercomparison, round 2008, NCC_wb1 landmask
                 # 2 = Model intercomparison, round 2009A, CRU landmask
# set WhichScenario = 1 # 1 = WFD, baseline
# set Continents = 0
# set BasinAvg = 0
set ForceYear = 1979 #start, forcing data (assumes 0101)
set ForceEndYear = 2013 #end, forcing data (assumes 1231)
set StartYearSim = 1979 #(0101)
set EndYear = 2013   #(1231)

set StartYear = 1990 # after spinup


set YearInfo = 2000 #for dam and irrigation information, FIXED number, don't change!!
#############################################################
set SoilFileAlt = ( $CurrentPath/data/soil/global_025_soil_tian.txt \
		    $CurrentPath/data/soil/global_025_soil_tian.txt \
                    $CurrentPath/data/soil/global_025_soil_tian.txt \
                    $CurrentPath/data/soil/global_025_soil_tian.txt \
                    $CurrentPath/data/soil/global_025_soil_tian.txt ) #VICRun
set ForcingsOrigin = wfd #set the scenario (wfd or watch_wb3)
set Scenario = $ForcingsOrigin/baseline

set MetPath = $CurrentPath/data/met/asc_vicinp         # hard coded
set MetOldPath = $CurrentPath/data/met/asc_vicinp      # hard coded
set MetNewPath = $RunPath/input/met_irr
set MetData =  $MetPath/data_

set SetupAlt = ( irrig.wb.24hr \
                 noirrig.wb.24hr \
                 freeirrig.wb.24hr \
                 noirrig.res.wb.24hr \
                 irrig.res.wb.24hr ) #VICRun (five options, irrig, no irrig(must run before), freeirrig(must run before), 
				                     #no irrig but has reservoir, and irrig with reservoir)

set TimeStepAlt = ( 24 24 24 24 24 ) 
set GlobalFileNames = ( $CurrentPath/global_files/global.421.irrig.txt \
                        $CurrentPath/global_files/global.421.irrig.txt \
                        $CurrentPath/global_files/global.421.irrig.txt \
                        $CurrentPath/global_files/global.421.irrig.txt \
                        $CurrentPath/global_files/global.421.irrig.txt ) #VICRun
						
set GlobalFileBaseNames = ( $CurrentPath/global_files/global.421.txt \
                            $CurrentPath/global_files/global.421.txt \
                            $CurrentPath/global_files/global.421.txt \
                            $CurrentPath/global_files/global.421.txt \
                            $CurrentPath/global_files/global.421.txt ) #VICRun
							
set FluxFileNames = ( $CurrentPath/misc/fluxes.vic \
                      $CurrentPath/misc/fluxes.vic \
                      $CurrentPath/misc/fluxes.vic \
                      $CurrentPath/misc/fluxes.vic \
                      $CurrentPath/misc/fluxes.vic ) #VICRun
set IrrTypeAlt = ( 2 1 3 1 2 ) #VICRun 1:noirr 2:irr 3:freeirr 
set ModeAlt = ( wb wb wb wb wb ) #VICRun
set DemandAlt = ( 0 0 0 0 1 ) #VICRun
set QsCol = 7 #runoff column in VIC output
set QsbCol = 8 #baseflow column in VIC output (runoff and baseflow are two columns converted to ascii in step G)
set PrecCol = 3
set PrecOrigCol = 4
set EvapCol = 6 # EvapCol in noirr and freeirr
set ExtractWaterCol = 10
#####################################
# Calculate days
set StartForceDate = `date +%s -d "$ForceYear-01-01"`
set StartSimDate = `date +%s -d "$StartYearSim-01-01"`
set EndSimDate = `date +%s -d "$EndYear-12-31"`
set EndForceDate = `date +%s -d "$ForceEndYear-12-31"`

set TotalDays = `echo "1+(($EndForceDate - $StartForceDate)/86400)" | bc`
set SkipDays = `echo "(($StartSimDate - $StartForceDate)/86400)" | bc`
set CopyDays = `echo "1+(($EndSimDate - $StartSimDate)/86400)" | bc`

echo Forcing data starts from $ForceYear to $ForceEndYear. Total days is $TotalDays. Skip days is $SkipDays, copy days is $CopyDays
#####################################
# Initialize, and process command line
rm -rf *.tmp.*
set ExtractSoil = 0
set RearrangeSoil = 0
set CopyFlux = 0
set RunVIC = 0
set RoutOnly = 0
set MakeSummary = 0
set ReWrite = 0
set ExtractIrrig = 0
set CopyFiles = 0
set FindDams = 0
set IrrWaterDemand = 0
set MakeUpstream = 0
set CalcFluxData = 0
set CalcStreamflowVolume = 0
set FullEnergy = 0
set GrndFlux = 0 

if ($#argv > 0) then
    set Basin = ${argv[1]}
    shift
endif
if ($#argv > 0) then
    set VICRun = ${argv[1]}
    shift
endif

while ($#argv > 0)
    switch (${argv[1]})
	case '-b':
	    echo 'To do: Extract subset (basin) from soilfile'
	    set ExtractSoil = 1
	    breaksw
	case '-c':
	    echo 'To do: Extract the appropriate lines from the irrigation fraction file'
	    set ExtractIrrig = 1
	    breaksw
	case '-d':
	    echo 'To do: Copy the met files and freeirr noirr vic results for current basin to local documentry'
	    set CopyFiles = 1
	    breaksw
	case '-e':
	    echo 'To do: Find dams within basin'
	    set FindDams = 1
	    breaksw
	case '-f':
	    echo 'To do: Find irrigation water demand within basin'
	    set IrrWaterDemand = 1
	    breaksw
	case '-g':
	    echo 'To do: Rearrange soilfile'
	    set RearrangeSoil = 1
	    breaksw
	case '-h':
	    echo 'To do: rewrite binary files to ascii files - foreløpig alle, men trenger ikke ta flere enn der irrigfraction = 0'
	    set ReWrite = 1
	    breaksw
	case '-i':
	    echo 'To do: Make upstream file'
	    set MakeUpstream = 1
	    breaksw
	case '-j':
	    echo 'To do: Run VIC'
	    set RunVIC = 1
	    breaksw
	case '-k':
	    echo 'routing only'
	    set RoutOnly = 1
	    breaksw
	case '-l':
	    echo 'calc fluxdata'
	    set CalcFluxData = 1
	    breaksw
	case '-m':
	    echo 'calc volume runoff (streamflow), and delta reservoir storage'
	    set CalcStreamflowVolume = 1
	    breaksw
	case '-n':
	    echo 'make summary files'
	    set MakeSummary = 1
	    breaksw
	default:
	    echo 'Unrecognized command-line argument:' ${argv[1]}
	    exit 1
	    breaksw
    endsw
    shift
end
######################################
# Decide setup, files, paths, etc.
set IrrType = $IrrTypeAlt[$VICRun]
if( $IrrType == 3 ) then
set IrrFree = 1
else
set IrrFree = 0
endif
if( $IrrType == 2 || $IrrType == 3 ) then
set Irrigation = 1
else
set Irrigation = 0
endif
if( $VICRun == 4 || $VICRun == 5 ) then
set Reservoirs = 1
else
set Reservoirs = 0
endif

set Mode = $ModeAlt[$VICRun]
set Setup = $SetupAlt[$VICRun]
 
set SoilFile = $SoilFileAlt[$VICRun]
set GlobalFile = $GlobalFileNames[$VICRun]
set GlobalFileBase = $GlobalFileBaseNames[$VICRun]
set TimeStep = $TimeStepAlt[$VICRun]
set Demand = $DemandAlt[$VICRun]
set FluxFile = $FluxFileNames[$VICRun]
set SoilPointsPath = $RunPath/input/soil/points
set RoutingPath =  $RunPath/rout
set ShellPath = $CurrentPath/programs/scripts
set CPath = $CurrentPath/programs/C
set BinPath = $CurrentPath/programs/bin
set CropPath = $CurrentPath/data/aquastat
set IrrPath = $CurrentPath/irrigation
set DataPath =  $CurrentPath/data
set DamInfoPath = $DataPath/dams/unh
set DamFile = $DamInfoPath/dams_watch.txt
set GageInfoFile = $CurrentPath/misc/gagelist.txt #includes location of gages of interest (22 gages world-wide)
set RoutingNetworkPath = $DataPath/rivernetwork/$NetworkName
set RoutOutPath = $RoutingPath/output/$Scenario
set DirectionFilePath = $DataPath/rivernetwork/$NetworkName/dir30min_overlap
set RoutingMainFilePath = $DataPath/rivernetwork/$NetworkName/main30min
set FracFilePath = $DataPath/rivernetwork/$NetworkName/frac30min
set XMaskPath =  $DataPath/rivernetwork/$NetworkName/mask30min
set FluxPath = $RunPath/output
set VICSimOutPath = $FluxPath/$Scenario/$Setup
set VICFluxStandardPath =  $RunPath/output/$Scenario/noirrig.$Mode.{$TimeStep}hr
set NoirrFreeirrPath =  $RunPath/output/$Scenario
set GlobalNoirrFreeirrPath = /people/zhou014/shattel_arab_UCLA/flow/    #Tian May 2013
set GlobalMetPath = /people/zhou014/shattel_arab_UCLA/MidEast_forcing/         #Tian May 2013
set UpstreamCellsFile = upstreamcells.txt
set ReservoirFile = $Basin.reservoirs.firstline
set IrrFile = $DataPath/irrigation/irr.FAOV5.025.gmt # includes all cells in landmask
set WorkFix = work 
set ModFix = mod
set Resolution = 0.25
set CRU = 1 # CRU landmask: 1
set VegFile = 0 #not really used, but is argument to run_vic
set NewArno = 0 #not really used, but is argument to run_vic
set VIC = vicNl #executable vic
set FracFile = $CurrentPath/frac.txt
set IrrMonth = $DataPath/aquastat/world.cropping.month.025.wfd
set DemandFilePath = $DamInfoPath/$YearInfo
set Year = 2000
set YearForDams = 2010
set Region = 0
set NoIrrPath = ./

##############################################################################
#Start doing something
##############################################################################
# A. Make a list of the latitudes/longitudes and fractions of the cells that need 
# to be processed, based on the fraction file of current basin - results written to frac.txt
echo 'Do what is always required: Make llf, and copy some files to the appropriate directories'
$ShellPath/make_llf.sh $RoutingNetworkPath $Basin
echo backnow
cp $DirectionFilePath/$Basin.dir $RoutingPath/input/
cp $RoutingMainFilePath/$Basin.main $RoutingPath/input/
cp $FracFilePath/$Basin.frac $RoutingPath/input/
cp $XMaskPath/$Basin.xmask $RoutingPath/input/

rm $CurrentPath/temp/*
rm $RoutingPath/output/*.total
rm $RoutingPath/data/*.total


##############################################################################
# B. Extract the appropriate lines from the soil file (-b)
# - results written to input/soil/soil.current. Makes use of frac.txt from A. 
# Result used by most other subscripts.
# Can be somewhat time consuming
if($ExtractSoil) then
echo Extract the appropriate lines from the soil file 
$ShellPath/make_soil_subset.sh $SoilFile $Basin frac.txt $RunPath
else
endif
###############################################################################
# C. Extract the appropriate lines from the irrigation fraction file (-c)
# - results written to ./irr.$Basin.asc.
# Makes use of IrrFile irr.$Year.orig.gmt
# Somewhat time consuming
# Result is later used (soilfile.rearrange) to pick cells in basin with irrig > 0 (= those needed to be run in run_vic)
# Result is also used to find dams and irrigation within basin in question.
if($ExtractIrrig) then
echo Extract the appropriate lines from the irrigation fraction file 
rm irr.*.asc irr.*.gmt
$ShellPath/make_irrig_subset.sh $IrrFile $Basin frac.txt
#gcc -lm -o $BinPath/col2grid $CPath/col2grid.c
$BinPath/col2grid_tian irr.$Basin.gmt  $Resolution 0 > irr.$Basin.asc
else
endif
###############################################################################
# D. Copy the met file and freeirr and noirr vic run results to local documentry (-d) by Tian 2013
if($CopyFiles) then
echo Forcing is from $ForceYear to $ForceEndYear.
echo Copying Forcing file and freenoirrig data starts from $StartYearSim to $EndYear, $CopyDays days.
$ShellPath/copy_files.sh $CurrentPath/frac.txt $GlobalNoirrFreeirrPath $NoirrFreeirrPath $GlobalMetPath $MetPath $TotalDays $CopyDays $SkipDays $StartYearSim $EndYear $BinPath
else
endif
###############################################################################
# E. Find dams within basin at time of interest, $YearOfDams (-e)
# At the same time, find also gage(s) of interest + location of outlet cell
# Make *.reservoirs.firstline and *.points file for routing, and for sorting basin (see soilfile.rearrangepoints.c below)
# *.points file includes cells w dam, gages, and outlet cell (sorted from upstream to downstream)
# NB! You read .sta files (outlet info only!) from ddm30_watch_2009, og writes new .sta file (including dams, gages and outlet) to rout/input/!
# NB2! Here: UNH reservoir info!
if($FindDams) then
echo
echo 'Find dams within basin.'
rm *.reservoirs.firstline
#gcc -lm -o $BinPath/dams.gages.withinbasin.makeroutinput $CPath/dams.gages.withinbasin.makeroutinput.c 
$BinPath/dams.gages.withinbasin.makeroutinput_tian $Basin $YearForDams $RoutingNetworkPath/dir30min_overlap/$Basin.dir $RoutingNetworkPath/sta30min/$Basin.sta $RoutingPath/input/$Basin.reservoirs.firstline  $RoutingPath/input/$Basin.points $RoutingPath/input/$Basin.sta $DamFile $GageInfoFile
else
endif
###############################################################################
# F. Find irrigation water demand within basin (-f)
# Noirr and freirr runs must be performed before finding irrigation water demands. 
# Input:  $Basin.dir $Basin.frac $Basin.reservoirs.firstline, basin soilfile,
# Make: $Basin.reservoirs.upstream Located in run-directory, includes info on which dams are upstream each cell. 
#                                  For information/plotting purposes, - hence you can plot cells that are downstream each dam).
#       $Basin.reservoirs.extractwater Located in run-directory,
#                                      gives information on which dams water can be extracted from 
#                                      for metdata.modify.runoff.c) for each cell.  includes info on capacity and 
#                                      mean annual inflow to the reservoir. 
#       $DamInfoPath/$YearInfo/damname.calc.irrdemand.monthly: Monthly time series of a) net irr demand and b) gross irr demand 
#                                                              (but the latter is only slightly higher, should not really be interpreted as gross demand!!!
# NB! assumes runoff = col 7 and baseflow = col 8...... in flux files, and outlet cell=0 in direction file   
if($IrrWaterDemand == 1 ) then
echo Find irrigation water demand 
rm *.reservoirs.extractwater
rm *.reservoirs.upstream
#gcc -lm -Wall -o $BinPath/dams.find.irrigationwaterdemand $CPath/dams.find.irrigationwaterdemand.c
$BinPath/dams.find.irrigationwaterdemand $YearInfo $RoutingNetworkPath/dir30min_overlap/$Basin.dir $RoutingNetworkPath/frac30min/$Basin.frac $RoutingPath/input/$Basin.reservoirs.firstline $SoilFile $NoirrFreeirrPath $Basin.reservoirs.upstream $Basin.reservoirs.extractwater $DamInfoPath/$YearInfo $IrrFile $FluxFile $StartYearSim $EndYear $PrecCol $EvapCol
echo Finished find irrigation water demand $BinPath/dams.find.irrigationwaterdemand $YearInfo $RoutingNetworkPath/dir30min_overlap/$Basin.dir $RoutingNetworkPath/frac30min/$Basin.frac  $RoutingPath/input/$Basin.reservoirs.firstline $SoilFile $NoirrFreeirrPath $Basin.reservoirs.upstream $Basin.reservoirs.extractwater $DamInfoPath/$YearInfo $IrrFile $FluxFile $StartYearSim $EndYear $PrecCol $EvapCol
else
endif
###################################################################################
# G. Sort basin from upstream to downstream, take reservoir locations, gage locations, and irrigated cells into account (-g)
# Input information: soilfile for current basin, file with points (dams, gages, outlet) to be routed ($Basin.points), sorted from upstream to downstrem.
# Cells with irrigation and/or dam and/or outlet is selected,
# and will be VIC-simulated after routing the upstream area - in run_vic.sh. 
# Soil from ExtractSoil (-e)
# Irrigation information from above (-i)
# Points file (i.e with reservoirs, gages and outlet locations taken into account) hardcoded in program (-e)
# Make: One file for the cells that are to be run (meaning irrigated cells, gage locations, reservoir locations), 
#       file names from 10001 and upwards.  
if ($RearrangeSoil) then
echo  Rearrange soilfile, take reservoir locations and irrigated cells into account
rm -rf $SoilPointsPath/*
#gcc -lm -o $BinPath/soilfile.rearrangepoints $CPath/soilfile.rearrangepoints.c
$BinPath/soilfile.rearrangepoints_tian $Basin $RunPath/input/soil/soil.current $SoilPointsPath/ ./irr.$Basin.asc $RunPath/rout/input  
#echo 'Soilfile rearranged, reservoirs taken into account.'
endif
##########################################
# H. Rewrite nonirrigated part of area, binary files to ascii files. (-h)
# For now: Rewrite all cells in basin, from standard noirr run.
#          Not necessary to rewrite those where irrpercent < 0.1, but...
# First remove all files in output/daily.ascii
# Rewrite entire period $StartYearSim to $EndYear
if ($ReWrite) then
echo  Rewrite binary files to ascii files
awk '{ printf ("%.4f %.4f\n",$2,$1) }'  frac.txt >! frac2.txt
rm -rf $RunPath/output/daily.ascii/*
#gcc -lm -Wall -o $BinPath/fluxdata.binary.to.daily.ascii $CPath/fluxdata.binary.to.daily.ascii.c
$BinPath/fluxdata.binary.to.daily.ascii_tian -l$VICFluxStandardPath/ -q$FluxFile -sfrac2.txt -p$StartYearSim -v$StartYear -r$EndYear -o$RunPath/output/daily.ascii/ -m$QsCol -n$QsbCol

# convert the vic results to ascii (for testing purposes) Modified by Tian
# $BinPath/fluxdata.binary.to.daily.ascii_allcol -l$VICSimOutPath/ -q$FluxFile -sfrac2.txt -p$StartYearSim -v$StartYear -r$EndYear -o$RunPath/output/daily.ascii.allcol/$Setup/ -m$QsCol -n$QsbCol

rm -r frac2.txt
endif
#################################################################
# I. Figure out, for each cell, what cells are directly upstream (-i), 
# and hence from where you can extract water for irrigation (from river)
# Resulting file: upstreamcells.txt, used in run_vic.sh
if( $MakeUpstream == 1 ) then
echo Make upstream file
rm -rf $UpstreamCellsFile 
#gcc -lm -o $BinPath/find.upstreamcells $CPath/find.upstreamcells.c
$BinPath/find.upstreamcells_tian $DirectionFilePath/$Basin.dir $UpstreamCellsFile 
else
endif
################################################################
# J. Run VIC integrated with the routing model (-j)
echo test
if ($RunVIC) then
echo Run VIC for current basin, integrated with routing model
rm -rf input/met_irr/*
    echo "Start routing integrated with VIC modeling"
    $ShellPath/run_vic.sh $RunPath $RoutOutPath $Basin $WorkFix $ModFix $Mode $Setup $RoutingPath $SoilPointsPath $CPath $VICSimOutPath $MetOldPath $MetNewPath $Resolution $CRU $Irrigation $FullEnergy $GrndFlux $TimeStep $IrrFree $VegFile $NewArno $VIC $GlobalFile $FracFile $ReservoirFile $IrrMonth $Reservoirs $DemandFilePath $Year $Region $NoIrrPath $ShellPath $BinPath $UpstreamCellsFile $MetData $Scenario $Setup $GlobalFileBase $Demand  $StartYearSim $StartYear $EndYear $StartYearSim $ForcingsOrigin $QsCol $QsbCol $PrecCol $PrecOrigCol $ExtractWaterCol $FluxFile
endif
