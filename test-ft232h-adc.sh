#!/bin/sh

SERIAL=`motelist 2>/dev/null | grep DLRC | awk '{print $1}'`
if test -z "$SERIAL" 
then
	echo "no device found"
fi

testdir='test-data'
test -d $testdir && rm -rf $testdir
mkdir $testdir

for serial in $SERIAL
do
	while read con tms
	do
		echo "test $serial $con $tms ..."
		./ft232h-adc $serial $con $tms > "$testdir/$serial-$con-$tms.txt"
	done <<EOF
0 10
0 500
0 1000
1 10
1 500
1 1000
EOF
done

