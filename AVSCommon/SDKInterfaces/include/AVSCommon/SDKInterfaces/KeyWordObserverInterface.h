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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_KEYWORDOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_KEYWORDOBSERVERINTERFACE_H_

#include <limits>
#include <memory>
#include <vector>

#include "AVSCommon/AVS/AudioInputStream.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * A KeyWordObserverInterface is an interface class that clients can extend to listen for keyword detections.
 */
class KeyWordObserverInterface {
public:
    /// This represents when an index passed into the onKeyWordDetected() call should be ignored.
    static constexpr avs::AudioInputStream::Index UNSPECIFIED_INDEX =
        std::numeric_limits<avs::AudioInputStream::Index>::max();

    /**
     * Destructor.
     */
    virtual ~KeyWordObserverInterface() = default;

    /**
     * Used to notify the observer of keyword detections. Once called, the client should return as soon as possible.
     * Failure to do so might block the wake word engine from processing audio data, depending on which wake word
     * engine is used. Any additional work that needs to be done should be done on a separate thread or after
     * returning.
     *
     * @param stream The stream in which the keyword was detected.
     * @param keyword The keyword detected.
     * @param beginIndex The optional absolute begin index of the first part of the keyword found within the @c stream.
     * If this is set to UNSPECIFIED_INDEX, then it should be ignored.
     * @param endIndex The optional absolute end index of the last part of the keyword within the stream of the @c
     * stream. If this is set to UNSPECIFIED_INDEX, then it should be ignored.
     * @param KWDMetadata Wake word engine metadata.
     */
    virtual void onKeyWordDetected(
        std::shared_ptr<avs::AudioInputStream> stream,
        std::string keyword,
        avs::AudioInputStream::Index beginIndex = UNSPECIFIED_INDEX,
        avs::AudioInputStream::Index endIndex = UNSPECIFIED_INDEX,
        std::shared_ptr<const std::vector<char>> KWDMetadata = nullptr) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_KEYWORDOBSERVERINTERFACE_H_
