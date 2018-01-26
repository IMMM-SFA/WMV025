#!/bin/tcsh

set fracfile = ${argv[1]};
set globalrunpath = ${argv[2]};
set localrunpath = ${argv[3]};
set globalmetpath = ${argv[4]};
set localmetpath = ${argv[5]};
set totaldays = ${argv[6]};
set copydays = ${argv[7]};
set skipdays = ${argv[8]};
set simstartyear = ${argv[9]};
set endyear = ${argv[10]};
set bindir = ${argv[11]};

set lon = `awk '{ print($1) } ' $fracfile `;
set lat = `awk '{ print($2) } ' $fracfile `;

#echo $lon
#echo $lat

set n = `echo $lon | awk '{n=split($0, array, " ")} END{print n }'`

echo $n files need to be copied to basin free no irr directories
rm $localrunpath/freeirrig.wb.24hr/*;
rm $localrunpath/noirrig.wb.24hr/*;
rm $localmetpath/*;

set headcut = `echo "$skipdays + $copydays" | bc`
echo $skipdays $copydays $headcut

@ i = 1
while ($i <= $n)
  set vicfilename = fluxes_$lat[$i]_$lon[$i];
  set metfilename = forcings_$lat[$i]_$lon[$i];
  echo copying $vicfilename 
  echo $globalrunpath/freeirrig.wb.24hr/$vicfilename
  $bindir/clipbinaryfluxes $globalrunpath/freeirrig.wb.24hr/$vicfilename $localrunpath/freeirrig.wb.24hr/$vicfilename $skipdays $copydays $totaldays
  $bindir/clipbinaryfluxes $globalrunpath/noirrig.wb.24hr/$vicfilename $localrunpath/noirrig.wb.24hr/$vicfilename $skipdays $copydays $totaldays
  echo copying $metfilename from $simstartyear to $endyear which is $copydays days 
  head -n $headcut $globalmetpath/$metfilename >! $localmetpath/temp
  tail -n $copydays $localmetpath/temp >! $localmetpath/$metfilename
  #cp $globalmetpath/$metfilename $localmetpath;
  @ i += 1
end

echo 'done with copy'
