#!/bin/bash
if [ $# -ne 3 ]; then
    printf "USAGE:\n\
        $0 CURRENT_BINARY_DIR TOP_BINARY_DIR TOP_DATA_FILE"
    exit 1
fi

# Current project's binary directory.
CURRENT_BINARY_DIR=$1
# Top project's binary directory
TOP_BINARY_DIR=$2
# Top project's target data filename.
TOP_DATA_FILE=$3
# Reset coverage counters on the top binary directory if the coverage data file does not exist.
if [ ! -f $TOP_DATA_FILE ]; then
    lcov -b $TOP_BINARY_DIR -d $TOP_BINARY_DIR -z -q
fi
# Reset coverage counters on the current directory if it is not the same as the top directory.
if [ "$CURRENT_BINARY_DIR" != "$TOP_BINARY_DIR" ]; then
    lcov -b $CURRENT_BINARY_DIR -d $CURRENT_BINARY_DIR -z -q
fi
