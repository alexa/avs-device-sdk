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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_POSTCONNECTOPERATIONPROVIDERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_POSTCONNECTOPERATIONPROVIDERINTERFACE_H_

#include <memory>

#include <AVSCommon/SDKInterfaces/PostConnectOperationInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/*
 * Interface to be implemented to create new instances of @c PostConnectOperationInterface.
 */
class PostConnectOperationProviderInterface {
public:
    /**
     * Destructor.
     */
    virtual ~PostConnectOperationProviderInterface() = default;

    /**
     * Creates a post connect operation instance.
     *
     * @return a new instance of the @c PostConnectOperationInterface.
     */
    virtual std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationInterface> createPostConnectOperation() = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_POSTCONNECTOPERATIONPROVIDERINTERFACE_H_
