/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "AVSCommon/Utils/Common/TestableMessageObserver.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

using namespace avsCommon::sdkInterfaces;

/// String to identify log entries originating from this file.
static const std::string TAG("TestableMessageObserver");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

void TestableMessageObserver::receive(const std::string& contextId, const std::string& message) {
    ACSDK_INFO(LX("receive").d("message", message));
    std::lock_guard<std::mutex> lock(m_mutex);
    m_receivedDirectives.push_back(message);
    m_cv.notify_all();
}

bool TestableMessageObserver::waitForDirective(
    const std::string& directiveMessage,
    const std::chrono::seconds duration) {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_cv.wait_for(lock, duration, [this, directiveMessage]() {
        for (auto directive : m_receivedDirectives) {
            if (directive == directiveMessage) {
                return true;
            }
        }
        ACSDK_WARN(LX("waitForDirectiveFailed").d("reason", "directiveNotReceived").d("expected", directiveMessage));
        return false;
    });
}

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
