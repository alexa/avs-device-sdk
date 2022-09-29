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

#ifndef ACSDK_PROPERTIESINTERFACES_TEST_MOCKPROPERTIES_H_
#define ACSDK_PROPERTIESINTERFACES_TEST_MOCKPROPERTIES_H_

#include <acsdk/PropertiesInterfaces/PropertiesInterface.h>
#include <acsdk/Test/GmockExtensions.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>

namespace alexaClientSDK {
namespace propertiesInterfaces {
namespace test {

//! Mock class for @c PropertiesInterface.
/**
 * \ingroup Lib_acsdkPropertiesInterfaces
 */
class MockProperties : public PropertiesInterface {
public:
    /// @name PropertiesInterface stub methods
    ///@{
    MOCK_NOEXCEPT_METHOD2(getString, bool(const std::string&, std::string&));
    MOCK_NOEXCEPT_METHOD2(putString, bool(const std::string&, const std::string&));
    MOCK_NOEXCEPT_METHOD2(getBytes, bool(const std::string&, Bytes&));
    MOCK_NOEXCEPT_METHOD2(putBytes, bool(const std::string&, const Bytes&));
    MOCK_NOEXCEPT_METHOD1(getKeys, bool(std::unordered_set<std::string>&));
    MOCK_NOEXCEPT_METHOD1(remove, bool(const std::string&));
    MOCK_NOEXCEPT_METHOD0(clear, bool());
    ///@}
};

}  // namespace test
}  // namespace propertiesInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_PROPERTIESINTERFACES_TEST_MOCKPROPERTIES_H_
