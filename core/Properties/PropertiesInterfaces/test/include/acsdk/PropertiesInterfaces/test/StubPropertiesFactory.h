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

#ifndef ACSDK_PROPERTIESINTERFACES_TEST_STUBPROPERTIESFACTORY_H_
#define ACSDK_PROPERTIESINTERFACES_TEST_STUBPROPERTIESFACTORY_H_

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <acsdk/PropertiesInterfaces/PropertiesFactoryInterface.h>
#include <acsdk/Test/GmockExtensions.h>

namespace alexaClientSDK {
namespace propertiesInterfaces {
namespace test {

//! In-memory stub implementation of @c PropertiesFactoryInterface.
/**
 * In memory implementation for @c PropertiesFactoryInterface. This class is not thread safe.
 *
 * \ingroup Lib_acsdkPropertiesInterfaces
 */
class StubPropertiesFactory
        : public PropertiesFactoryInterface
        , public std::enable_shared_from_this<StubPropertiesFactory> {
public:
    ///! Creates new factory instance.
    /**
     * This method provides a new property factory instance. This instance has own in-memory storage for all
     * properties.
     *
     * @return New object instance.
     */
    static std::shared_ptr<StubPropertiesFactory> create() noexcept;

    /// @name PropertiesFactoryInterface functions.
    /// @{
    std::shared_ptr<PropertiesInterface> getProperties(const std::string& configUri) noexcept override;
    ///@}

private:
    friend class StubProperties;

    /// Private constructor
    StubPropertiesFactory() noexcept;

    /// Container to keep stored values. The format of the key is "configUri/key".
    std::unordered_map<std::string, std::pair<char, PropertiesInterface::Bytes>> m_storage;
};

}  // namespace test
}  // namespace propertiesInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_PROPERTIESINTERFACES_TEST_STUBPROPERTIESFACTORY_H_
