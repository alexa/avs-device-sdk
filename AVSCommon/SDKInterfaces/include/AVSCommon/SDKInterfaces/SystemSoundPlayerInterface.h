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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SYSTEMSOUNDPLAYERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SYSTEMSOUNDPLAYERINTERFACE_H_

#include <future>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This class allows the playback of various system sounds.
 */
class SystemSoundPlayerInterface {
public:
    /**
     * Destructor
     */
    virtual ~SystemSoundPlayerInterface() = default;

    /// The different system sounds supported
    enum class Tone {
        /// The sound to notify wake word has been detected
        WAKEWORD_NOTIFICATION,
        /// The sound to notify when Alexa is done recording speech
        END_SPEECH
    };

    /**
     * Method to play a system sound.
     *
     * @param tone The tone to play.
     *
     * @return A future that will return true on successful playback and false on error.
     */
    virtual std::shared_future<bool> playTone(Tone tone) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SYSTEMSOUNDPLAYERINTERFACE_H_
