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

#ifndef ACSDKAUTHORIZATIONDELEGATE_AUTHORIZATIONDELEGATECOMPONENT_H_
#define ACSDKAUTHORIZATIONDELEGATE_AUTHORIZATIONDELEGATECOMPONENT_H_

#include <memory>

#include <acsdkCryptoInterfaces/CryptoFactoryInterface.h>
#include <acsdkCryptoInterfaces/KeyStoreInterface.h>
#include <acsdkManufactory/Component.h>
#include <acsdkManufactory/Import.h>
#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpPostInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <CBLAuthDelegate/CBLAuthRequesterInterface.h>
#include <RegistrationManager/CustomerDataManagerInterface.h>

namespace alexaClientSDK {
namespace acsdkAuthorizationDelegate {

/**
 * Manufactory Component definition for the CBL implementation of AuthDelegateInterface.
 */
using AuthorizationDelegateComponent = acsdkManufactory::Component<
    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>,
    acsdkManufactory::Import<std::shared_ptr<authorization::cblAuthDelegate::CBLAuthRequesterInterface>>,
    acsdkManufactory::Import<std::shared_ptr<alexaClientSDK::avsCommon::utils::configuration::ConfigurationNode>>,
    acsdkManufactory::Import<std::unique_ptr<avsCommon::utils::libcurlUtils::HttpPostInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::DeviceInfo>>,
    acsdkManufactory::Import<std::shared_ptr<registrationManager::CustomerDataManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<acsdkCryptoInterfaces::CryptoFactoryInterface>>,
    acsdkManufactory::Import<std::shared_ptr<acsdkCryptoInterfaces::KeyStoreInterface>>>;

/**
 * Get the @c Manufactory component for creating an instance of AVSConnectionMangerInterface.
 *
 * @return The @c Manufactory component for creating an instance of AuthDelegateInterface.
 */
AuthorizationDelegateComponent getComponent();

}  // namespace acsdkAuthorizationDelegate
}  // namespace alexaClientSDK

#endif  // ACSDKAUTHORIZATIONDELEGATE_AUTHORIZATIONDELEGATECOMPONENT_H_
