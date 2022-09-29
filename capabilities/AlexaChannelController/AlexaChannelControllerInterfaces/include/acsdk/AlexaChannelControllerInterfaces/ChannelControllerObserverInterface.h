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

#ifndef ACSDK_ALEXACHANNELCONTROLLERINTERFACES_CHANNELCONTROLLEROBSERVERINTERFACE_H_
#define ACSDK_ALEXACHANNELCONTROLLERINTERFACES_CHANNELCONTROLLEROBSERVERINTERFACE_H_

#include "ChannelType.h"

namespace alexaClientSDK {
namespace alexaChannelControllerInterfaces {
/**
 * This interface is used to observe changes to the channel value properties of an endpoint
 * that are caused by the @c ChannelControllerInterface.
 */
class ChannelControllerObserverInterface {
public:
    /**
     * Destructor.
     */
    virtual ~ChannelControllerObserverInterface() = default;

    /**
     * Notifies the change in the channel value properties of the endpoint.
     *
     * @param channel The channel related values such as number, callSign and affiliateCallSign, uri, as well as
     * metadata for the channel like name and image.
     */
    virtual void onChannelChanged(std::unique_ptr<alexaChannelControllerTypes::Channel> channel) = 0;
};

}  // namespace alexaChannelControllerInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_ALEXACHANNELCONTROLLERINTERFACES_CHANNELCONTROLLEROBSERVERINTERFACE_H_
