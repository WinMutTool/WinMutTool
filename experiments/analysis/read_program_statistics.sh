#!/bin/bash

PROJECT_BASE=$1

MUTNUM=$(find ${PROJECT_BASE} -name '*.c.mut' | xargs cat | wc -l)
echo "Mut num: ${MUTNUM}"
BBNUM=$(find ${PROJECT_BASE} -name '*.c.stat' | xargs cat | wc -l)
echo "Basic block num: ${BBNUM}"
SPLITNUM=$(find ${PROJECT_BASE} -name '*.c.stat' | xargs cat | awk -F ':' '{
if ($1 !="0") {
sum+=1
sum+=$3
} else {
sum+=$3
}
}
END { print sum }')
echo "Split num: ${SPLITNUM}"
INSTNUM=$(find ${PROJECT_BASE} -name '*.c.stat' | xargs cat | awk -F ':' '{ sum+=$1; sum+=$2; sum+=$3} END { print sum }')

MUTPERSPLIT=$(echo "scale=2; ${MUTNUM} / ${SPLITNUM}" | bc)
echo "Mut per split: $MUTPERSPLIT"
MUTPERINST=$(echo "scale=2; ${MUTNUM} / ${INSTNUM}" | bc)
echo "Mut per inst: $MUTPERINST"



