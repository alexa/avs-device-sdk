/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_FOCUSMANAGEROBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_FOCUSMANAGEROBSERVERINTERFACE_H_

#include "AVSCommon/AVS/FocusState.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/// An interface that clients can extend to register to observe focus changes.
class FocusManagerObserverInterface {
public:
    virtual ~FocusManagerObserverInterface() = default;

    /**
     * Used to notify the observer of focus changes.  This function should return quickly.
     *
     * @param channelName The name of the channel which changed @c FocusState.
     * @param newFocus The new @c FocusState of @c channelName.
     */
    virtual void onFocusChanged(const std::string& channelName, avsCommon::avs::FocusState newFocus) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_FOCUSMANAGEROBSERVERINTERFACE_H_
