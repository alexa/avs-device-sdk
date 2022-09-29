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

#ifndef ALEXA_CLIENT_SDK_COMMUNICATION_INCLUDE_COMMUNICATION_MESSAGINGSERVEROBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_COMMUNICATION_INCLUDE_COMMUNICATION_MESSAGINGSERVEROBSERVERINTERFACE_H_

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace communication {

/**
 * Observe @c MessagingServer events.
 */
class MessagingServerObserverInterface {
public:
    /**
     * A new connection to the server has been opened.
     */
    virtual void onConnectionOpened() = 0;

    /**
     * A connection to the server has been closed.
     */
    virtual void onConnectionClosed() = 0;
};

}  // namespace communication
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_COMMUNICATION_INCLUDE_COMMUNICATION_MESSAGINGSERVEROBSERVERINTERFACE_H_
