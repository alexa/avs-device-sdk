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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AVSGATEWAYOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AVSGATEWAYOBSERVERINTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * An interface for setting AVS gateway.
 */
class AVSGatewayObserverInterface {
public:
    /**
     * Destructor.
     */
    virtual ~AVSGatewayObserverInterface() = default;

    /**
     * Observer method to be called when the AVS Gateway is changed.
     *
     * @param avsGateway The AVS Gateway the device should be connected to.
     */
    virtual void onAVSGatewayChanged(const std::string& avsGateway) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AVSGATEWAYOBSERVERINTERFACE_H_
