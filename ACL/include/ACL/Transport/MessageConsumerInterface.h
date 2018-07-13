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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGECONSUMERINTERFACE_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGECONSUMERINTERFACE_H_

#include <memory>

namespace alexaClientSDK {
namespace acl {

/**
 * An interface which allows a derived class to consume a Message from AVS.
 */
class MessageConsumerInterface {
public:
    /**
     * Destructor.
     */
    virtual ~MessageConsumerInterface() = default;

    /**
     * Called when a message has been received from AVS.
     *
     * @param contextId The context id for the current message.
     * @param message The AVS message in string representation.
     */
    virtual void consumeMessage(const std::string& contextId, const std::string& message) = 0;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGECONSUMERINTERFACE_H_
