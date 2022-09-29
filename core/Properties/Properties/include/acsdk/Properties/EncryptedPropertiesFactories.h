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

#ifndef ACSDK_PROPERTIES_ENCRYPTEDPROPERTIESFACTORIES_H_
#define ACSDK_PROPERTIES_ENCRYPTEDPROPERTIESFACTORIES_H_

#include <memory>

#include <acsdk/PropertiesInterfaces/PropertiesFactoryInterface.h>
#include <acsdk/CryptoInterfaces/KeyFactoryInterface.h>
#include <acsdk/CryptoInterfaces/KeyStoreInterface.h>
#include <acsdk/CryptoInterfaces/CryptoFactoryInterface.h>
#include <acsdk/Properties/MiscStorageAdapter.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>

namespace alexaClientSDK {
namespace properties {

using alexaClientSDK::avsCommon::sdkInterfaces::storage::MiscStorageInterface;
using alexaClientSDK::cryptoInterfaces::CryptoFactoryInterface;
using alexaClientSDK::cryptoInterfaces::KeyStoreInterface;
using alexaClientSDK::propertiesInterfaces::PropertiesFactoryInterface;

/**
 * @brief Creates properties factory with encryption support by wrapping a factory without encryption support.
 *
 * Encrypted properties factory protects all values using AES-256 cipher. The data key is stored as one of the
 * underlying properties with reserved name "$acsdkEncryption$" in encrypted form. Hardware security module is used for
 * storing the main encryption key and wrapping/unwrapping data keys.
 *
 * When client code accesses @c PropertiesInterface through encrypted @c PropertiesFactoryInterface, all existing
 * data is automatically converted into encrypted form.
 *
 * @param[in] innerFactory  Properties factory without encryption support.
 * @param[in] cryptoFactory Crypto factory reference. This parameter must not be nullptr.
 * @param[in] keyStore      Key store factory reference. This parameter must not be nullptr.
 *
 * @return Properties factory reference or nullptr on error.
 */
std::shared_ptr<PropertiesFactoryInterface> createEncryptedPropertiesFactory(
    const std::shared_ptr<PropertiesFactoryInterface>& innerFactory,
    const std::shared_ptr<CryptoFactoryInterface>& cryptoFactory,
    const std::shared_ptr<KeyStoreInterface>& keyStore) noexcept;

/**
 * @brief Creates properties factory with encryption support by wrapping a @c MiscStorageInterface.
 *
 * Encrypted properties factory protects all values using AES-256 cipher. The data key is stored as one of the
 * underlying properties with reserved name "$acsdkEncryption$" in encrypted form. Hardware security module is used for
 * storing the main encryption key and wrapping/unwrapping data keys.
 *
 * When client code accesses @c PropertiesInterface through encrypted @c PropertiesFactoryInterface, all existing
 * data is automatically converted into encrypted form.
 *
 * The method automatically creates database if it is not created. When user creates \c PropertiesInterface, the
 * implementation automatically creates corresponding table.
 *
 * As all encrypted property values are in binary form, the implementation uses base64 encoding to store values.
 *
 * @param[in] innerStorage  Storage reference. This parameter must not be nullptr.
 * @param[in] uriMapper     URI mapper reference.
 * @param[in] cryptoFactory Crypto factory reference. This parameter must not be nullptr.
 * @param[in] keyStore      Key store factory reference. This parameter must not be nullptr.
 *
 * @return Properties factory reference or nullptr on error.
 *
 * @ingroup Lib_acsdkProperties
 */
std::shared_ptr<PropertiesFactoryInterface> createEncryptedPropertiesFactory(
    const std::shared_ptr<MiscStorageInterface>& innerStorage,
    const std::shared_ptr<MiscStorageUriMapperInterface>& uriMapper,
    const std::shared_ptr<CryptoFactoryInterface>& cryptoFactory,
    const std::shared_ptr<KeyStoreInterface>& keyStore) noexcept;

}  // namespace properties
}  // namespace alexaClientSDK

#endif  // ACSDK_PROPERTIES_ENCRYPTEDPROPERTIESFACTORIES_H_
