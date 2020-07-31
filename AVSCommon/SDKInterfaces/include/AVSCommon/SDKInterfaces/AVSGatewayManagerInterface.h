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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AVSGATEWAYMANAGERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AVSGATEWAYMANAGERINTERFACE_H_

#include <string>
#include <memory>

#include <AVSCommon/SDKInterfaces/AVSGatewayAssignerInterface.h>
#include <AVSCommon/SDKInterfaces/AVSGatewayObserverInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * The interface class for the AVS Gateway Manager. The AVS Gateway manages the AVS Gateway the device should connect
 * to.
 */
class AVSGatewayManagerInterface {
public:
    /**
     * Destructor.
     */
    virtual ~AVSGatewayManagerInterface() = default;

    /**
     * Sets the AVS Gateway Assigner used to change the Gateway the client is connected to.
     *
     * @param avsGatewayAssigner The @c AVSGatewayAssigner used to change the AVS Gateway the client is connected to.
     * @return True if successful, else false.
     */
    virtual bool setAVSGatewayAssigner(
        std::shared_ptr<avsCommon::sdkInterfaces::AVSGatewayAssignerInterface> avsGatewayAssigner) = 0;

    /**
     * Gets the AVS Gateway URL.
     *
     * @return The string representing the AVS Gateway URL
     */
    virtual std::string getGatewayURL() const = 0;

    /**
     * Sets the AVS Gateway URL.
     *
     * @param avsGatewayURL The string representing the AVS Gateway URL.
     * @return True if successful, else false.
     */
    virtual bool setGatewayURL(const std::string& avsGatewayURL) = 0;

    /**
     * Adds an observer.
     *
     * @param observer The @c AVSGatewayObserver
     */
    virtual void addObserver(std::shared_ptr<avsCommon::sdkInterfaces::AVSGatewayObserverInterface> observer) = 0;

    /**
     * Removes an observer.
     *
     * @param observer The @c AVSGatewayObserver.
     */
    virtual void removeObserver(std::shared_ptr<avsCommon::sdkInterfaces::AVSGatewayObserverInterface> observer) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_AVSGATEWAYMANAGERINTERFACE_H_
