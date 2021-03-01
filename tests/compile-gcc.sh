#!/bin/bash
source ./common.sh
IFILE=$IDIR/sample
OFILE=$ODIR/sample_gcc
echo "Building the key sample with GCC..."
if ! gcc $IFILE.c -o $OFILE; then
	echo "${CR}Compilation failed${RC}"
	exit 1
fi
