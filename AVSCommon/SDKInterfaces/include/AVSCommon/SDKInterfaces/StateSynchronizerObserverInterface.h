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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_STATESYNCHRONIZEROBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_STATESYNCHRONIZEROBSERVERINTERFACE_H_

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This interface provides a callback that signals state has been synchronized successfully. Since @c SynchronizeState
 * event should be the first message sent to AVS upon connection, if a component is sending a message, then it needs to
 * know the state of @c StateSynchronizer in order to start sending, and therefore contain an implementation of this
 * interface. Moreover, said component or implementation should add themselves to @c StateSynchronizer to receive the
 * callback.
 */
class StateSynchronizerObserverInterface {
public:
    /**
     * This enum provides the state of the @c StateSynchronizer.
     */
    enum class State {
        /// The state in which @c StateSynchronizer has not send @c SynchronizeState event.
        NOT_SYNCHRONIZED,
        /// The state in which the state has been synchronized.
        SYNCHRONIZED
    };

    /**
     * Destructor.
     */
    virtual ~StateSynchronizerObserverInterface() = default;

    /**
     * Get the notification that the state has been synchronized.
     *
     * @param newState The state to which the @c StateSynchronizer has transitioned.
     * @note The implementation of this function should return fast in order not to block the component that calls it.
     */
    virtual void onStateChanged(State newState) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_STATESYNCHRONIZEROBSERVERINTERFACE_H_
