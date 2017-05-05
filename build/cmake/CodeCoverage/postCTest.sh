#!/bin/bash
if [ $# -ne 7 ]; then
    printf "USAGE:\n\
        $0 CURRENT_BINARY_DIR CURRENT_DATA_FILE CURRENT_REPORT_FOLDER TOP_BINARY_DIR TOP_DATA_FILE \
                TOP_REPORT_FOLDER TOP_REPORT\n"
    exit 1
fi

# Current project name, derived by the current project folder.
CURRENT_PROJECT_NAME=$(basename $3)
# Top project name, derived by the top project folder.
TOP_PROJECT_NAME=$(basename $6)
# Current project's binary directory.
CURRENT_BINARY_DIR=$1
# Top project's binary directory
TOP_BINARY_DIR=$4
# Current project's target data filename.
CURRENT_DATA_FILE=$2
# Top project's target data filename.
TOP_DATA_FILE=$5
# Current project's target report folder.
CURRENT_REPORT_FOLDER=$3
# Top project's target report folder.
TOP_REPORT_FOLDER=$6
# Top project's target report filename.
TOP_REPORT=$7
# The list of excluded libraries.
EXCLUDED_LIBRARIES=( '"ThirdParty*"' '"/usr*"' )
# Collect the coverage data relative base directory.
lcov -b $CURRENT_BINARY_DIR -d $CURRENT_BINARY_DIR -c --derive-func-data -o $CURRENT_DATA_FILE
# Discard non-SDK libraries out of the coverage data.
echo ${EXCLUDED_LIBRARIES[@]} | xargs lcov -o $CURRENT_DATA_FILE -r $CURRENT_DATA_FILE
# Generate/update coverage report HTML files for the current project.
genhtml --demangle-cpp -o $CURRENT_REPORT_FOLDER -t "${CURRENT_PROJECT_NAME} Coverage" --num-spaces 4 $CURRENT_DATA_FILE
if [ "$CURRENT_PROJECT_NAME" != "$TOP_PROJECT_NAME" ]; then
    # Collect the coverage data relative base directory.
    lcov -b $TOP_BINARY_DIR -d $TOP_BINARY_DIR -c --derive-func-data -o $TOP_DATA_FILE
    # Discard non-SDK libraries out of the coverage data.
    echo ${EXCLUDED_LIBRARIES[@]} | xargs lcov -o $TOP_DATA_FILE -r $TOP_DATA_FILE
    # Generate/update coverage report HTML files for the top-level project.
    genhtml --demangle-cpp -o $TOP_REPORT_FOLDER -t "${TOP_PROJECT_NAME} Coverage" --num-spaces 4 $TOP_DATA_FILE
fi
# Create a redirect HTML for the coverage report of the top-level project.
echo "<meta http-equiv=\"refresh\" content=\"0; url=file://$TOP_REPORT_FOLDER/index.html\" />" > $TOP_REPORT
