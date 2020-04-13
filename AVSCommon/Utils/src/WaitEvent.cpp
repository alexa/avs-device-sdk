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

#include "AVSCommon/Utils/WaitEvent.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

WaitEvent::WaitEvent() : m_wakeUpTriggered{false} {
}

void WaitEvent::wakeUp() {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_wakeUpTriggered = true;
    m_condition.notify_all();
}

bool WaitEvent::wait(const std::chrono::milliseconds& timeout) {
    std::unique_lock<std::mutex> lock{m_mutex};
    return m_condition.wait_for(lock, timeout, [this] { return m_wakeUpTriggered; });
}

void WaitEvent::reset() {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_wakeUpTriggered = false;
}

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
