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

#include <acsdk/Properties/MiscStorageAdapter.h>
#include <acsdk/Properties/private/EncryptedPropertiesFactory.h>
#include <acsdk/Properties/private/Logging.h>

namespace alexaClientSDK {
namespace properties {

using namespace ::alexaClientSDK::avsCommon::sdkInterfaces::storage;
using namespace ::alexaClientSDK::cryptoInterfaces;

/// String to identify log entries originating from this file.
/// @private
#define TAG "EncryptedPropertiesFactoryApi"

std::shared_ptr<PropertiesFactoryInterface> createEncryptedPropertiesFactory(
    const std::shared_ptr<PropertiesFactoryInterface>& innerPropertiesFactory,
    const std::shared_ptr<CryptoFactoryInterface>& cryptoFactory,
    const std::shared_ptr<KeyStoreInterface>& keyStore) noexcept {
    auto res = EncryptedPropertiesFactory::create(innerPropertiesFactory, cryptoFactory, keyStore);
    if (!res) {
        ACSDK_ERROR(LX("createEncryptedPropertiesFactoryFailed"));
    }
    return res;
}

std::shared_ptr<alexaClientSDK::propertiesInterfaces::PropertiesFactoryInterface> createEncryptedPropertiesFactory(
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::storage::MiscStorageInterface>& innerStorage,
    const std::shared_ptr<MiscStorageUriMapperInterface>& uriMapper,
    const std::shared_ptr<alexaClientSDK::cryptoInterfaces::CryptoFactoryInterface>& cryptoFactory,
    const std::shared_ptr<alexaClientSDK::cryptoInterfaces::KeyStoreInterface>& keyStore) noexcept {
    auto adapter = createPropertiesFactory(innerStorage, uriMapper);
    if (!adapter) {
        ACSDK_ERROR(LX("createEncryptedPropertiesFactoryFailed").d("reason", "miscStorageAdapterCreateFailed"));
        return nullptr;
    }
    return createEncryptedPropertiesFactory(adapter, cryptoFactory, keyStore);
}

}  // namespace properties
}  // namespace alexaClientSDK
