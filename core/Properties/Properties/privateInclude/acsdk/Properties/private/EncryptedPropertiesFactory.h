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

#ifndef ACSDK_PROPERTIES_PRIVATE_ENCRYPTEDPROPERTIESFACTORY_H_
#define ACSDK_PROPERTIES_PRIVATE_ENCRYPTEDPROPERTIESFACTORY_H_

#include <acsdk/PropertiesInterfaces/PropertiesFactoryInterface.h>
#include <acsdk/CryptoInterfaces/CryptoFactoryInterface.h>
#include <acsdk/CryptoInterfaces/KeyFactoryInterface.h>
#include <acsdk/CryptoInterfaces/KeyStoreInterface.h>
#include <acsdk/Properties/MiscStorageAdapter.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>

namespace alexaClientSDK {
namespace properties {

/**
 * @brief Properties factory wrapper to encrypt all properties.
 *
 * This factory works with @name EncryptedProperties class to ensure all property values are stored in encrypted form
 * in the underlying storage.
 *
 * @ingroup Lib_acsdkProperties
 */
class EncryptedPropertiesFactory : public alexaClientSDK::propertiesInterfaces::PropertiesFactoryInterface {
public:
    /**
     * @brief Creates properties factory using given dependencies.
     *
     * @param[in] innerFactory Internal factory for accessing properties in plain text manner.
     * @param[in] cryptoFactory Encryption facilities factory.
     * @param[in] keyStore HSM key store.
     *
     * @return Reference to factory or nullptr on error.
     */
    static std::shared_ptr<PropertiesFactoryInterface> create(
        const std::shared_ptr<alexaClientSDK::propertiesInterfaces::PropertiesFactoryInterface>& innerFactory,
        const std::shared_ptr<alexaClientSDK::cryptoInterfaces::CryptoFactoryInterface>& cryptoFactory,
        const std::shared_ptr<alexaClientSDK::cryptoInterfaces::KeyStoreInterface>& keyStore) noexcept;

    /// @name PropertiesFactoryInterface methods
    ///@{
    std::shared_ptr<alexaClientSDK::propertiesInterfaces::PropertiesInterface> getProperties(
        const std::string& configUri) noexcept override;
    ///@}

private:
    /**
     * @brief Constructs factory.
     *
     * @param[in] innerFactory Internal factory for accessing properties in plain text manner.
     * @param[in] cryptoFactory Encryption facilities factory.
     * @param[in] keyStore HSM key store.
     */
    EncryptedPropertiesFactory(
        const std::shared_ptr<PropertiesFactoryInterface>& innerFactory,
        const std::shared_ptr<alexaClientSDK::cryptoInterfaces::CryptoFactoryInterface>& cryptoFactory,
        const std::shared_ptr<alexaClientSDK::cryptoInterfaces::KeyStoreInterface>& keyStore) noexcept;

    bool init() noexcept;

    /// Nested unencrypted properties factory.
    const std::shared_ptr<alexaClientSDK::propertiesInterfaces::PropertiesFactoryInterface> m_storage;
    /// Cryptography service factory.
    const std::shared_ptr<alexaClientSDK::cryptoInterfaces::CryptoFactoryInterface> m_cryptoFactory;
    /// HSM keystore interface.
    const std::shared_ptr<alexaClientSDK::cryptoInterfaces::KeyStoreInterface> m_keyStore;
};

}  // namespace properties
}  // namespace alexaClientSDK

#endif  // ACSDK_PROPERTIES_PRIVATE_ENCRYPTEDPROPERTIESFACTORY_H_
