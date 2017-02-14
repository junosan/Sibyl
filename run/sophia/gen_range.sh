#!/bin/bash
# Generate ($1 : $2 : $3) in MATLAB syntax; any of $# can be floating point
# Assumes $3 >= $1 and $2 > 0

IDX=0
while true; do
    val=`echo "$1 + $IDX * $2" | bc | awk '{printf "%f", $0}'`
    if [ `echo "$val > $3" | bc` == 1 ]; then
        break
    fi
    echo $val
    ((IDX++))
done
