#!/bin/bash
source ./common.sh
CCSRC=../c
CCOUT=../cc0
IFILE=$IDIR/sample
OFILE=$ODIR/sample_fc
echo "Compiling FemtoC with FemtoC..."
cat $CCSRC.c | ../cc > $CCOUT.s
if ! grep -q "no errors encountered" $CCOUT.s; then
	echo "${CR}Compilation failed${RC}"
	exit 1
fi
echo "Assembling..."
if ! as $CCOUT.s -o $CCOUT.o; then
	echo "${CR}Assembly failed${RC}"
	exit 1
fi
echo "Linking..."
if ! ld $CCOUT.o -o $CCOUT; then
	echo "${CR}Linkage failed${RC}"
	exit 1
fi
echo "Compiling sample with stage 2 FemtoC..."
cat $IFILE.c | $CCOUT > $OFILE.s
if ! grep -q "no errors encountered" $OFILE.s; then
	echo "${CR}Compilation failed${RC}"
	exit 1
fi
echo "Assembling..."
if ! as $OFILE.s -o $OFILE.o; then
	echo "${CR}Assembly failed${RC}"
	exit 1
fi
echo "Linking..."
if ! ld $OFILE.o -o $OFILE; then
	echo "${CR}Linkage failed${RC}"
	exit 1
fi
echo "Clean up"
rm $OFILE.o $OFILE.s $CCOUT*
