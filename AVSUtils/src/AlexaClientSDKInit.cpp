/*
 * AlexaClientSDKInit.h
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

#include "AVSUtils/Initialization/AlexaClientSDKInit.h"
#include "AVSUtils/Logging/Logger.h"

#include <curl/curl.h>

namespace alexaClientSDK {
namespace avsUtils {
namespace initialization {
/// Tracks whether we've initialized the Alexa Client SDK or not
std::atomic_int AlexaClientSDKInit::g_isInitialized{0};

bool AlexaClientSDKInit::isInitialized() {
    return g_isInitialized > 0;
}

bool AlexaClientSDKInit::initialize() {
    if (CURLE_OK != curl_global_init(CURL_GLOBAL_ALL)) {
        Logger::log("Could not initialize libcurl");
        return false;
    }
    g_isInitialized++;
    return true;
}

void AlexaClientSDKInit::uninitialize() {
    if (0 == g_isInitialized) {
        Logger::log("AlexaClientSDKInit::terminate called without corresponding AlexaClientSDKInit::initialize");
        return;
    }
    g_isInitialized--;
    curl_global_cleanup();
}

} // namespace initialization
} // namespace avsUtils
} // namespace alexaClientSDK
