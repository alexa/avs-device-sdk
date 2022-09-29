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

#include <acsdk/Properties/private/EncryptedPropertiesFactory.h>
#include <acsdk/Properties/private/EncryptedProperties.h>
#include <acsdk/Properties/private/Logging.h>

namespace alexaClientSDK {
namespace properties {

using namespace ::alexaClientSDK::avsCommon::sdkInterfaces::storage;
using namespace ::alexaClientSDK::propertiesInterfaces;
using namespace ::alexaClientSDK::cryptoInterfaces;

/// String to identify log entries originating from this file.
/// @private
#define TAG "EncryptedPropertiesFactory"

std::shared_ptr<PropertiesFactoryInterface> EncryptedPropertiesFactory::create(
    const std::shared_ptr<PropertiesFactoryInterface>& innerFactory,
    const std::shared_ptr<CryptoFactoryInterface>& cryptoFactory,
    const std::shared_ptr<KeyStoreInterface>& keyStore) noexcept {
    auto res = std::shared_ptr<EncryptedPropertiesFactory>(
        new EncryptedPropertiesFactory(innerFactory, cryptoFactory, keyStore));
    if (!res->init()) {
        res.reset();
    }
    return res;
}

EncryptedPropertiesFactory::EncryptedPropertiesFactory(
    const std::shared_ptr<PropertiesFactoryInterface>& storage,
    const std::shared_ptr<CryptoFactoryInterface>& cryptoFactory,
    const std::shared_ptr<KeyStoreInterface>& keyStore) noexcept :
        m_storage(storage),
        m_cryptoFactory(cryptoFactory),
        m_keyStore(keyStore) {
}

bool EncryptedPropertiesFactory::init() noexcept {
    if (!m_storage) {
        ACSDK_ERROR(LX("innerStorageNull"));
        return false;
    } else if (!m_cryptoFactory) {
        ACSDK_ERROR(LX("cryptoFactoryNull"));
        return false;
    } else if (!m_keyStore) {
        ACSDK_ERROR(LX("keyStoreNull"));
        return false;
    } else {
        return true;
    }
}

std::shared_ptr<PropertiesInterface> EncryptedPropertiesFactory::getProperties(const std::string& configUri) noexcept {
    return EncryptedProperties::create(configUri, m_storage->getProperties(configUri), m_cryptoFactory, m_keyStore);
}

}  // namespace properties
}  // namespace alexaClientSDK
