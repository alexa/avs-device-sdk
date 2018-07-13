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

#ifndef ALEXA_CLIENT_SDK_AFML_INCLUDE_AFML_ACTIVITYTRACKERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AFML_INCLUDE_AFML_ACTIVITYTRACKERINTERFACE_H_

#include <vector>

#include "AFML/Channel.h"

namespace alexaClientSDK {
namespace afml {

/**
 * This @c ActivityTrackerInterface interface is used by the @c FocusManager to notify its activity tracker
 * any focus updates to a vector of @c Channel::State due to @c acquireChannel(), @c releaseChannel() or
 * stopForegroundActivity().
 */
class ActivityTrackerInterface {
public:
    /// Destructor.
    virtual ~ActivityTrackerInterface() = default;

    /**
     * This function is called whenever an activity in @c FocusManager causes updates to a vector of channels.
     *
     * @param channelStates A vector of @c Channel::State that has been updated.
     */
    virtual void notifyOfActivityUpdates(const std::vector<Channel::State>& channelStates) = 0;
};

}  // namespace afml
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AFML_INCLUDE_AFML_ACTIVITYTRACKERINTERFACE_H_
