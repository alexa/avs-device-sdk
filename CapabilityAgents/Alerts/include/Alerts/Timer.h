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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_TIMER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_TIMER_H_

#include "Alerts/Alert.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {

/**
 * A Timer class.  This represents an alert which the user wishes to activate at a point in time relative to the
 * current time.  This is different from requesting an alert at an absolute point in time.
 *
 * Timers may be basic, or named.  If named, they will use custom assets at the point of activation.
 *
 * Example of basic timer use:
 * "Alexa, set a timer for 10 seconds."
 * 10 seconds later : device will render a simple audio file, local to the device, to alert the user.
 *
 * Example of named timer use:
 * "Alexa, set an egg timer for 10 seconds"
 * 10 seconds later : Alexa will say something like "Your egg timer is complete".
 */
class Timer : public Alert {
public:
    /// String representation of this type.
    static const std::string TYPE_NAME;

    Timer(
        std::function<std::unique_ptr<std::istream>()> defaultAudioFactory,
        std::function<std::unique_ptr<std::istream>()> shortAudioFactory);

    std::string getTypeName() const override;
};

}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_TIMER_H_
