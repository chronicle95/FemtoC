#!/bin/bash
source ./common.sh
TITLE="Smoke fail test"
IFILE=$IDIR/smoke-fail
OFILE=$ODIR/smoke-fail
echo "${CG}${TITLE} begin${RC}"
echo "Compiling..."
cat $IFILE.fc | ../cc > $OFILE.s
if ! grep -q "type expected" $OFILE.s; then
	echo "${CR}Test failed: error message not found${RC}"
	exit 1
fi
echo "Clean up"
rm $OFILE.s
echo "${CG}${TITLE} end${RC}"
