#!/bin/bash

if [[ $1 != "" ]]
then
    prog=./speed_$1
else
    prog=./speed
fi

iterations=10
tmp=tmp.csv
result=result.csv

echo "running $prog"
rm -f $tmp
for i in `seq $iterations`
do
    echo "iteration $i"
    $prog --csv | tee -a $tmp
done

mv $tmp $result
