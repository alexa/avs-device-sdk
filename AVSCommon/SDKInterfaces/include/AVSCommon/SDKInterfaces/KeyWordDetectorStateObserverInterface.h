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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_KEYWORDDETECTORSTATEOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_KEYWORDDETECTORSTATEOBSERVERINTERFACE_H_

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * A KeyWordDetectorStateObserverInterface is an interface class that clients can extend to be notified of the state
 * of the KeyWordDetector.
 */
class KeyWordDetectorStateObserverInterface {
public:
    /**
     * An enum class used to specify the states that the KeyWordDetector can be in.
     */
    enum class KeyWordDetectorState {
        /// Represents a healthy functioning KeyWordDetector.
        ACTIVE,

        /// Represents when the stream that a KeyWordDetector is reading from has been closed.
        STREAM_CLOSED,

        /// Represents an unknown error.
        ERROR
    };
    /**
     * Destructor.
     */
    virtual ~KeyWordDetectorStateObserverInterface() = default;

    /**
     * Used to notify the observer of the KeyWordDetector of state changes and errors when reading from its stream in
     * case the user needs to initialize a new stream.
     */
    virtual void onStateChanged(KeyWordDetectorState keyWordDetectorState) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_KEYWORDDETECTORSTATEOBSERVERINTERFACE_H_
