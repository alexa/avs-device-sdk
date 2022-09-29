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

#ifndef ACSDK_PKCS11_KEYSTOREFACTORY_H_
#define ACSDK_PKCS11_KEYSTOREFACTORY_H_

#include <memory>

#include <acsdk/CryptoInterfaces/KeyStoreInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>

namespace alexaClientSDK {
namespace pkcs11 {

using alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface;

/**
 * @brief Create instance of @c KeyStoreInterface.
 *
 * Method creates key store factory instance backed by hardware security module. This method dynamically loads
 * dependencies according to configuration.
 *
 * @param[in] metricRecorder Optional reference of @c MetricRecorderInterface for operational and error metrics.
 *
 * @return Key store reference or nullptr on error.
 *
 * @see alexaClientSDK::cryptoInterfaces::KeyStoreInterface
 * @see alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface
 * @ingroup Lib_acsdkPkcs11
 */
std::shared_ptr<alexaClientSDK::cryptoInterfaces::KeyStoreInterface> createKeyStore(
    const std::shared_ptr<MetricRecorderInterface>& metricRecorder = nullptr) noexcept;

}  // namespace pkcs11
}  // namespace alexaClientSDK

#endif  // ACSDK_PKCS11_KEYSTOREFACTORY_H_
