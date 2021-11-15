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

#ifndef ACSDKPROPERTIES_PRIVATE_MISCSTORAGEPROPERTIESFACTORY_H_
#define ACSDKPROPERTIES_PRIVATE_MISCSTORAGEPROPERTIESFACTORY_H_

#include <mutex>

#include <acsdkPropertiesInterfaces/PropertiesFactoryInterface.h>
#include <acsdkProperties/MiscStorageAdapter.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>

namespace alexaClientSDK {
namespace acsdkProperties {

using alexaClientSDK::acsdkPropertiesInterfaces::PropertiesFactoryInterface;
using alexaClientSDK::acsdkPropertiesInterfaces::PropertiesInterface;
using alexaClientSDK::avsCommon::sdkInterfaces::storage::MiscStorageInterface;

/**
 * @brief Properties factory for MiscStorageInterface.
 *
 * This class adapts \ref MiscStorageInterface into \ref PropertiesFactoryInterface.
 *
 * @ingroup PropertiesIMPL
 */
class MiscStoragePropertiesFactory : public PropertiesFactoryInterface {
public:
    static std::shared_ptr<PropertiesFactoryInterface> create(
        const std::shared_ptr<MiscStorageInterface>& storage,
        const std::shared_ptr<MiscStorageUriMapperInterface>& uriMapper) noexcept;

    /// @name PropertiesFactoryInterface methods
    /// @{
    std::shared_ptr<PropertiesInterface> getProperties(const std::string& configURI) noexcept override;
    /// @}

private:
    MiscStoragePropertiesFactory(
        const std::shared_ptr<MiscStorageInterface>& storage,
        const std::shared_ptr<MiscStorageUriMapperInterface>& uriMapper) noexcept;

    /// Helper to initialize the factory.
    bool init() noexcept;

    /// Helper to cleanup \a m_openProperties from expired references.
    void dropNullReferences() noexcept;

    /// Inner storage reference.
    const std::shared_ptr<MiscStorageInterface> m_storage;

    /// URI mapper to determine component name and table name.
    const std::shared_ptr<MiscStorageUriMapperInterface> m_uriMapper;

    /// Mutex to serialize access to properties cache.
    std::mutex m_stateMutex;

    /// Properties cache to return the same object reference as long as it is in use.
    std::unordered_map<std::string, std::weak_ptr<PropertiesInterface>> m_openProperties;
};

}  // namespace acsdkProperties
}  // namespace alexaClientSDK

#endif  // ACSDKPROPERTIES_PRIVATE_MISCSTORAGEPROPERTIESFACTORY_H_
