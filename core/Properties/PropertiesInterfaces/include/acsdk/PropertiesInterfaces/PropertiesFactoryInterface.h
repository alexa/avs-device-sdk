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

#ifndef ACSDK_PROPERTIESINTERFACES_PROPERTIESFACTORYINTERFACE_H_
#define ACSDK_PROPERTIESINTERFACES_PROPERTIESFACTORYINTERFACE_H_

#include <memory>
#include <string>

#include <acsdk/PropertiesInterfaces/PropertiesInterface.h>

namespace alexaClientSDK {
//! \addtogroup Lib_acsdkPropertiesInterfaces
//! @{
namespace propertiesInterfaces {

//! Factory interface to component properties.
/**
 * This interface provide a way to construct component-specific key-value storage. The storage is identified by
 * configuration URI (Uniform Resource Identifier) to disambiguate properties between different components. The format
 * of configuration URI is implementation specific, but must conform to RFC3986.
 *
 * Application may operate with a single or multiple instances of this interface, that use different implementation, or
 * provide access to different physical resources for access control.
 *
 * \ingroup Lib_acsdkPropertiesInterfaces
 * \sa Lib_acsdkProperties
 */
class PropertiesFactoryInterface {
public:
    /**
     * Destructor.
     */
    virtual ~PropertiesFactoryInterface() = default;

    //! Create properties interface for a given component and namespace.
    /**
     * This method creates interface to access component specific configuration. The implementation maps URI into
     * implementation-specific configuration container, and URI can be treated as a file location, database or table
     * name, or region in non-volatile memory.
     *
     * The method may return the same or different instances for the same configuration URI, but if different instances
     * are returned, all of them must provide access to the same configuration container.
     *
     * @param uri Resource URI. The format must conform to RFC3986, but handling is implementation-specific.
     *
     * @return Reference to properties adapter or nullptr on error.
     */
    virtual std::shared_ptr<PropertiesInterface> getProperties(const std::string& uri) noexcept = 0;
};

}  // namespace propertiesInterfaces
//! @}
}  // namespace alexaClientSDK

#endif  // ACSDK_PROPERTIESINTERFACES_PROPERTIESFACTORYINTERFACE_H_
