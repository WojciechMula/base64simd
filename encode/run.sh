#!/bin/bash

if [[ $1 == "avx2" ]]
then
    prog=./speed_avx2
else
    prog=./speed
fi

iterations=5
tmp=raw-results.txt
results=results.txt

rm -f $tmp
for i in `seq $iterations`
do
    echo "iteration $i"
    $prog | tee -a $tmp
done

cp $tmp $results

python script/print_table.py $results
