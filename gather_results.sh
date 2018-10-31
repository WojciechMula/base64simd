#!/bin/bash

function usage
{
    echo "$0 [-a sse|avx2|avx512|avx512bw|avx512vbmi] [-n name] [-k]"
}

BASENAME=results
ARCHITECTURE=sse
TARGET=benchmark
KEEP=0

while getopts ":a:n:hk" o
do
    case "${o}" in
        a)
            ARCHITECTURE=${OPTARG}
            if [[ ${ARCHITECTURE} == "sse" ]]
            then
                TARGET=benchmark
            elif [[ ${ARCHITECTURE} == "avx2"
                 || ${ARCHITECTURE} == "avx512"
                 || ${ARCHITECTURE} == "avx512bw"
                 || ${ARCHITECTURE} == "avx512vbmi" ]]

            then
                TARGET=benchmark_${ARCHITECTURE}
            else
                usage
                exit 1
            fi
            ;;
        n)
            BASENAME=${OPTARG}
            ;;
        k)
            KEEP=1
            ;;
        h)
            usage
            exit 0
            ;;
    esac
done

ENCODE_RESULTS_PATH=encode_${BASENAME}.txt
DECODE_RESULTS_PATH=decode_${BASENAME}.txt
ENCODE_TABLE_PATH=encode_${BASENAME}.table
DECODE_TABLE_PATH=decode_${BASENAME}.table
META_PATH=${BASENAME}.meta
ARCHIVE_PATH=${BASENAME}.tgz


function build_benchmark
{
    make -C encode ${TARGET}
    make -C decode ${TARGET}
}


function run_benchmark
{
    local ALGORITHM=$1
    local RESULTS_PATH=$2
    local LOOPS=3
    echo "Running ${ALGORITHM} benchmark..."
    rm -f ${RESULTS_PATH}
    for i in `seq ${LOOPS}`
    do
        ./${ALGORITHM}/${TARGET} >> ${RESULTS_PATH}
    done
}


function get_metadata
{
    CPUINFO=$(awk -F: '/^model name/ {
        gsub(/^[ ]+/, "", $2);  # remove leading spaces
        gsub(/[ ]+$/, "", $2);  # remove trailing spaces
        gsub(/[ ]+/, " ", $2);  # normalize spaces
        print $2;
        exit                    # print just 1st CPU
    }' < /proc/cpuinfo)

    GCCVERSION=$(gcc --version | head -n1)

    rm -f ${META_PATH}
    echo "CPU: ${CPUINFO}" >> ${META_PATH}
    echo "GCC: ${GCCVERSION}" >> ${META_PATH}
}


function create_table
{
    local RESULTS_PATH=$1
    local TABLE_PATH=$2
    python scripts/format.py ${RESULTS_PATH} > ${TABLE_PATH}
}


function create_archive
{
    local files="${META_PATH} ${ENCODE_RESULTS_PATH} ${ENCODE_TABLE_PATH} ${DECODE_RESULTS_PATH} ${DECODE_TABLE_PATH}"
    if [[ ${KEEP} == 1 ]]
    then
        tar -czf ${ARCHIVE_PATH} ${files}
        echo "File '${ARCHIVE_PATH}' was created"
    else
        tar --remove-files -czf ${ARCHIVE_PATH} ${files}
        echo "Files ${files} moved to archive ${ARCHIVE_PATH}"
    fi
}

build_benchmark
run_benchmark encode ${ENCODE_RESULTS_PATH}
run_benchmark decode ${DECODE_RESULTS_PATH}
create_table ${ENCODE_RESULTS_PATH} ${ENCODE_TABLE_PATH}
create_table ${DECODE_RESULTS_PATH} ${DECODE_TABLE_PATH}
get_metadata
create_archive
