#!/bin/bash

SCRIPT_PATH=${0%/*}
ZIP_ROOT=$SCRIPT_PATH/../../..

for datadate in `ls $ZIP_ROOT/Data | sed 's/\(.*\)\..*/\1/'`; do
	if [ -f $ZIP_ROOT/Data/$datadate.zip ] && [ $datadate -ge 20151118 ]; then
		$SCRIPT_PATH/run_e.sh $datadate	
	fi
done