#!/bin/bash

SCRIPT_PATH=${0%/*}
LIST_PATH=$SCRIPT_PATH/batch

for filename in `ls $LIST_PATH`; do
    cp $LIST_PATH/$filename $SCRIPT_PATH/workspace.list
    cat $SCRIPT_PATH/workspace.list
    $SCRIPT_PATH/run_g_list.sh comb.txt	
done
