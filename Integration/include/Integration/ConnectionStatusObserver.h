/*
 * ConnectionStatusObserver.h
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

#ifndef ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_CONNECTION_STATUS_OBSERVER_H_
#define ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_CONNECTION_STATUS_OBSERVER_H_

#include <chrono>
#include <condition_variable>
#include <mutex>

#include "ACL/ConnectionStatusObserverInterface.h"

namespace alexaClientSDK {
namespace integration {

using namespace alexaClientSDK::acl;

class ConnectionStatusObserver : public ConnectionStatusObserverInterface {
public:
    ConnectionStatusObserver();
    void onConnectionStatusChanged(const ConnectionStatus connectionStatus,
                                   const ConnectionChangedReason reason) override;
    ConnectionStatus getConnectionStatus() const;
    bool waitFor(const ConnectionStatus connectionStatus,
                 const std::chrono::seconds duration = std::chrono::seconds(10));
private:
    ConnectionStatus m_connectionStatus;
    std::mutex m_mutex;
    std::condition_variable m_wakeTrigger;
};

} // namespace integration
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_CONNECTION_STATUS_OBSERVER_H_
