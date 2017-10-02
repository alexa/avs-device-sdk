/*
 * Alarm.h
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_ALERTS_INCLUDE_ALERTS_ALARM_H_
#define ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_ALERTS_INCLUDE_ALERTS_ALARM_H_

#include "Alerts/Alert.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {

/**
 * An Alarm class.  This represents an alert which the user wishes to activate at a specific point in time,
 * which they specify as that absolute time point, rather than an offset from the current time.
 *
 * There is no expected special behavior for this type of alert - it will render a simple audio asset on the device
 * when activated.
 */
class Alarm : public Alert {
public:
    /// String representation of this type.
    static const std::string TYPE_NAME;

    /**
     * A static function to set the default audio file path for this type of alert.
     *
     * @note This function should only be called at initialization, before any objects have been instantiated.
     *
     * @param filePath The path to the audio file.
     */
    static void setDefaultAudioFilePath(const std::string& filePath);

    /**
     * A static function to set the short audio file path for this type of alert.
     *
     * @note This function should only be called at initialization, before any objects have been instantiated.
     *
     * @param filePath The path to the audio file.
     */
    static void setDefaultShortAudioFilePath(const std::string& filePath);

    std::string getDefaultAudioFilePath() const override;

    std::string getDefaultShortAudioFilePath() const override;

    std::string getTypeName() const override;

private:
    /// The class-level audio file path.
    static std::string m_defaultAudioFilePath;
    /// The class-level short audio file path.
    static std::string m_defaultShortAudioFilePath;
};

}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_ALERTS_INCLUDE_ALERTS_ALARM_H_