/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <curl/curl.h>

#include "AVSCommon/AVS/Initialization/AlexaClientSDKInit.h"
#include "AVSCommon/Utils/Configuration/ConfigurationNode.h"
#include "AVSCommon/Utils/Logger/Logger.h"
#include "AVSCommon/Utils/SDKVersion.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace initialization {

/// String to identify log entries originating from this file.
static const std::string TAG("AlexaClientSdkInit");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Tracks whether we've initialized the Alexa Client SDK or not
std::atomic_int AlexaClientSDKInit::g_isInitialized{0};

std::function<std::shared_ptr<AlexaClientSDKInit>(std::shared_ptr<utils::logger::Logger>)> AlexaClientSDKInit::
    getCreateAlexaClientSDKInit(const std::vector<std::shared_ptr<std::istream>>& jsonStreams) {
    return [jsonStreams](std::shared_ptr<utils::logger::Logger> logger) -> std::shared_ptr<AlexaClientSDKInit> {
        if (!logger) {
            ACSDK_ERROR(LX("getCreateAlexaClientSDKInitFailed").d("reason", "nullLogger"));
            return nullptr;
        }
        if (!initialize(jsonStreams)) {
            ACSDK_ERROR(LX("getCreateAlexaClientSDKInitFailed").d("reason", "initializeFailed"));
            return nullptr;
        }
        return std::shared_ptr<AlexaClientSDKInit>(new AlexaClientSDKInit);
    };
}

/**
 * Destructor.
 */
AlexaClientSDKInit::~AlexaClientSDKInit() {
    uninitialize();
}

bool AlexaClientSDKInit::isInitialized() {
    return g_isInitialized > 0;
}

bool AlexaClientSDKInit::initialize(const std::vector<std::shared_ptr<std::istream>>& jsonStreams) {
    ACSDK_INFO(LX(__func__).d("sdkversion", avsCommon::utils::sdkVersion::getCurrentVersion()));

    if (!(curl_version_info(CURLVERSION_NOW)->features & CURL_VERSION_HTTP2)) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "curlDoesNotSupportHTTP2"));
        return false;
    }

    if (!utils::configuration::ConfigurationNode::initialize(jsonStreams)) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "ConfigurationNode::initializeFailed"));
        return false;
    }

    if (CURLE_OK != curl_global_init(CURL_GLOBAL_ALL)) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "curl_global_initFailed"));
        utils::configuration::ConfigurationNode::uninitialize();
        return false;
    }

    g_isInitialized++;
    return true;
}

void AlexaClientSDKInit::uninitialize() {
    if (0 == g_isInitialized) {
        ACSDK_ERROR(LX("initializeError").d("reason", "notInitialized"));
        return;
    }
    g_isInitialized--;
    curl_global_cleanup();
    utils::configuration::ConfigurationNode::uninitialize();
}

}  // namespace initialization
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
