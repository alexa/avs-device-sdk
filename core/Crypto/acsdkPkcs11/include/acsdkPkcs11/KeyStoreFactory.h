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

#ifndef ACSDKPKCS11_KEYSTOREFACTORY_H_
#define ACSDKPKCS11_KEYSTOREFACTORY_H_

#include <memory>

#include <acsdkCryptoInterfaces/KeyStoreInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>

namespace alexaClientSDK {
namespace acsdkPkcs11 {

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
 * @see alexaClientSDK::acsdkCryptoInterfaces::KeyStoreInterface
 * @see alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface
 * @ingroup CryptoPKCS11
 */
std::shared_ptr<alexaClientSDK::acsdkCryptoInterfaces::KeyStoreInterface> createKeyStore(
    const std::shared_ptr<MetricRecorderInterface>& metricRecorder = nullptr) noexcept;

}  // namespace acsdkPkcs11
}  // namespace alexaClientSDK

#endif  // ACSDKPKCS11_KEYSTOREFACTORY_H_
