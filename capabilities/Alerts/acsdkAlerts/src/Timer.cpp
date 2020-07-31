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

#include "acsdkAlerts/Timer.h"

namespace alexaClientSDK {
namespace acsdkAlerts {

Timer::Timer(
    std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()> defaultAudioFactory,
    std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()> shortAudioFactory,
    std::shared_ptr<settings::DeviceSettingsManager> settingsManager) :
        Alert(defaultAudioFactory, shortAudioFactory, settingsManager) {
}

std::string Timer::getTypeName() const {
    return Timer::getTypeNameStatic();
}

}  // namespace acsdkAlerts
}  // namespace alexaClientSDK
