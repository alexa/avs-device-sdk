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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SPEAKERMANAGEROBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SPEAKERMANAGEROBSERVERINTERFACE_H_

#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This interface is for observing changes to speakers that are made by the @c SpeakerManager.
 *
 * Observers of the SpeakerManager are notified using the SpeakerManagers internal thread. The callback function must
 * exit as quickly as possible and perform minimal calculations. Not doing so can cause delays in the @c SpeakerManager.
 * No other SpeakerManager methods which utilize that thread must be called from this callback.
 */
class SpeakerManagerObserverInterface {
public:
    /// Indicates whether the source of the call is from an AVS Directive or through a Local API call.
    enum class Source {
        // The call occured as a result of an AVS Directive.
        DIRECTIVE,
        // The call occured as a result of a local API call.
        LOCAL_API
    };

    /**
     * A callback for when the @c SpeakerInterface::SpeakerSettings succesfully changes.
     *
     * @param source. This indicates the origin of the call.
     * @param type. This indicates the type of speaker that was modified.
     * @param settings. This indicates the current settings after the change.
     */
    virtual void onSpeakerSettingsChanged(
        const Source& source,
        const SpeakerInterface::Type& type,
        const SpeakerInterface::SpeakerSettings& settings) = 0;

    /**
     * Destructor.
     */
    virtual ~SpeakerManagerObserverInterface() = default;
};

/**
 * Write a @c Source value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param type The source value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, SpeakerManagerObserverInterface::Source source) {
    switch (source) {
        case SpeakerManagerObserverInterface::Source::DIRECTIVE:
            stream << "DIRECTIVE";
            return stream;
        case SpeakerManagerObserverInterface::Source::LOCAL_API:
            stream << "LOCAL_API";
            return stream;
    }
    stream << "UNKNOWN";
    return stream;
}

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_SPEAKERMANAGEROBSERVERINTERFACE_H_
