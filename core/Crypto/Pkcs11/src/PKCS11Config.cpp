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

#include <acsdk/Pkcs11/private/Logging.h>
#include <acsdk/Pkcs11/private/PKCS11Config.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>

namespace alexaClientSDK {
namespace pkcs11 {

using namespace alexaClientSDK::avsCommon::utils::configuration;

/// String to identify log entries originating from this file.
/// @private
#define TAG "pkcs11::Config"

/// @private
static const std::string PKCS11_MODULE_CONFIG_KEY{"pkcs11Module"};

std::shared_ptr<PKCS11Config> PKCS11Config::create() noexcept {
    std::shared_ptr<PKCS11Config> config = std::shared_ptr<PKCS11Config>(new PKCS11Config);

    if (!config->loadFromSettings()) {
        ACSDK_ERROR(LX("configLoadingFailed"));
        config.reset();
    }

    return config;
}

PKCS11Config::PKCS11Config() noexcept {
}

std::string PKCS11Config::getLibraryPath() const noexcept {
    return m_libraryPath;
}

std::string PKCS11Config::getUserPin() const noexcept {
    return m_userPin;
}

std::string PKCS11Config::getTokenName() const noexcept {
    return m_tokenName;
}

std::string PKCS11Config::getDefaultKeyName() const noexcept {
    return m_defaultKeyName;
}

bool PKCS11Config::loadFromSettings() noexcept {
    auto configurationRoot = ConfigurationNode::getRoot()[PKCS11_MODULE_CONFIG_KEY];

    if (!configurationRoot.getString("libraryPath", &m_libraryPath)) {
        ACSDK_ERROR(LX("libraryPathMissing"));
        return false;
    }

    if (!configurationRoot.getString("userPin", &m_userPin)) {
        ACSDK_ERROR(LX("userPinMissing"));
        return false;
    }

    if (!configurationRoot.getString("tokenName", &m_tokenName)) {
        ACSDK_ERROR(LX("tokenNameMissing"));
        return false;
    }

    if (!configurationRoot.getString("defaultKeyName", &m_defaultKeyName)) {
        ACSDK_ERROR(LX("defaultKeyNameMissing"));
        return false;
    }

    return true;
}

}  // namespace pkcs11
}  // namespace alexaClientSDK
