/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_JSONHEADER_H_
#define ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_JSONHEADER_H_

#include <string>

// Todo ACSDK-443: Move the JSON to text file
/// This is a basic synchronize JSON message which may be used to initiate a connection with AVS.
// clang-format off
static const std::string SYNCHRONIZE_STATE_JSON =
    "{"
        "\"context\":[{"
            "\"header\":{"
                "\"name\":\"SpeechState\","
                "\"namespace\":\"SpeechSynthesizer\""
            "},"
            "\"payload\":{"
                "\"playerActivity\":\"FINISHED\","
                "\"offsetInMilliseconds\":0,"
                "\"token\":\"\""
            "}"
        "}],"
        "\"event\":{"
            "\"header\":{"
                "\"messageId\":\"00000000-0000-0000-0000-000000000000\","
                "\"name\":\"SynchronizeState\","
                "\"namespace\":\"System\""
            "},"
            "\"payload\":{"
            "}"
        "}"
    "}";
// clang-format on

#endif  // ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_JSONHEADER_H_
