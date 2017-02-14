#!/bin/bash

SCRIPT_PATH=${0%/*}
BIN_PATH=$SCRIPT_PATH/../../bin
RUN_PATH=$SCRIPT_PATH/../../run

ZIP_ROOT=$SCRIPT_PATH/../../..
ZIP_DATA=$ZIP_ROOT/Data/$1.zip
ZIP_DATAREF=$ZIP_ROOT/Data0/DataG/$1.zip

TEMP_ROOT=/tmp/Sibyl
TEMP_DATA_PATH=$TEMP_ROOT/Data/$1
TEMP_DATAREF_PATH=$TEMP_ROOT/DataRef/$1

TCP_PORT=50505

if [ -f $ZIP_DATA ] && [ -f $ZIP_DATAG ]; then
	rm -rf $TEMP_ROOT

	mkdir -p $TEMP_DATA_PATH
	unzip -qq -d $TEMP_DATA_PATH $ZIP_DATA
	
	mkdir -p $TEMP_DATAREF_PATH
	unzip -qq -d $TEMP_DATAREF_PATH $ZIP_DATAREF
	
	printf '%s\t' $1

	$BIN_PATH/simserv $RUN_PATH/KOSPI.config $TEMP_DATA_PATH $TCP_PORT &

	sleep 1

	$BIN_PATH/refclnt $RUN_PATH/reward.config $TEMP_DATAREF_PATH 127.0.0.1 $TCP_PORT

	rm -rf $TEMP_ROOT
fi
