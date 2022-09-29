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

#ifndef ACSDK_PROPERTIES_PRIVATE_MISCSTORAGEPROPERTIES_H_
#define ACSDK_PROPERTIES_PRIVATE_MISCSTORAGEPROPERTIES_H_

#include <memory>

#include <acsdk/PropertiesInterfaces/PropertiesInterface.h>
#include <acsdk/Properties/private/RetryExecutor.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>

namespace alexaClientSDK {
namespace properties {

/**
 * @brief Properties for MiscStorageInterface.
 *
 * This class adapts @c MiscStorageInterface into @c PropertiesInterface.
 *
 * This class is thread safe and can be shared between multiple consumers.
 *
 * @see alexaClientSDK::propertiesInterfaces::PropertiesInterface
 * @see alexaClientSDK::avsCommon::sdkInterfaces::storage::MiscStorageInterface
 *
 * @ingroup Lib_acsdkProperties
 */
class MiscStorageProperties : public alexaClientSDK::propertiesInterfaces::PropertiesInterface {
public:
    /**
     * @brief Factory method.
     *
     * This method creates a new object instance to access configuration properties.
     *
     * @param storage Interface for data access.
     * @param configUri Configuration URI.
     * @param componentName Component name for data access calls.
     * @param tableName Table name for data access calls.
     *
     * @return New object reference or nullptr on error.
     */
    static std::shared_ptr<MiscStorageProperties> create(
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::storage::MiscStorageInterface>& storage,
        const std::string& configUri,
        const std::string& componentName,
        const std::string& tableName);

    /// @name PropertiesInterface methods
    /// @{
    bool getString(const std::string& key, std::string& value) noexcept override;
    bool putString(const std::string& key, const std::string& value) noexcept override;
    bool getBytes(const std::string& key, Bytes& value) noexcept override;
    bool putBytes(const std::string& key, const Bytes& value) noexcept override;
    bool remove(const std::string& key) noexcept override;
    bool getKeys(std::unordered_set<std::string>& valueContainer) noexcept override;
    bool clear() noexcept override;
    /// @}

private:
    // Constructor.
    MiscStorageProperties(
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::storage::MiscStorageInterface>& storage,
        const std::string& configUri,
        const std::string& componentName,
        const std::string& tableName);

    /**
     * @brief Initialization helper.
     *
     * This method ensures the underlying table is present.
     *
     * @return true on success, false on error.
     */
    bool init();

    // Inner properties operations
    bool loadKeysWithRetries(RetryExecutor& executor, std::unordered_set<std::string>& keys) noexcept;
    bool executeRetryableKeyAction(
        RetryExecutor& executor,
        const std::string& actionName,
        const std::string& key,
        const std::function<bool()>& action,
        bool canCleanup,
        bool failOnCleanup) noexcept;
    bool deleteValueWithRetries(RetryExecutor& executor, const std::string& key) noexcept;
    bool clearAllValuesWithRetries(RetryExecutor& executor) noexcept;

    /// Inner storage interface for data access.
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::storage::MiscStorageInterface> m_storage;

    /// Configuration URI.
    const std::string m_configUri;

    /// Component name for data access API.
    const std::string m_componentName;

    /// Table name for data access API.
    const std::string m_tableName;
};

}  // namespace properties
}  // namespace alexaClientSDK

#endif  // ACSDK_PROPERTIES_PRIVATE_MISCSTORAGEPROPERTIES_H_
