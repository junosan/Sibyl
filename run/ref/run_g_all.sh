#!/bin/bash

SCRIPT_PATH=${0%/*}
ZIP_ROOT=$SCRIPT_PATH/../../..

for datadate in `ls $ZIP_ROOT/Data | sed 's/\(.*\)\..*/\1/'`; do
	if [ -f $ZIP_ROOT/Data/$datadate.zip ]; then
		$SCRIPT_PATH/run_g.sh $datadate	
	fi
done