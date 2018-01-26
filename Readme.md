# Water Management model coupling with VIC hydrologic model at 0.25 degree resolution (WMV025)
- Running VIC with irrigation and dam regulations included.
- Ingjerd Haddeland, December 2010. Updated November 2011 and March 2012 
- Tian Zhou, Sep. 2013, Jan 2018

This document and accompanying files are an incomplete description/tutorial on how to
run VIC with irrigation and dams included. Make a directory, copy the BasinRun to that directory.
As the VIC irrigation and dam modules are not totally flexible (but you can of course change this yourself!). The
scheme is described in Haddeland et al. (2006a; 2006b), but the tutorial version is missing some
features that are in the original version (e.g. reservoir evaporation). The tutorial version has undergone
limited quality checking, so do not be overly surprised of you find some errors. Also, some of the
scripts are flexible, others still expect e.g. certain paths or file formats. I've tried to include
information on this in the run_*.sh scripts, but I won't guarantee that the information is complete.
VIC runs cell by cell, and routing is done as a postprocessing step. In order to do water withdrawals
properly, we need to work our way from upstream to downstream locations in a basin, and
downstream cells need to have information on how much water is available in the river and in
upstream reservoirs. Rewriting VIC to work in time domain instead of spatial domain would probably
have been the most elegant solution, but possibly also the most time consuming solution. Anyway,
VIC-routing-VIC interaction was chosen, meaning some programs are written to make VIC and the
routing model talk together, while still being two separate models. Result = Spaghetti model? Yes!
The information given here (and/or in C-programs, scripts and accompanying files) is for a situation
where your area of interest is large, i.e. global, but focuses on a run for one basin within the area, i.e.
the Colorado River basin. All basins must have distinct numbers. In this example, the Colorado River
Basin has number 38801 (DDM30 global basin mask file).

Before you start a VIC-irrig-dam run, you have to do two stand-alone VIC runs for your area. These
runs, and the other runs (and routing), should cover the same period.
1) Naturalized run  flux files should be in output/wfd/baseline/noirr.wb.24hr. Use
global_files/global.406.wb.24hr.2009A with IRRIGATION=FALSE and IRR_FREE=FALSE
2) Potential irrigation run flux files should be in output/wfd/baseline/freeirr.wb.24hr. Use
global_files/global.406.wb.24hr.2009A with IRRIGATION=TRUE and IRR_FREE=TRUE
Meterological forcings are not included in tutorial. Routing input files are only included for Colorado
(basin number 38801).
NB! If you only want to use the reservoir scheme, you can use the included routing-reservoir scheme
directly, since the routing scheme is totally independent of VIC itself. However, take into account that
one of the objective functions in the reservoir scheme is water demand, and if you do not give
information on water demands to the routing scheme, the reservoirs built for irrigation purposes may
behave somewhat strange.

- OVERVIEW
run/run_all.sh initiates a run for one basin (or many basins in a loop), here it is defined what
combination of irrigation and/or dams that are to be used. run/run_all.sh also starts
programs/scripts/run_irrig.sh .

programs/scripts/run_irrig.sh: Paths and files to use are defined, and tasks are performed
according to flags given. Mostly preprocessing tasks to extract information for the basin of interest,
and make files needed to run the vic-routing-vic scheme. The scripts is made to allow for global
use, multiple routing networks and forcings, so if you are concentrating on a single basin and
forcings, you may simplify.

A (always): Make a list of the latitudes/longitudes and fractions of cells within basin
B (flag �e): Extract the appropriate lines from the soil file
C (-i): Extract the appropriate lines from the irrigation fraction file
D (-f): Find dams within basin at time of interest
E (-w): Find irrigation water demand within basin
F (-s): Sort basin from upstream to downstream, take reservoir locations, gage locations,
and irrigated cells into account.
G (-d): Rewrite nonirrigated part of area, binary files to ascii files.
H (-u): Figure out, for each cell, what cells are directly upstream
I (-v): Run VIC integrated with the routing model: programs/scripts/run_vic.sh:

programs/scripts/run_vic.sh: The scheme itself. Goes through the basin from
upstream to downstream location, and starts VIC and/or the routing model for
each cell needed to be taken into account (i.e. where you need information on
river flow, or where irrigated fraction >0.1 percent, or a cell in which is a dam or
a gage, or the cell is at the outlet of the basin). Everything needed to do the run is
done automatically, but is dependent on A-H above.

-  Files needed; explanations

A number of files are needed, in addition to the traditional VIC files, for the vic-rout-vic runs.
misc/fluxes.vic: Information on fluxes in VIC output. The columns are: Name, number (from 0 to
nfluxes), flux or avg number (1=flux,2=avg), type (1=char,2=unsigned short int,3=signed short
int,4=float,5=int), mult. factor in output (corresponding to the numbers in your global file).

misc/gagelist.txt: List of gauges of interest. Basin number, basin name, gage name (max 5 char),
latitude and longitude.

global_files/global.406.wb.24hr.2009A and global.406.wb.24hr.irrig.2009A

data/dams/unh/dams_watch.txt: UNH dam information. Information updated according to 2003
ICOLD data, and purpose added.

data/irrigation/irr.2000.orig.all.gmt. Latitide, longitude and irrigation percentage in cell. Must
cover entire area of interest (here: CRU landmask)

data/aquastat/world.cropping.month.wfd: Cropping calendar. Must cover entire are of interest,
also non-irrigated cells.

- Files made (preprocessing acitivities); some explanations

run/rout/input/38801.reservoirs.firstline (flag �f)
10 5 -110.75 33.25 506 Coolidge 1929 1323526.0 76080.0 0.0 0 0 0 77 IHR
2 9 -114.75 35.25 509 Davis 1953 2242840.0 114122.0 0.0 0 0 0 61 H
3 11 -114.25 36.25 524 Hoover 1936 37296796.0 663687.0 0.0 1434 0 0 223 SHI
8 12 -111.75 36.75 519 Glen_Can 1964 35550184.0 686754.0 0.0 900 0 0 216 HIRX
17 15 -107.25 38.25 492 BlueMesa 1966 1160337.0 37150.0 0.0 86 0 0 119 IHRC
13 20 -109.25 40.75 515 FlamingG 1964 4937752.0 177334.0 0.0 152 0 0 153 SHRC

Information on the reservoirs within the basin. Explanation: col row lon lat id name Year
Capacity(1000m3) SurfArea(1000m2) CatchArea(km2) InstCap(MW) AnnEnergy(GWh)
IrrAreas(km2) Height(m) Purpose
Purpose: Must use capital letters. H = hydropower, I = irrigation, C = flood (ICOLD dam register
letters).
run/rout/input/38801.points(flag �f)
17 15 -107.25 38.25 BlueMesa 1
10 5 -110.75 33.25 Coolidge 2
13 20 -109.25 40.75 FlamingG 3
8 12 -111.75 36.75 Glen_Can 4
3 11 -114.25 36.25 Hoover 5
2 9 -114.75 35.25 Davis 6
2 2 -114.75 31.75 Outlet 7
The .points file includes information on cells within the basin of special interest, i.e. dams, gage
location and the outlet location. This file will be used to sort the basin cells before the simulations are
performed.
run/38801.reservoirs.extractwater (flag �w)
Gives information on which dams water can be extracted from. For each cell. includes info on
capacity and mean annual inflow to the reservoir.
data/dams/unh/2000/<damname>.calc.irrdemand.monthly (flag -w)
Monthly time series of a) net irrigation demand and b) gross irrigation demand (but the latter is only
slightly higher, should not really be interpreted as gross demand; irrigation efficiencies are not
included)

- VIC 4.0.6 WITH IRRIGATION.
Limitations in modelling scheme when irrigation is included in this VIC version: Only tested in daily
water balance mode, one elevation band, and without distributed precipitation. Also, you can get water
balance errors at the beginning of the second simulation month (has to do with initialization), so
always include a spin up period! This version does not allow for the use of INIT_STATE. You can
only have one irrigated vegetation type within each cell.

Global file
Two extra lines are included:
IRRIGATION FALSE/TRUE
IRR_FREE FALSE/TRUE
If you set both these options to TRUE, it means water is assumed available, i.e. potential irrigation. If
you run the VIC irrigation model without reservoirs, this is the most common setup and will give you
potential irrigation water use as result. IRRIGATION = TRUE and IRR_FREE = FALSE means
irrigation with water limitations, which is what you normally want when you run the irrigation scheme
and the reservoir scheme together. If you set both options to FALSE, the crops will not be irrigated.

Vegetation parameter file
A cell without irrigated vegetation should look like this (i.e. one extra 0 on the first line):
20000 5 0
1 0.1608 0.30 0.30 0.70 0.70
5.2130 5.2130 5.2130 5.2130 5.2130 5.2130 5.2130 5.2130 5.2130 5.2130 5.2130 5.2130
5 0.4148 0.30 0.30 0.70 0.70
0.1000 0.1000 0.1620 0.4620 3.6620 5.2750 5.5880 5.3630 3.0750 0.6500 0.2000 0.1500
6 0.2401 0.30 0.60 0.70 0.40
0.2870 0.3120 0.3120 0.3120 1.2500 3.4630 4.3380 3.5250 2.0880 0.7620 0.6130 0.4620
7 0.1284 0.30 0.60 0.70 0.40
0.3250 0.2500 0.2500 0.2500 1.0620 3.1500 3.8750 3.0870 1.1750 0.4250 0.4250 0.4250
11 0.0558 0.30 0.50 0.70 0.50
0.1500 0.1620 0.2880 0.2640 3.0110 4.6190 4.4940 4.2060 2.3230 0.6900 0.1900 0.1900

An example of a cell in the veg param file cell with irrigated vegetation can be seen below. There
should be one extra 1 on the first line, + information on percent irrigated area pr month on last line.
Number of vegetation types is, as you can see, only 6, although there may seem to be information on 7
types listed. The vegetation type "110" is irrigated vegetation. The percentage tells the model how
much of the area equipped for irrigation within that cell (0.0487+0.0487) that is actually irrigated that
month (information taken from e.g. FAO). The last line is the cropping calendar, i.e. how much of the
area equipped for irrigation is actually irrigated that month. Do not go below 1 percent or above 99
percent on the last line!

20376 6 1
1 0.0410 0.30 0.30 0.70 0.70
4.8780 4.8780 4.8780 4.8780 4.8780 4.8780 4.8780 4.8780 4.8780 4.8780 4.8780 4.8780
6 0.0608 0.30 0.60 0.70 0.40
0.3380 0.3630 0.3630 0.3750 0.7880 2.1250 3.1500 2.6370 1.4620 0.6250 0.3870 0.3870
7 0.2061 0.30 0.60 0.70 0.40
0.4630 0.6500 0.9880 1.9380 3.3630 3.2370 2.6630 1.8120 1.9250 1.1380 0.6130 0.3380
10 0.0230 0.30 0.80 0.70 0.20
0.2250 0.3250 0.4750 0.8370 2.4000 3.1380 2.9380 2.1250 1.7000 1.0500 0.4500 0.2500
11 0.5719 0.30 0.50 0.70 0.50
0.1080 0.1560 0.2470 0.5410 2.1790 3.6170 3.1640 1.2390 0.9970 0.6120 0.3250 0.1750
110 0.0487 0.30 0.50 0.70 0.50
5
0.1000 0.1000 0.1000 0.1000 0.2230 0.9340 2.2950 2.4310 1.0530 0.1000 0.1000 0.1000
110 0.0487 0.30 0.50 0.70 0.50
0.1000 0.1000 0.1000 0.1000 0.2230 0.9340 2.2950 2.4310 1.0530 0.1000 0.1000 0.1000
10.0 10.0 10.0 10.0 80.0000 80.0 80.0 80.0 80.0 10.0 10.0 10.0

Vegetation library
Same as 4.0.6 (but you may have to add vegetation types, e.g. I've added information on the vegetation
type "110".

Soil file
Three extra columns are added to the standard 4.0.6 soil file. These columns have nothing to do with
the irrigation scheme; they are included for flexibility purposes in the VIC runs. Given three soil
layers, the added columns are: Col 54: Currently not used (although it may seem so if you read
read_soilparam.c). Col 55: options.BASEFLOW (overrules info in global file). Col 56:
options.ROOT_ZONES (overrules info in global file).

-  ROUTING WITH RESERVOIRS (ROUT_NEW)
You need to customize �ReadDataForReservoirEvaporation.c�! Reads VIC daily binary output files,
expects a certain number and order of fluxes in binary files. In the version you�ve got: Commented out.

You need to make links in the directory where you run the routing model to "run_moscem" and
"run_moscem_leapyear" (moscem related files; included in the .tar file).

All arcinfo-type input files must have the header included. Preprocessing scripts expects a �0� at the
outlet cells in the direction files.

You must first run the model for naturalized situation, and in this case OUT_FILE_PATH,
WORK_PATH and NAT_PATH (in routing input file) should be the location of the output files.
When including reservoirs, set OUT_FILE_PATH and WORK_PATH to another directory, and
NAT_PATH should point to the directory of the naturalized simulations.
Column number 3 in *.month (output file) is outflow from the reservoir.
Comparison to the fortran-version of the code (changes made and added features)

The routing input file has to be similar to the one above. I.e. list the input files in the same order,
and keep the structure as is.

Two columns are added to the station information file. The second number on the line gives
information on whether the station is already routed (1). If so, it won't be routed again. If you set
the number to 0, the routing program assumes you want to rout the area again. The right-most
column tells the routing program what kind of cell we are dealing with. 1: Regular cell
(nonirrigated part), 2: Dam, 3: Irrigated part of cell. NB! NB! NB! You must list the stations from

upstream to downstream locations! NB! NB! NB! Be aware that this causes changes to the .uh_s
file!!!!!

The scheme produces streamflow in the cells you tell it to (as in the fortran version). The tutorial
setup is customized to my phd needs, for which I only needed the irrig cells, the dam cells, gage
locations, and the outlet cell. The selection of cells was also kept to a limited number in order to
save CPU time, which was limited when I did my phd. If you want routing to be performed in all
cells, you have to give the routing scheme that information. Remember that this version needs the
cells to be routed listed from upstream to downstream location! You can define it directly in
the .sta file; as in the fortran version. The scripts are manufactured to produce the .sta file from
other sources of information, though, so you may have to skip some of the flags. However,
possibly you use the routing scheme as a stand-alone model in which case you can put the info
directly in the .sta file as you are used to. Except the upstream-to-downstream thing, in order to
keep the effect of reservoirs also in downstream reaches (among other things).

For each routed location, a file called 'streamflow_lat_lon' is made. It will be located in the
'Output_files' directory. Units m3/s, time step daily.

The output is written to m3/s instead of ft3/s.

New feature: Skips days in VIC simulation files that won't be routed. i.e. ndays is number of days
to be routed, not number of days to be routed + number of simulation days before routing starts,
which it used to be. NB! This may result in less runoff at the beginning of the routing period (spinup)!!

Areas already routed won't be routed again. I.e. if an upstream station location has been routed
during this run, or previously (see above).

If you want to do the routing at a location upstream other gauges, you have to make sure the
'Already routed' column is set to 0 at the downstream location.......oh, bad programming�...

Daily results can be somewhat different when routing multiple locations at once, compared to
routing one by one. Monthly files seem ok, though.

The VIC simulation flux files have to be ascii files with the following columns: Year Month Day
Runoff Baseflow

References:
Haddeland, I., 2006a, Anthropogenic impacts on the continental water cycle, No 553, Series of
dissertations submitted to the Faculty of Mathematics and Natural Sciences, University of
Oslo, Norway.
Haddeland, I., T. Skaugen, and D.P. Lettenmaier, 2006b, Anthropogenic impacts on continental
surface water fluxes, Geophys. Res. Lett., 33(8), Art. No. L08406,
doi:10.1029/2006GL026047


----------
**Added by Tian Zhou**

- The steps to run the reservoir model
1.	The base directory of the reservoir model is located in /net/power/raid/tizhou/BASE
2.	To run a new basin (e.g. Colorado), please create a new directory (e.g. Colorado_run) and copy everything in BASE to the new directory.
3.	Update 3 files for your basin:
a) flow direction file /data/rivernetwork/ddm30_watch_2009/dir30min_overlap/38801.dir
b) fraction file /data/rivernetwork/ddm30_watch_2009/frac30min/38801.frac
c) mask file /data/rivernetwork/ddm30_watch_2009/mask30min/38801.xmask
4.	Run the "makeexe.sh" in BasinRun directory to change the permission of executables
5.	Run "run_irrig.sh 38801 2 -b -c -d -e -f -g -h -i -j ("2" indicates no reservoir no irrigation scenario)
6.	Run "run_irrig.sh 38801 5 -j". ("5" indicates reservoir and irrigation scenario)
7.	The results are located in /run/rout/output/wfd/baseline/irrig.res.wb.24hr/

Notes:
1.	Before running the reservoir model, two sets of vic runs (regular run and water free run) must be completed. The global parameter files, soil parameters, and results are located in /net/power/raid/tizhou/Global_run
2.	The forcing data are located in /net/power/raid/tizhou/Global_met/asc_vicinp
3.	Most of the paths, file names are defined in /run_irrig.sh
4.	There are five scenarios: 1) irrigation without reservoir; 2) no irrigation no reservoir; 3) free water irrigation no reservoir; 4) no irrigation with reservoir; and 5) irrigation and reservoir.
5.	In most cases we just need to run scenarios 2 and 5. The purpose to run 2 is to derive the minimum flow (7Q10) and the annual flood (bankfull flow over a year).
6.	Step -b generates frac.txt in /BasinRun and soil.current in /run/input/soil
7.	Step -c generates irri.38801.gmt and irr.38801.asc in working directory
8.	Step -d copies the forcing data from the Global_met directory to /data/met/asc_vicinp and the vic results from Global_run directory to /run/output/wfd/baseline/
9.	Step -e generates 38801.reservoirs.firstline and 38801.points in /run/rout/input
10.	Step -f generates 38801.reservoirs.upstream and 38801.reservoirs.extractwater in working directory. It also generates XXX.cal.irrdemand.monthly (XXX denotes the dam name) in /data/dames/unh/2000
11.	Step -g generates n files in /BasinRun/run/input/soil/points; n is the number of cells with irrigation fraction >0
12.	Step -h converts the binary vic output to ascii in /run/output/daily.ascii
13.	Step -I generates upstreamcells.txt in working directory
14.	Step -j runs the run_vic.sh in /programs/scripts
15.	Run_vic.sh uses most of the scripts and C programs located in /programs
16.	Run_vic.sh goes through the irrigated cells from upstream to downstream.

Source code:
1.	VIC : /models/vic
2.	ROUT : /models/rout/rout_new
3.	MOSCEM: /models/rout/moscem
4.	Misc c programs: /programs/C
  a)	clipbinaryfluxes.c
  b)	col2grid.c
  c)	dams.find.irrigationwaterdemand.c
  d)	dams.find.irrigationwaterdemand.c.withskip
  e)	dams.gages.withinbasin.makeroutinput.c
  f)	find.upstreamcells.c
  g)	fluxdata.binary.to.daily.ascii.allcolc.c
  h)	fluxdata.binary.to.daily.ascii.c
  i)	fluxdata.binary.to.daily.ascii.c.withskip
  j)	metdata.modify.runoff.c
  k)	routing.modifystationfile.c
  l)	routing.subtract.water.used.for.irrigation.c
  m)	routing.subtract.water.used.for.irrigation.c~
  n)	soilfile.rearrangepoints.c
5.	Misc scripts: /programs/scripts
  a)	copy_files.sh
  b)	global.file.modify.sh
  c)	latlon.from.fractionfile.sh
  d)	make_irrig_subset.sh
  e)	make_llf.sh
  f)	make_routing_input_files.sh
  g)	make_soil_subset.sh
  h)	routing.modify.inputfile.sh
  i)	run_vic.sh
  j)	soilfile.extract.cells.sh
  k)	upstream.extract.cell.sh
