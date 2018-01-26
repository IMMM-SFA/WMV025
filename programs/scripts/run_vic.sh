#!/bin/tcsh
# Run the VIC model - integrated with the routing model
set RunPath = ${argv[1]}
set RoutOutPath = ${argv[2]}
set Basin = ${argv[3]} 
set WorkFix = ${argv[4]}
set ModFix = ${argv[5]} 
set Mode = ${argv[6]}
set PostFix = ${argv[7]}
set RoutPath = ${argv[8]}
set SoilPointsPath = ${argv[9]}
set CPath = ${argv[10]}
set VICSimOutPath = ${argv[11]}
set MetOldPath = ${argv[12]} 
set MetNewPath = ${argv[13]} 
set Resolution = ${argv[14]}
set Cru = ${argv[15]}
set Irrigation = ${argv[16]}
set FullEnergy = ${argv[17]} 
set GrndFlux = ${argv[18]} 
set TimeStep = ${argv[19]} 
set IrrFree = ${argv[20]}
set VegFile = ${argv[21]}
set NewArno = ${argv[22]}
set VIC = ${argv[23]} 
set GlobalFile = ${argv[24]}
set FracTmpFile = ${argv[25]} 
set ReservoirFile = ${argv[26]}
set IrrMonth = ${argv[27]}
set Reservoirs = ${argv[28]}
set DemandFilePath = ${argv[29]}
set Year = ${argv[30]}
set Region = ${argv[31]}
set NorIrrPath = ${argv[32]}
set ShellPath = ${argv[33]}
set BinPath = ${argv[34]}
set UpstreamCellsFile =  ${argv[35]}
set MetForcings = ${argv[36]}
set ScenarioName = ${argv[37]}
set Setup = ${argv[38]}
set GlobalFileBase = ${argv[39]}
set Demand = ${argv[40]}
set StartYearSim = ${argv[41]}
set StartYear = ${argv[42]}
set EndYear = ${argv[43]}
set ForceYear = ${argv[44]}
set ForcingsOrigin = ${argv[45]}
set QsCol = ${argv[46]}
set QsbCol = ${argv[47]}
set PrecCol = ${argv[48]}
set PrecOrigCol = ${argv[49]}
set ExtractWaterCol = ${argv[50]}
set FluxFile = ${argv[51]}

# File made by dams.find.irrigationwaterdemand.c:
set ExtractWaterFile =  $Basin.reservoirs.extractwater

cd ${RunPath}/
rm -rf temp/*
rm -rf *log.*
rm -rf  $RoutPath/routedcells.txt
rm -rf  $RoutPath/rout.inp.*
rm -rf  $RoutPath/rout.*
rm -rf  $RoutPath/*log.*
rm -rf  $RoutPath/$Basin.*
rm -rf  $RoutPath/*.sta*
rm -rf  $RoutPath/*.dir
rm -rf  $RoutPath/*.reservoirs*
rm -rf  $RoutPath/dirtest.txt
rm -rf  $RunPath/globalinput.*.txt
echo routout: $RoutOutPath/$Setup/
echo runout: $VICSimOutPath/
rm -rf $RoutOutPath/$Setup/* #precaution
rm -rf $VICSimOutPath/* #precaution
cp -f $RoutPath/input/$Basin.main $RoutPath/rout.inp
cp -f $RoutPath/input/$Basin.sta $RoutPath/$Basin.sta
cp -f $RoutPath/input/$Basin.dir $RoutPath/$Basin.dir
cp -f $RoutPath/input/$Basin.dir $RoutPath/dirtest.txt
cp -f $RoutPath/input/$Basin.reservoirs.firstline $RoutPath/$Basin.reservoirs.firstline
set StationFile = $Basin.sta
set Count = 1

foreach File ( $SoilPointsPath/* ) # loop in soil point files. The points are cells of irrigation, reservoir, outlet, or gage. Number of points is no greater than total cell number
#foreach File ( $SoilPointsPath/10001 $SoilPointsPath/10002 $SoilPointsPath/10003 $SoilPointsPath/10004  ) # test cells
#foreach File ( $SoilPointsPath/10004 )
        set Latitude = `awk '{ print($3) } ' $File `      # find latitude for current point
        set Longitude = `awk '{ print($4) } ' $File `     # find longitude for current point
        set IrrigationInCell = `awk '{ if($1=='$Latitude' && $2=='$Longitude') print($3) } ' $RunPath/../irr.$Basin.gmt `   #find irrfraction for current cell from .gmt file 
        set ReservoirInCell = 0 # your first guess is no reservoir, thereafter check reservoir information file and set ReservoirInCell to 1 if dam exists:
        set ReservoirInCell = `awk ' { if($4=='$Latitude' && $3=='$Longitude') printf("%d\n",1) } ' $RoutPath/input/$Basin.reservoirs.firstline `  # find if there's reservoir in cell from .firstline file 
        set RoutCell =  `awk ' { if($4=='$Latitude' && $3=='$Longitude') printf("%d\n",1) } ' $RoutPath/input/$Basin.points `   # 0 or 1, 1 means current cell needs to be routed, reservoir or outlet or gage from .points file. 
        echo
		echo
		echo
		echo Current cell: $File , lat=$Latitude, long=$Longitude
		echo IrrigationInCell: $IrrigationInCell
        echo ReservoirInCell: $ReservoirInCell
        echo RoutCell: $RoutCell
        awk '{ if($1=='$Latitude' && $2=='$Longitude') print $0 } ' $IrrMonth > irr.month # crops irrigation info. 
        awk '{ if($2=='$Latitude' && $3=='$Longitude') print $0 } ' $RunPath/../$ExtractWaterFile > extract.reservoirs # find which dam this cell can extract water from. Info is in .extractwater file (StepE). dams not built for irrigation are not on the list
	cd $RunPath/
        #Rout upstream cells if necessary. Station file and main file are modified before routing
        $ShellPath/upstream.extract.cell.sh $UpstreamCellsFile $File upstream.cells $RunPath  # find the upstream cells for the current cell and write it into upstream.cells. info is in upstreamcells.txt (Step H), based on direction file.
	awk '{ printf ("%d\n", $6) }' upstream.cells >! upstream.tmp2 # write the 6th column of upstream.cells into a temp file. The 6th column is the number of upstream cells 
        set UpstreamExist = `awk '{print($1)}' upstream.tmp2`  # if there's more than one cell upstream of the current cell 
        if( $UpstreamExist ) then
           #Make routing input file (main). Can include more than one upstream routing point. 
           #If reservoirs==1 you need to exclude dam locations from the station file (these are routed directly after vic simulation). 
	   $ShellPath/make_routing_input_files.sh $Basin $File $UpstreamCellsFile $RoutPath $BinPath $ShellPath $WorkFix $Mode $PostFix $ReservoirFile $DemandFilePath $Year $Reservoirs $Count $MetForcings $ScenarioName $Demand $CPath $StartYearSim $EndYear $ForcingsOrigin
           # make the routing input files for current cell if routing is needed. generate rout.inp in RoutPath
		   set LogTmpFile = log.tmp
	   cd $RoutPath
           echo 'Start routing'
	   ./rout rout.inp > $LogTmpFile         # ROUTE THE CURRENT CELL #############################################################################
           cp -f $LogTmpFile routlog.$Count
	       cp -f rout.inp rout.$Count
           cp -f $Basin.sta $Basin.sta.$Count
           echo 'Finished routing current cell'
           #dirtest comes from routing program. already routed cells are marked in routedcells.txt, 
           #so that you do not have to go through all (convolution) in downstream cells
           cp -f dirtest.txt $Basin.dir 
        else
         echo 'No upstream cells, routing not necessary yet'
        endif
	# VIC simulation, current cell. 
	
	######################################################
	
	cd $RunPath
        echo IrrigationInCell: $IrrigationInCell
	if( $IrrigationInCell == 0.0 ||  $IrrFree == 1 || $Irrigation == 0 ) then 
          # Meaning this must be a dam or a gage or an outlet cell, or free irrigation
	  echo 'Dam or outlet wo irrigation, do vic simulations using original metdata file and original global file'
          cd $RunPath
	  $ShellPath/global.file.modify.sh $File $GlobalFileBase $Irrigation $IrrFree $VICSimOutPath $ForceYear $StartYearSim $EndYear $MetOldPath # modify the global parameter file, only run VIC for current cell, adjust the IRRIGATION option
	else # Irrigation in cell, modify metfile before doing vic simulation. 
             # The last two columns in new met file represents available water 
             # (upstream river runoff and in more distant reservoirs) in mm averaged over cell area 
             #echo Irrigationincell $IrrigationInCell 
	     #NB! Hardocded reading and writing of metdata in metdata.modify.runoff! Uffda, ikke bra.
	     echo 'Modify last two columns in met-file, based on water availability: upstream routed runoff and reservoirs'
             #gcc -lm -o $BinPath/metdata.modify.runoff $CPath/metdata.modify.runoff.c
	     $BinPath/metdata.modify.runoff $File extract.reservoirs irr.month $MetOldPath/ $MetNewPath/ $RoutOutPath/$Setup/ $RoutOutPath/$Setup/ $Basin $Resolution $Reservoirs upstream.cells $IrrigationInCell  $ForceYear $StartYearSim $EndYear
	     echo 'Finished modifying met-file, now modify global file'
	     $ShellPath/global.file.modify.sh $File $GlobalFile $Irrigation $IrrFree $VICSimOutPath $ForceYear $StartYearSim $EndYear $MetNewPath
    endif # if irrigation in cell
	echo 'Run VIC for current cell'
        set GlobalFileTmp = global.txt
		cp global.txt globalinput.$Count.txt
	$RunPath/../models/vic/vic_irrig_42a/${VIC} -g ${GlobalFileTmp} > viclog.$Count  ##################################################################################
	echo 'VIC finished'
	#Rewrite binary data for current cell (date, prec, evap, run and base) to ascii data. 
	#Results written to run/output/daily.ascii/
        awk '{ printf ("%.4f %.4f 1\n", $3,$4) }' $File >! frac.tmp2
	$BinPath/fluxdata.binary.to.daily.ascii_tian -l$VICSimOutPath/  -q$FluxFile -sfrac.tmp2 -p$StartYearSim -v$StartYear -r$EndYear -o./output/daily.ascii/ -m$QsCol -n$QsbCol
       # testing purposes modified by Tian
       # $BinPath/fluxdata.binary.to.daily.ascii_allcol -l$VICSimOutPath/  -q$FluxFile -sfrac.tmp2 -p$StartYearSim -v$StartYear -r$EndYear -o./output/daily.ascii.allcol/$Setup/ -m$QsCol -n$QsbCol

	#Subtract water. Water withdrawals taken from (in order of priority) 
        #1) local cell (done within VIC), 2) local river, and 3) possibly from reservoir(s).
	#Both the streamflow file and the 'routed reservoir' file are modified
	if( $IrrigationInCell == 0.0 || $Irrigation == 0 ) then
        else
          if( $Reservoirs == 1 || $UpstreamExist >= 1 ) then
             #echo 'Subract water used for irrigation from upstream routed area'
	     #echo 'Thereafter subract water from more distant location if applicable'
             echo 'routing.subtract.water.used.for.irrigation'
             #if water can be extracted from dam, or if upstream cell(s) exist do the following:
             #gcc -lm -o  $BinPath/routing.subtract.water.used.for.irrigation  $CPath/routing.subtract.water.used.for.irrigation.c 
	     $BinPath/routing.subtract.water.used.for.irrigation_tian extract.reservoirs irr.month upstream.cells $FluxFile $FracTmpFile $VICSimOutPath/ $RoutOutPath/$Setup/ $Basin $Resolution $Reservoirs $Irrigation $IrrFree $IrrigationInCell $ForceYear $StartYearSim $EndYear $PrecCol $PrecOrigCol $ExtractWaterCol $RunPath/../temp/	
            mv $RoutPath/temp/streamflow* $RoutOutPath/$Setup/   #Modified by Tian, set the temp file for streamflow file
            #echo 'routing.subtract.water.used.for.irrigation done'
          endif 
        endif
        # Here: if reservoirs == 1 and current cell includes a dam: Do routing with reserovir included. 
        # Remember that all cells with reservoirs are included in the collection of cells to be run, even though irrig fraction=0
	if( $ReservoirInCell == 1 || $RoutCell == 1 ) then
           echo ReservoirRouting or Gage or Outlet routing
	   if( $ReservoirInCell == 1 ) then
	   echo "Reservoir in cell"
           $ShellPath/make_routing_input_files.sh $Basin $File $UpstreamCellsFile $RoutPath $BinPath $ShellPath $WorkFix $Mode $PostFix $ReservoirFile $DemandFilePath $Year $Reservoirs $Count $MetForcings $ScenarioName $Demand $CPath $StartYearSim $EndYear $ForcingsOrigin
	   else
	   echo "No reservoir in cell, i.e. gage or outlet, set Reservoirs = 0 in rout.inp "
           $ShellPath/make_routing_input_files.sh $Basin $File $UpstreamCellsFile $RoutPath $BinPath $ShellPath $WorkFix $Mode $PostFix $ReservoirFile $DemandFilePath $Year 0 $Count $MetForcings $ScenarioName $Demand $CPath $StartYearSim $EndYear $ForcingsOrigin
	   endif
           set LogTmpFile = log.tmp
	   cd $RoutPath
	   #Make new .sta, in order to rout only current cell
           awk '{ if( $4=='$Latitude' && $3=='$Longitude') printf ("1 0 %s %d %d -999 2\nNONE\n",$6,$1,$2) }' input/$Basin.points >! $Basin.sta
           echo 'Start reservoir or gage or outlet routing'
	   ./rout rout.inp >& $LogTmpFile                   ####################################################################################
           cp -f $LogTmpFile routlog.res.$Count
	       cp -f rout.inp rout.res.$Count
           cp -f $Basin.sta $Basin.sta.res.$Count 
           cp -f dirtest.txt $Basin.dir    
		   cp -f routedcells.txt $Count.routedcells.txt
           echo 'Finished reservoir routing current cell'
           #dirtest comes from routing program, 
           #routedcells.txt comes from routing program, already routed cells are marked. 
           cd $RunPath
        else 
           echo No ReservoirRouting
        endif
	@ Count ++	
    end

cd $RunPath
rm -rf temp/*
