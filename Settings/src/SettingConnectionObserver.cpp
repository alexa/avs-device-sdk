
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

#include "Settings/SettingConnectionObserver.h"

namespace alexaClientSDK {
namespace settings {

using namespace avsCommon::sdkInterfaces;

std::shared_ptr<SettingConnectionObserver> SettingConnectionObserver::create(ConnectionStatusCallback notifyCallback) {
    return std::shared_ptr<SettingConnectionObserver>(new SettingConnectionObserver(notifyCallback));
}

SettingConnectionObserver::SettingConnectionObserver(ConnectionStatusCallback notifyCallback) :
        m_connectionStatusCallback{notifyCallback} {
}

void SettingConnectionObserver::onConnectionStatusChanged(
    ConnectionStatusObserverInterface::Status status,
    ConnectionStatusObserverInterface::ChangedReason reason) {
    switch (status) {
        case ConnectionStatusObserverInterface::Status::CONNECTED:
            m_connectionStatusCallback(true);
            break;
        case ConnectionStatusObserverInterface::Status::DISCONNECTED:
        case ConnectionStatusObserverInterface::Status::PENDING:
            m_connectionStatusCallback(false);
            break;
    }
}

}  // namespace settings
}  // namespace alexaClientSDK
