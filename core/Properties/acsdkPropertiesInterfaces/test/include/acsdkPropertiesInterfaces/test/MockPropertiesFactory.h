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

#ifndef ACSDKPROPERTIESINTERFACES_TEST_MOCKPROPERTIESFACTORY_H_
#define ACSDKPROPERTIESINTERFACES_TEST_MOCKPROPERTIESFACTORY_H_

#include <gmock/gmock.h>

#include <acsdkPropertiesInterfaces/PropertiesFactoryInterface.h>
#include <acsdkPropertiesInterfaces/PropertiesInterface.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>

namespace alexaClientSDK {
namespace acsdkPropertiesInterfaces {
namespace test {

//! Mock class for @c PropertiesFactoryInterface.
/**
 * \ingroup PropertiesAPI
 */
class MockPropertiesFactory : public PropertiesFactoryInterface {
public:
    /// @name PropertiesFactoryInterface methods
    ///@{
    std::shared_ptr<PropertiesInterface> getProperties(const std::string& configUri) noexcept override;
    ///@}
    /// @name PropertiesFactoryInterface stub methods for 1.8.x gmock
    ///@{
    MOCK_METHOD1(_getProperties, std::shared_ptr<PropertiesInterface>(const std::string&));
    ///@}
};

inline std::shared_ptr<PropertiesInterface> MockPropertiesFactory::getProperties(
    const std::string& configUri) noexcept {
    return _getProperties(configUri);
}

}  // namespace test
}  // namespace acsdkPropertiesInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKPROPERTIESINTERFACES_TEST_MOCKPROPERTIESFACTORY_H_
