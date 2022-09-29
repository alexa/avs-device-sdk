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

#ifndef ACSDK_PKCS11_PRIVATE_PKCS11CONFIG_H_
#define ACSDK_PKCS11_PRIVATE_PKCS11CONFIG_H_

#include <memory>
#include <string>

namespace alexaClientSDK {
namespace pkcs11 {

class PKCS11Key;
class PKCS11Slot;
class PKCS11Session;

/**
 * @brief PKCS11 Platform Configuration.
 *
 * This class provides access to PKCS11 configuration. Te configuration file includes path to PKCS#11 module, token
 * name, PIN, and default key alias to use with encryption function.
 *
 * The configuration file shall have restricted access to service account, that executes application.
 *
 * @sa Lib_acsdkPkcs11
 */
class PKCS11Config {
public:
    /**
     * @brief Creates object.
     *
     * @return Object reference or nullptr on error.
     */
    static std::shared_ptr<PKCS11Config> create() noexcept;

    /**
     * @brief Returns file path to PKCS11 library.
     *
     * @return Path to PKCS11 shared library object.
     */
    std::string getLibraryPath() const noexcept;

    /**
     * @brief Returns PIN for authentication.
     *
     * @return User PIN.
     */
    std::string getUserPin() const noexcept;

    /**
     * @brief Returns token name.
     *
     * @return Token name to use for key store operations.
     */
    std::string getTokenName() const noexcept;

    /**
     * @brief Returns main encryption key alias.
     *
     * @return Object name for the main encryption key to use.
     */
    std::string getDefaultKeyName() const noexcept;

private:
    /// @brief Private constructor
    PKCS11Config() noexcept;

    /**
     * @brief Helper method to load configuration from platform settings.
     *
     * @return True if operation is successful.
     */
    bool loadFromSettings() noexcept;

    /// Path to PKCS11 shared library object.
    std::string m_libraryPath;
    /// User PIN to use for token authentication
    std::string m_userPin;
    /// Token name
    std::string m_tokenName;
    /// Default main key name
    std::string m_defaultKeyName;
};

}  // namespace pkcs11
}  // namespace alexaClientSDK

#endif  // ACSDK_PKCS11_PRIVATE_PKCS11CONFIG_H_
