#!/bin/bash

SCRIPT_PATH=${0%/*}
SCREEN_WIDTH=160
STATE_FILE=$SCRIPT_PATH/../../bin/state/client_cur.log
SPLIT_LINE=70

TMP_ROOT=/tmp/Sibyl_display_only

rm -rf $TMP_ROOT
mkdir -p $TMP_ROOT

head -n  $SPLIT_LINE $STATE_FILE > $TMP_ROOT/left.log
tail -n +$SPLIT_LINE $STATE_FILE > $TMP_ROOT/right.log

watch -n1 pr -w $SCREEN_WIDTH -m -t $TMP_ROOT/left.log $TMP_ROOT/right.log
