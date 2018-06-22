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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CHANNELOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CHANNELOBSERVERINTERFACE_H_

#include "AVSCommon/AVS/FocusState.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * A ChannelObserverInterface is an interface class that clients can extend to register for focus changes.
 */
class ChannelObserverInterface {
public:
    /**
     * Destructor.
     */
    virtual ~ChannelObserverInterface() = default;

    /**
     * Used to notify the observer of the Channel of focus changes. Once called, the client should make a user
     * observable change only and return immediately. Any additional work that needs to be done should be done on a
     * separate thread or after returning. "User observable change" here refers to events that the end user of the
     * product can visibly see or hear. For example, Alexa speech or music playing would be examples of user
     * observable changes. Other work, such as database storing, logging, or communicating via network should be
     * done on a different thread. Not doing so could result in delays for other clients trying to access the
     * Channel.
     *
     * @param newFocus The new Focus of the channel.
     */
    virtual void onFocusChanged(avs::FocusState newFocus) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CHANNELOBSERVERINTERFACE_H_
