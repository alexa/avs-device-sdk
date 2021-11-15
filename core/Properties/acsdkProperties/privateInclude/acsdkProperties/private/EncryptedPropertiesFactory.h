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

#ifndef ACSDKPROPERTIES_PRIVATE_ENCRYPTEDPROPERTIESFACTORY_H_
#define ACSDKPROPERTIES_PRIVATE_ENCRYPTEDPROPERTIESFACTORY_H_

#include <acsdkPropertiesInterfaces/PropertiesFactoryInterface.h>
#include <acsdkCryptoInterfaces/CryptoFactoryInterface.h>
#include <acsdkCryptoInterfaces/KeyFactoryInterface.h>
#include <acsdkCryptoInterfaces/KeyStoreInterface.h>
#include <acsdkProperties/MiscStorageAdapter.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>

namespace alexaClientSDK {
namespace acsdkProperties {

/**
 * @brief Properties factory wrapper to encrypt all properties.
 *
 * This factory works with @name EncryptedProperties class to ensure all property values are stored in encrypted form
 * in the underlying storage.
 *
 * @ingroup PropertiesIMPL
 */
class EncryptedPropertiesFactory : public alexaClientSDK::acsdkPropertiesInterfaces::PropertiesFactoryInterface {
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
        const std::shared_ptr<alexaClientSDK::acsdkPropertiesInterfaces::PropertiesFactoryInterface>& innerFactory,
        const std::shared_ptr<alexaClientSDK::acsdkCryptoInterfaces::CryptoFactoryInterface>& cryptoFactory,
        const std::shared_ptr<alexaClientSDK::acsdkCryptoInterfaces::KeyStoreInterface>& keyStore) noexcept;

    /// @name PropertiesFactoryInterface methods
    ///@{
    std::shared_ptr<alexaClientSDK::acsdkPropertiesInterfaces::PropertiesInterface> getProperties(
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
        const std::shared_ptr<alexaClientSDK::acsdkCryptoInterfaces::CryptoFactoryInterface>& cryptoFactory,
        const std::shared_ptr<alexaClientSDK::acsdkCryptoInterfaces::KeyStoreInterface>& keyStore) noexcept;

    bool init() noexcept;

    /// Nested unencrypted properties factory.
    const std::shared_ptr<alexaClientSDK::acsdkPropertiesInterfaces::PropertiesFactoryInterface> m_storage;
    /// Cryptography service factory.
    const std::shared_ptr<alexaClientSDK::acsdkCryptoInterfaces::CryptoFactoryInterface> m_cryptoFactory;
    /// HSM keystore interface.
    const std::shared_ptr<alexaClientSDK::acsdkCryptoInterfaces::KeyStoreInterface> m_keyStore;
};

}  // namespace acsdkProperties
}  // namespace alexaClientSDK

#endif  // ACSDKPROPERTIES_PRIVATE_ENCRYPTEDPROPERTIESFACTORY_H_
