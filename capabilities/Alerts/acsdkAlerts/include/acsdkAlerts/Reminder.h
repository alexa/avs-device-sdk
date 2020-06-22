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

#ifndef ACSDKALERTS_REMINDER_H_
#define ACSDKALERTS_REMINDER_H_

#include "acsdkAlerts/Alert.h"

namespace alexaClientSDK {
namespace acsdkAlerts {

/**
 * A reminder class.  This represents an alert which the user wishes to activate at a specific point in time,
 * which they specify as that absolute time point, rather than an offset from the current time.
 *
 * The user expectation is that the activation of the reminder will include custom assets (if the device is connected
 * to the internet), such as Alexa telling the user something specific with respect to the reminder being set.
 *
 * Usage example:
 * "Alexa, remind me to walk the dog at 10am".
 * 10am : Alexa will say "This is your 10am reminder to walk the dog."
 */
class Reminder : public Alert {
public:
    Reminder(
        std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()>
            defaultAudioFactory,
        std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()> shortAudioFactory,
        std::shared_ptr<settings::DeviceSettingsManager> settingsManager);

    std::string getTypeName() const override;

    /// Static member function to get the string representation of this type.
    static std::string getTypeNameStatic() {
        return "REMINDER";
    }
};

}  // namespace acsdkAlerts
}  // namespace alexaClientSDK

#endif  // ACSDKALERTS_REMINDER_H_
