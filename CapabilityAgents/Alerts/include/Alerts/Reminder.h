/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_REMINDER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_REMINDER_H_

#include "Alerts/Alert.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {

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
    /// String representation of this type.
    static const std::string TYPE_NAME;

    Reminder(
        std::function<std::unique_ptr<std::istream>()> defaultAudioFactory,
        std::function<std::unique_ptr<std::istream>()> shortAudioFactory);

    std::string getTypeName() const override;
};

}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_REMINDER_H_
