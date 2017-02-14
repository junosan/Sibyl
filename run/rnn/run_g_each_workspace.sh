#!/bin/bash

SCRIPT_PATH=${0%/*}
BIN_TO_SCRIPT_PATH=../run/rnn
WORKSPACE_DIR=../../../Trade/fractal/train/run

for workspace in `ls $WORKSPACE_DIR`; do
    if [ -d $WORKSPACE_DIR/$workspace ]; then
        echo $workspace
        rm -rf workspace.list
        echo $BIN_TO_SCRIPT_PATH/$WORKSPACE_DIR/$workspace > workspace.list
        $SCRIPT_PATH/run_g_testset.sh
    fi
done
