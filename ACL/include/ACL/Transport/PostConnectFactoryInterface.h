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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTFACTORYINTERFACE_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTFACTORYINTERFACE_H_

#include <memory>

#include "ACL/Transport/PostConnectInterface.h"

namespace alexaClientSDK {
namespace acl {

/**
 * Interface for creating post-connect objects which should be used to perform activities after a
 * connection is established.
 */
class PostConnectFactoryInterface {
public:
    /**
     * Destructor.
     */
    virtual ~PostConnectFactoryInterface() = default;

    /**
     * Create an instance of @c PostConnectInterface.
     *
     * @return An instance of @c PostConnectInterface.
     */
    virtual std::shared_ptr<PostConnectInterface> createPostConnect() = 0;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTFACTORYINTERFACE_H_
