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
#include "acsdkBluetooth/BluetoothEventState.h"

namespace alexaClientSDK {
namespace acsdkBluetooth {
BluetoothEventState::BluetoothEventState(
    alexaClientSDK::avsCommon::sdkInterfaces::bluetooth::DeviceState state,
    alexaClientSDK::avsCommon::utils::Optional<alexaClientSDK::avsCommon::avs::Requester> requester,
    alexaClientSDK::avsCommon::utils::Optional<std::string> profileName) :
        m_state(state),
        m_requester(requester),
        m_profileName(profileName) {
}

avsCommon::utils::Optional<avsCommon::avs::Requester> BluetoothEventState::getRequester() const {
    return m_requester;
}

avsCommon::sdkInterfaces::bluetooth::DeviceState BluetoothEventState::getDeviceState() const {
    return m_state;
}

avsCommon::utils::Optional<std::string> BluetoothEventState::getProfileName() const {
    return m_profileName;
}

}  // namespace acsdkBluetooth
}  // namespace alexaClientSDK
