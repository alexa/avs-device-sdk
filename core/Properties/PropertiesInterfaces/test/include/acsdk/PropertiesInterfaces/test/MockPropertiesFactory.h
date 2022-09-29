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

#ifndef ACSDK_PROPERTIESINTERFACES_TEST_MOCKPROPERTIESFACTORY_H_
#define ACSDK_PROPERTIESINTERFACES_TEST_MOCKPROPERTIESFACTORY_H_

#include <acsdk/PropertiesInterfaces/PropertiesFactoryInterface.h>
#include <acsdk/PropertiesInterfaces/PropertiesInterface.h>
#include <acsdk/Test/GmockExtensions.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>

namespace alexaClientSDK {
namespace propertiesInterfaces {
namespace test {

//! Mock class for @c PropertiesFactoryInterface.
/**
 * \ingroup Lib_acsdkPropertiesInterfaces
 */
class MockPropertiesFactory : public PropertiesFactoryInterface {
public:
    /// @name PropertiesFactoryInterface stub methods.
    ///@{
    MOCK_NOEXCEPT_METHOD1(getProperties, std::shared_ptr<PropertiesInterface>(const std::string&));
    ///@}
};

}  // namespace test
}  // namespace propertiesInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_PROPERTIESINTERFACES_TEST_MOCKPROPERTIESFACTORY_H_
