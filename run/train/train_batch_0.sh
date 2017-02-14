#!/bin/bash

SCRIPT_PATH=${0%/*}
BIN_PATH=$SCRIPT_PATH/../../bin
HOME_PATH=$SCRIPT_PATH/../../..
DATA_PATH=$HOME_PATH/MATLAB/fractal/DataV
WORKSPACE_DIR=$HOME_PATH/workspace

NAME=20161228_Fractal_22800sec_90day

IDX_LIST=`echo 0`
export CUDA_VISIBLE_DEVICES=0

mkdir -p $WORKSPACE_DIR

for idx in $IDX_LIST; do
    WORKSPACE=$WORKSPACE_DIR/workspace_$NAME"_"$idx
    LOG_FILE=$WORKSPACE_DIR/$NAME"_"$idx".log"
    $BIN_PATH/train -t  $DATA_PATH $WORKSPACE    | tee -a $LOG_FILE 
    cp -r $WORKSPACE $WORKSPACE"r"
    $BIN_PATH/train -tc $DATA_PATH $WORKSPACE"r" | tee -a $LOG_FILE 
done
