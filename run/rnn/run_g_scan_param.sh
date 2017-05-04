#!/bin/bash

SCRIPT_PATH=${0%/*}
CONFIG_FILE=$SCRIPT_PATH/../reshaper_0.config
DATE_LIST=$SCRIPT_PATH/date.list
LOG_FILE=$SCRIPT_PATH/scan_param.log

rm -f $LOG_FILE

for b_th in     `$SCRIPT_PATH/gen_range.sh 0.8  0.1  1.1 `; do
    for s_th in `$SCRIPT_PATH/gen_range.sh 0.5  0.1  0.8 `; do
        #if [ `echo "$b_th % 0.1 == 0 && $s_th % 0.1 == 0" | bc` == 0 ]; then
        rm -f $CONFIG_FILE
        touch $CONFIG_FILE
        
        echo B_TH=$b_th >> $CONFIG_FILE
        echo S_TH=$s_th >> $CONFIG_FILE
        
        echo ----------------                 | tee -a $LOG_FILE
        date                                  | tee -a $LOG_FILE
        echo B_TH=$b_th                       | tee -a $LOG_FILE
        echo S_TH=$s_th                       | tee -a $LOG_FILE
        $SCRIPT_PATH/run_g_list.sh $DATE_LIST | tee -a $LOG_FILE
        #fi
    done
done
