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

#ifndef ACSDKPROPERTIESINTERFACES_TEST_MOCKPROPERTIES_H_
#define ACSDKPROPERTIESINTERFACES_TEST_MOCKPROPERTIES_H_

#include <gmock/gmock.h>

#include <acsdkPropertiesInterfaces/PropertiesInterface.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>

namespace alexaClientSDK {
namespace acsdkPropertiesInterfaces {
namespace test {

//! Mock class for @c PropertiesInterface.
/**
 * \ingroup PropertiesAPI
 */
class MockProperties : public PropertiesInterface {
public:
    /// @name PropertiesInterface methods
    ///@{
    bool getString(const std::string& key, std::string& value) noexcept override;
    bool putString(const std::string& key, const std::string& value) noexcept override;
    bool getBytes(const std::string& key, Bytes& value) noexcept override;
    bool putBytes(const std::string& key, const Bytes& value) noexcept override;
    bool getKeys(std::unordered_set<std::string>& keys) noexcept override;
    bool remove(const std::string& key) noexcept override;
    bool clear() noexcept override;
    ///@}
    /// @name PropertiesInterface stub methods for 1.8.x gmock
    ///@{
    MOCK_METHOD2(_getString, bool(const std::string&, std::string&));
    MOCK_METHOD2(_putString, bool(const std::string&, const std::string&));
    MOCK_METHOD2(_getBytes, bool(const std::string&, Bytes&));
    MOCK_METHOD2(_putBytes, bool(const std::string&, const Bytes&));
    MOCK_METHOD1(_getKeys, bool(std::unordered_set<std::string>&));
    MOCK_METHOD1(_remove, bool(const std::string&));
    MOCK_METHOD0(_clear, bool());
    ///@}
};

inline bool MockProperties::getString(const std::string& key, std::string& value) noexcept {
    return _getString(key, value);
}

inline bool MockProperties::putString(const std::string& key, const std::string& value) noexcept {
    return _putString(key, value);
}

inline bool MockProperties::getBytes(const std::string& key, Bytes& value) noexcept {
    return _getBytes(key, value);
}

inline bool MockProperties::putBytes(const std::string& key, const Bytes& value) noexcept {
    return _putBytes(key, value);
}

inline bool MockProperties::getKeys(std::unordered_set<std::string>& keys) noexcept {
    return _getKeys(keys);
}

inline bool MockProperties::remove(const std::string& key) noexcept {
    return _remove(key);
}

inline bool MockProperties::clear() noexcept {
    return _clear();
}

}  // namespace test
}  // namespace acsdkPropertiesInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKPROPERTIESINTERFACES_TEST_MOCKPROPERTIES_H_
