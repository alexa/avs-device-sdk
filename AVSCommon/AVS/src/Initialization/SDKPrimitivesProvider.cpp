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
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Timing/TimerDelegateFactory.h>

#include "AVSCommon/AVS/Initialization/SDKPrimitivesProvider.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace initialization {

/// String to identify log entries originating from this file.
#define TAG "SDKPrimitivesProvider"

std::shared_ptr<SDKPrimitivesProvider> SDKPrimitivesProvider::m_provider;
std::mutex SDKPrimitivesProvider::m_mutex;

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<SDKPrimitivesProvider> SDKPrimitivesProvider::getInstance() {
    ACSDK_DEBUG5(LX("getInstance"));

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_provider) {
        SDKPrimitivesProvider::m_provider = std::shared_ptr<SDKPrimitivesProvider>(new SDKPrimitivesProvider());
    }

    return m_provider;
}

SDKPrimitivesProvider::SDKPrimitivesProvider() :
        m_initialized{false},
        m_timerDelegateFactory{std::make_shared<utils::timing::TimerDelegateFactory>()} {
}

bool SDKPrimitivesProvider::withTimerDelegateFactory(
    std::shared_ptr<sdkInterfaces::timing::TimerDelegateFactoryInterface> timerDelegateFactory) {
    ACSDK_DEBUG5(LX("withTimerDelegateFactory"));

    if (!timerDelegateFactory) {
        ACSDK_ERROR(LX("withTimerDelegateFactoryFailed").d("reason", "nullTimerDelegateFactory"));
        return false;
    }

    if (isInitialized()) {
        ACSDK_ERROR(LX("withTimerDelegateFactoryFailed").d("reason", "alreadyInitialized"));
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_timerDelegateFactory = timerDelegateFactory;

    return true;
}

std::shared_ptr<sdkInterfaces::timing::TimerDelegateFactoryInterface> SDKPrimitivesProvider::getTimerDelegateFactory() {
    ACSDK_DEBUG5(LX("getTimerDelegateFactory"));

    if (!isInitialized()) {
        ACSDK_ERROR(LX("getTimerDelegateFactoryFailed").d("reason", "notInitialized"));
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    return m_timerDelegateFactory;
}

bool SDKPrimitivesProvider::initialize() {
    ACSDK_DEBUG5(LX("initialize"));

    if (isInitialized()) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "alreadyInitialized"));
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_initialized = true;

    return true;
}

bool SDKPrimitivesProvider::isInitialized() const {
    ACSDK_DEBUG5(LX("isInitialized"));

    std::lock_guard<std::mutex> lock(m_mutex);
    return m_initialized;
}

void SDKPrimitivesProvider::terminate() {
    ACSDK_DEBUG5(LX("terminate"));

    reset();

    std::lock_guard<std::mutex> lock(m_mutex);
    m_provider.reset();
}

void SDKPrimitivesProvider::reset() {
    ACSDK_DEBUG5(LX("reset"));

    std::lock_guard<std::mutex> lock(m_mutex);
    m_timerDelegateFactory.reset();
    m_initialized = false;
}

}  // namespace initialization
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
