#!/bin/bash

source ./bgx.sh

cpunum=$(grep -c ^processor /proc/cpuinfo)
rm -rf buildlog
mkdir -p buildlog

runwithdirection() {
    file=$1; shift
    $* >$file 2>&1 
}

group1=""
for project in $*; do
#for project in lua; do
    for method in WinMut AccMut validate timing; do
        echo -e $(date | awk '{print $4}') "\t[info]\tstarted building ${project}/${method}"
        bgxgrp=${group1}; bgxlimit ${cpunum} runwithdirection "buildlog/${project}-${method}.log" ./run.sh ${project} build ${method} ;
        group1=${bgxgrp}
    done
    # echo $(date | awk '{print $4}') '[' ${group1} ']'
done

# Wait until all others are finished.

echo -e $(date | awk '{print $4}') "\t[info]\t waiting unfinished jobs"
bgxgrp=${group1}; bgxupdate; group1=${bgxgrp}
while [[ ${bgxcount} -ne 0 ]]; do
    oldcount=${bgxcount}
    while [[ ${oldcount} -eq ${bgxcount} ]]; do
        sleep 1
        bgxgrp=${group1}; bgxupdate; group1=${bgxgrp}
    done
done


