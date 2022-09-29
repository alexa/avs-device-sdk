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

#ifndef ACSDK_PROPERTIESINTERFACES_TEST_STUBPROPERTIES_H_
#define ACSDK_PROPERTIESINTERFACES_TEST_STUBPROPERTIES_H_

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <acsdk/PropertiesInterfaces/PropertiesInterface.h>
#include <acsdk/PropertiesInterfaces/test/StubPropertiesFactory.h>
#include <acsdk/Test/GmockExtensions.h>

namespace alexaClientSDK {
namespace propertiesInterfaces {
namespace test {

//! In-memory stub implementation of @c PropertiesInterface.
/**
 * This class provides in-memory implementation of \c PropertiesInterface. Users can create instances of this class
 * by using \ref StubPropertiesFactory.
 *
 * \sa StubPropertiesFactory.
 * \ingroup Lib_acsdkPropertiesInterfaces
 */
class StubProperties : public PropertiesInterface {
public:
    /// @name PropertiesFactoryInterface functions.
    /// @{
    bool getString(const std::string& key, std::string& value) noexcept override;
    bool putString(const std::string& key, const std::string& value) noexcept override;
    bool getBytes(const std::string& key, Bytes& value) noexcept override;
    bool putBytes(const std::string& key, const Bytes& value) noexcept override;
    bool getKeys(std::unordered_set<std::string>& keys) noexcept override;
    bool remove(const std::string& key) noexcept override;
    bool clear() noexcept override;
    ///@}
private:
    friend class StubPropertiesFactory;

    StubProperties(const std::shared_ptr<StubPropertiesFactory>& owner, const std::string& configUri) noexcept;

    /// Provides fully qualified name in the parent's container
    std::string createFullyQualifiedName(const std::string& keyName) noexcept;

    /// Provides prefix for container-owned keys.
    std::string createKeyPrefix() noexcept;

    /// Reference of owning class that contains all the data.
    const std::shared_ptr<StubPropertiesFactory> m_owner;

    /// Configuration URI.
    const std::string m_configUri;
};

}  // namespace test
}  // namespace propertiesInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_PROPERTIESINTERFACES_TEST_STUBPROPERTIES_H_
