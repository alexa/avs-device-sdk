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
#ifndef ACSDK_APLCAPABILITYCOMMONINTERFACES_VISUALSTATEPROVIDERINTERFACE_H_
#define ACSDK_APLCAPABILITYCOMMONINTERFACES_VISUALSTATEPROVIDERINTERFACE_H_

#include <AVSCommon/SDKInterfaces/ContextRequestToken.h>

#include "PresentationToken.h"

namespace alexaClientSDK {
namespace aplCapabilityCommonInterfaces {

/**
 * A @c VisualStateProviderInterface may be any client component that provides
 * updates on the visual context of the device.
 */
class VisualStateProviderInterface {
public:
    /**
     * Destructor.
     */
    virtual ~VisualStateProviderInterface() = default;

    /**
     * A request to a @c VisualStateProvider to provide the state.
     * The requester specifies a token which the @c VisualStateProvider must use when it updates
     * the state via the @c onVisualContextAvailable call to @c APL Capability Agent.
     *
     * @param token The token of the last displaying document to get the correct context in the @c
     * onVisualContextAvailable call.
     * @param stateRequestToken The token to use in the @c onVisualContextAvailable call.
     */
    virtual void provideState(
        const PresentationToken& token,
        const avsCommon::sdkInterfaces::ContextRequestToken stateRequestToken) = 0;
};

}  // namespace aplCapabilityCommonInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_APLCAPABILITYCOMMONINTERFACES_VISUALSTATEPROVIDERINTERFACE_H_