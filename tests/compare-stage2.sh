#!/bin/bash
source ./common.sh

TITLE0="Stage 2 output comparison test"
echo "${CG}${TITLE0} begin${RC}"

source ./compile-gcc.sh
source ./compile-fc-2.sh

IFILE1=$ODIR/sample_gcc
IFILE2=$ODIR/sample_fc
OFILE1=$TDIR/temp1
OFILE2=$TDIR/temp2

echo "Executing key sample..."
./$IFILE1 > $OFILE1
echo "Executing produced sample..."
./$IFILE2 > $OFILE2
echo "Calculating diff..."
if ! cmp $OFILE1 $OFILE2; then
	echo "${CR}Files not matching${RC}"
	exit 1
else
	echo "...files match"
fi
echo "Clean up"
rm $IFILE1 $IFILE2 $OFILE1 $OFILE2

echo "${CG}${TITLE0} end${RC}"
