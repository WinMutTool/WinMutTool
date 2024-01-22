#!/bin/bash

declare -A bgxnames

# bgxupdate - update active processes in a group.
#   Works by transferring each process to new group
#   if it is still active.
# in:  bgxgrp - current group of processes.
# out: bgxgrp - new group of processes.
# out: bgxcount - number of processes in new group.

bgxupdate() {
    bgxoldgrp=${bgxgrp}
    bgxgrp=""
    ((bgxcount = 0))
    bgxjobs=" $(jobs -pr | tr '\n' ' ')"
    for bgxpid in ${bgxoldgrp} ; do
        echo "${bgxjobs}" | grep " ${bgxpid} " >/dev/null 2>&1
        if [[ $? -eq 0 ]]; then
            bgxgrp="${bgxgrp} ${bgxpid}"
            ((bgxcount++))
        else
            wait $bgxpid
	    ret=$?
	    if [[ $ret -ne 0 ]]; then
                echo -e $(date | awk '{print $4}') "\t[warn]\tcommand '${bgxnames[$bgxpid]}' exited with non-zero exit code ${ret}"
            else
                echo -e $(date | awk '{print $4}') "\t[info]\tcommand '${bgxnames[$bgxpid]}' exited normally"
	    fi
        fi
    done
}

# bgxlimit - start a sub-process with a limit.

#   Loops, calling bgxupdate until there is a free
#   slot to run another sub-process. Then runs it
#   an updates the process group.
# in:  $1     - the limit on processes.
# in:  $2+    - the command to run for new process.
# in:  bgxgrp - the current group of processes.
# out: bgxgrp - new group of processes

bgxlimit() {
    bgxmax=$1; shift
    bgxupdate
    while [[ ${bgxcount} -ge ${bgxmax} ]]; do
        sleep 1
        bgxupdate
    done
    if [[ "$1" != "-" ]]; then
        echo -e $(date | awk '{print $4}') "\t[info]\texecute command '$*'"
        $* &
	pid=$!
	bgxnames[$pid]=$*
        bgxgrp="${bgxgrp} $pid"
    fi
}

