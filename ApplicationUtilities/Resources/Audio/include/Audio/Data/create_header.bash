#!/bin/bash

# This is used to create a header file that contains binary data from a file that can be used in this library.  The
# files are downloaded from https://developer.amazon.com/docs/alexa-voice-service/ux-design-overview.html#sounds.

if [ "$#" -ne 2 ] || ! [ -f "${1}" ] || [ -f "${2}" ]; then
  echo "Usage: $0 <file> <output_header_file>" >&2
  exit 1
fi

FILE=${1}
FULL_OUTPUT=${2}
OUTPUT_FILE=$(basename ${FULL_OUTPUT})
GUARD=`echo "ALEXA_CLIENT_SDK_${FULL_OUTPUT}" | tr '[:lower:]' '[:upper:]' | sed -e 's/[^a-zA-Z0-9\]/_/g' | sed -e 's/$/_/g'`

cat <<EOF > ${FULL_OUTPUT}
/*
 * ${OUTPUT_FILE}
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

/*
 * ******************
 * ALEXA AUDIO ASSETS
 * ******************
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates ("Amazon").
 * All Rights Reserved.
 *
 * These materials are licensed to you as "Alexa Materials" under the Alexa Voice
 * Service Agreement, which is currently available at
 * https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/support/terms-and-agreements.
 */
EOF

echo "" >> ${FULL_OUTPUT}
echo "#ifndef ${GUARD}" >> ${FULL_OUTPUT}
echo "#define ${GUARD}" >> ${FULL_OUTPUT}
echo "" >> ${FULL_OUTPUT}
echo "namespace alexaClientSDK {" >> ${FULL_OUTPUT}
echo "namespace applicationUtilities {" >> ${FULL_OUTPUT}
echo "namespace resources {" >> ${FULL_OUTPUT}
echo "namespace audio {" >> ${FULL_OUTPUT}
echo "namespace data {" >> ${FULL_OUTPUT}
echo "" >> ${FULL_OUTPUT}
echo "// clang-format off" >> ${FULL_OUTPUT}
xxd -i ${FILE} >> ${FULL_OUTPUT}
echo "// clang-format on" >> ${FULL_OUTPUT}
echo "" >> ${FULL_OUTPUT}
echo "#endif  // ${GUARD}" >> ${FULL_OUTPUT}
echo "" >> ${FULL_OUTPUT}
echo "}  // namespace data" >> ${FULL_OUTPUT}
echo "}  // namespace audio" >> ${FULL_OUTPUT}
echo "}  // namespace resources" >> ${FULL_OUTPUT}
echo "}  // namespace applicationUtilities" >> ${FULL_OUTPUT}
echo "}  // namespace alexaClientSDK" >> ${FULL_OUTPUT}

