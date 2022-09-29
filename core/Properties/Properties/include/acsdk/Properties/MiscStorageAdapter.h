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

#ifndef ACSDK_PROPERTIES_MISCSTORAGEADAPTER_H_
#define ACSDK_PROPERTIES_MISCSTORAGEADAPTER_H_

#include <memory>
#include <string>

#include <acsdk/PropertiesInterfaces/PropertiesFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>

namespace alexaClientSDK {
namespace properties {

using alexaClientSDK::avsCommon::sdkInterfaces::storage::MiscStorageInterface;
using alexaClientSDK::propertiesInterfaces::PropertiesFactoryInterface;

/**
 * @brief Interface to map properties config URI into component name and table name.
 *
 * This interface connects \ref Lib_acsdkPropertiesInterfaces and @c MiscStorageInterface.
 *
 * @c PropertiesFactoryInterface uses configuration URI to open properties container. When working with @c
 * MiscStorageInterface this URI must be mapped into @a componentName and @a tableName parameters.
 *
 * @sa Lib_acsdkPropertiesInterfaces
 * @sa alexaClientSDK::propertiesInterfaces::PropertiesFactoryInterface
 * @sa alexaClientSDK::avsCommon::sdkInterfaces::storage::MiscStorageInterface
 * @ingroup Lib_acsdkProperties
 */
class MiscStorageUriMapperInterface {
public:
    /// @brief Default destructor.
    virtual ~MiscStorageUriMapperInterface() noexcept = default;

    /**
     * @brief Extracts component name and table name from configuration URI.
     *
     * This method maps configuration URI from @c PropertiesFactoryInterface into component name and table name for @c
     * MiscStorageInterface.
     *
     * This method must be idempotent, and always return the same result for the same input.
     *
     * @param[in]   configUri       Configuration URI.
     * @param[out]  componentName   Component.
     * @param[out]  tableName       Table name.
     *
     * @return True if operation succeeds. If false is returned the state of \a componentName and \a tableName is
     * undefined.
     */
    virtual bool extractComponentAndTableName(
        const std::string& configUri,
        std::string& componentName,
        std::string& tableName) noexcept = 0;
};

/**
 * @brief Generic URI mapper for MiscStorageInterface adapter.
 *
 * This object converts configuration URI into component name and table name. The object expects that the URI contains
 * only component name and table name separated by a single character. For example, when parsing "component/tableName"
 * URI and using '/' as a separator, the object will return "component" as a component name, and "tableName" as a table
 * name.
 *
 * @sa createPropertiesFactory()
 * @ingroup Lib_acsdkProperties
 */
class SimpleMiscStorageUriMapper : public MiscStorageUriMapperInterface {
public:
    /**
     * @brief Creates mapper instance.
     *
     * @param[in] sep Separator character.
     *
     * @return New object reference or nullptr on error.
     */
    static std::shared_ptr<SimpleMiscStorageUriMapper> create(char sep = '/') noexcept;

    /// @name MiscStorageUriMapperInterface methods
    /// @{
    bool extractComponentAndTableName(
        const std::string& configUri,
        std::string& componentName,
        std::string& tableName) noexcept override;
    /// @}
private:
    SimpleMiscStorageUriMapper(char separator) noexcept;

    const char m_separator;
};

/**
 * @brief Creates @c PropertiesFactoryInterface from @c MiscStorageInterface.
 *
 * The method automatically creates database if it is not created. When user creates \c PropertiesInterface, the
 * implementation automatically creates corresponding table.
 *
 * Because underlying interface supports only string properties, the implementation uses base64 encoding to store
 * all binary properties. This may cause side effects, as when content is decoded using base64, the result may contain
 * additional padding 0 bytes, and client code must work correctly in this case.
 *
 * @param[in] innerStorage  Storage reference. This parameter must not be nullptr.
 * @param[in] nameMapper    Name mapper interface. This interface will be used to map configuration URI into table name
 *                          and component name values when accessing \ref MiscStorageInterface API.
 *
 * @return Factory reference or nullptr on error.
 * @ingroup Lib_acsdkProperties
 */
std::shared_ptr<PropertiesFactoryInterface> createPropertiesFactory(
    const std::shared_ptr<MiscStorageInterface>& innerStorage,
    const std::shared_ptr<MiscStorageUriMapperInterface>& nameMapper = SimpleMiscStorageUriMapper::create()) noexcept;

}  // namespace properties
}  // namespace alexaClientSDK

#endif  // ACSDK_PROPERTIES_MISCSTORAGEADAPTER_H_
