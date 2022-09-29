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
#include "AVSCommon/AVS/Initialization/InitializationParametersBuilder.h"
#include "AVSCommon/AVS/Initialization/SDKPrimitivesProvider.h"
#include "AVSCommon/Utils/Configuration/ConfigurationNode.h"
#include "AVSCommon/Utils/Logger/Logger.h"
#include "AVSCommon/Utils/Power/NoOpPowerResourceManager.h"
#include "AVSCommon/Utils/Power/PowerMonitor.h"
#include "AVSCommon/Utils/SDKVersion.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace initialization {

/// String to identify log entries originating from this file.
#define TAG "AlexaClientSdkInit"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
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

std::function<std::shared_ptr<AlexaClientSDKInit>(std::shared_ptr<utils::logger::Logger>)> AlexaClientSDKInit::
    getCreateAlexaClientSDKInit(const std::shared_ptr<InitializationParameters>& initParams) {
    return [initParams](std::shared_ptr<utils::logger::Logger> logger) -> std::shared_ptr<AlexaClientSDKInit> {
        if (!logger) {
            ACSDK_ERROR(LX("getCreateAlexaClientSDKInitFailed").d("reason", "nullLogger"));
            return nullptr;
        }

        if (!initialize(initParams)) {
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
    auto builder = InitializationParametersBuilder::create();
    if (!builder) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullBuilder"));
        return false;
    }

    // Copy to preserve backwards compatibility.
    auto jsonStreamsPtr = std::make_shared<std::vector<std::shared_ptr<std::istream>>>(jsonStreams);
    builder->withJsonStreams(jsonStreamsPtr);
    auto initParams = builder->build();
    return initialize(std::move(initParams));
}

bool AlexaClientSDKInit::initialize(const std::shared_ptr<InitializationParameters>& initParams) {
    ACSDK_INFO(LX("initialize").d("sdkversion", avsCommon::utils::sdkVersion::getCurrentVersion()));

    if (!initParams) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullInitParams"));
        return false;
    }

    // Pointer to static struct, does not need to be freed.
    auto curlVersion = curl_version_info(CURLVERSION_NOW);
    if (!curlVersion) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullCurlVersionInfo"));
        return false;
    }

    ACSDK_INFO(LX("initialize").d("curlVersion", curlVersion->version));

    if (!(curlVersion->features & CURL_VERSION_HTTP2)) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "curlDoesNotSupportHTTP2"));
        return false;
    }

    if (!utils::configuration::ConfigurationNode::initialize(*initParams->jsonStreams)) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "ConfigurationNode::initializeFailed"));
        return false;
    }

    if (CURLE_OK != curl_global_init(CURL_GLOBAL_ALL)) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "curl_global_initFailed"));
        utils::configuration::ConfigurationNode::uninitialize();
        return false;
    }

    auto timerDelegateFactory = initParams->timerDelegateFactory;
    if (!timerDelegateFactory) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullTimerDelegateFactory"));
        cleanup();
        return false;
    }

#ifdef ENABLE_LPM
    auto powerResourceManager = initParams->powerResourceManager;
    if (powerResourceManager) {
        utils::power::PowerMonitor::getInstance()->activate(powerResourceManager);

        if (!timerDelegateFactory->supportsLowPowerMode()) {
            ACSDK_ERROR(LX("initializeFailed")
                            .d("reason", "unsupportedTimerDelegateFactory")
                            .d("missing", "lowPowerModeSupport"));
            cleanup();
            return false;
        }
    } else {
        // Logic in PowerMonitor has fallback for the non active case.
        ACSDK_ERROR(LX("initalizeFailed")
                        .d("reason", "nullPowerResourceManager")
                        .m("Falling back to non-activated PowerMonitor"));
    }
#endif

    auto primitivesProvider = SDKPrimitivesProvider::getInstance();
    if (!primitivesProvider) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "nullSDKPrimitivesProvider"));
        cleanup();
        return false;
    }

    primitivesProvider->withTimerDelegateFactory(timerDelegateFactory);
    if (!primitivesProvider->initialize()) {
        cleanup();
        return false;
    }

    g_isInitialized++;
    return true;
}

void AlexaClientSDKInit::cleanup() {
    curl_global_cleanup();
    utils::configuration::ConfigurationNode::uninitialize();
    utils::power::PowerMonitor::getInstance()->deactivate();
    auto primitivesProvider = SDKPrimitivesProvider::getInstance();
    if (primitivesProvider) {
        primitivesProvider->terminate();
    }
}

void AlexaClientSDKInit::uninitialize() {
    if (0 == g_isInitialized) {
        ACSDK_ERROR(LX("initializeError").d("reason", "notInitialized"));
        return;
    }
    g_isInitialized--;
    cleanup();
}

}  // namespace initialization
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
