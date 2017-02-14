#!/bin/bash

SCRIPT_PATH=${0%/*}

for datadate in `cat $1`; do
    $SCRIPT_PATH/run_g.sh $datadate	
done
