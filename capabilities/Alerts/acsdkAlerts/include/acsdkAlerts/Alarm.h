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

#ifndef ACSDKALERTS_ALARM_H_
#define ACSDKALERTS_ALARM_H_

#include <Settings/DeviceSettingsManager.h>

#include "acsdkAlerts/Alert.h"

namespace alexaClientSDK {
namespace acsdkAlerts {

/**
 * An Alarm class.  This represents an alert which the user wishes to activate at a specific point in time,
 * which they specify as that absolute time point, rather than an offset from the current time.
 *
 * There is no expected special behavior for this type of alert - it will render a simple audio asset on the device
 * when activated.
 */
class Alarm : public Alert {
public:
    Alarm(
        std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()>
            defaultAudioFactory,
        std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()> shortAudioFactory,
        std::shared_ptr<settings::DeviceSettingsManager> settingsManager);

    std::string getTypeName() const override;

    /// Static member function to get the string representation of this type.
    static std::string getTypeNameStatic() {
        return "ALARM";
    }
};

}  // namespace acsdkAlerts
}  // namespace alexaClientSDK

#endif  // ACSDKALERTS_ALARM_H_
