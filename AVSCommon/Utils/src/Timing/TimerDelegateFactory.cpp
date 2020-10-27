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

#include <AVSCommon/Utils/Memory/Memory.h>
#include <AVSCommon/Utils/Timing/TimerDelegate.h>
#include <AVSCommon/Utils/Timing/TimerDelegateFactory.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace timing {

bool TimerDelegateFactory::supportsLowPowerMode() {
    return false;
}

std::unique_ptr<sdkInterfaces::timing::TimerDelegateInterface> TimerDelegateFactory::getTimerDelegate() {
    return memory::make_unique<TimerDelegate>();
}

}  // namespace timing
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
