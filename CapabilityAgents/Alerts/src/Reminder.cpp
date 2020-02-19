/*
 * Copyright 2017-2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "Alerts/Reminder.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {

Reminder::Reminder(
    std::function<std::unique_ptr<std::istream>()> defaultAudioFactory,
    std::function<std::unique_ptr<std::istream>()> shortAudioFactory,
    std::shared_ptr<settings::DeviceSettingsManager> settingsManager) :
        Alert(defaultAudioFactory, shortAudioFactory, settingsManager) {
}

std::string Reminder::getTypeName() const {
    return Reminder::getTypeNameStatic();
}

}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
