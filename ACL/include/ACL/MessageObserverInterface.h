/*
* MessageObserverInterface.h
*
* Copyright 2016-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_MESSAGE_OBSERVER_INTERFACE_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_MESSAGE_OBSERVER_INTERFACE_H_

#include <memory>

#include "ACL/Message.h"

namespace alexaClientSDK {
namespace acl {

/**
 * This class allows a client to receive messages from AVS.
 */
class MessageObserverInterface {
public:
    /**
     * Destructor.
     */
    virtual ~MessageObserverInterface() = default;

    /**
     * A function that a client must implement to receive Messages from AVS.
     * @param msg A message that has been received from AVS.
     */
    virtual void receive(std::shared_ptr<Message> msg) = 0;
};

} // namespace acl
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_MESSAGE_OBSERVER_INTERFACE_H_
