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

#include <acsdk/Pkcs11/KeyStoreFactory.h>
#include <acsdk/Pkcs11/private/PKCS11KeyStore.h>

namespace alexaClientSDK {
namespace pkcs11 {

using alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface;

std::shared_ptr<alexaClientSDK::cryptoInterfaces::KeyStoreInterface> createKeyStore(
    const std::shared_ptr<MetricRecorderInterface>& metricRecorder) noexcept {
    return PKCS11KeyStore::create(metricRecorder);
}

}  // namespace pkcs11
}  // namespace alexaClientSDK
