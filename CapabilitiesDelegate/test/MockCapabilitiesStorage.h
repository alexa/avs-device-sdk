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

#ifndef ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_TEST_MOCKCAPABILITIESSTORAGE_H_
#define ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_TEST_MOCKCAPABILITIESSTORAGE_H_

#include <string>
#include <unordered_map>

#include <CapabilitiesDelegate/Storage/CapabilitiesDelegateStorageInterface.h>

namespace alexaClientSDK {
namespace capabilitiesDelegate {
namespace test {

/**
 * Mock class for @c CapabilitiesDelegateStorageInterface.
 */
class MockCapabilitiesDelegateStorage : public storage::CapabilitiesDelegateStorageInterface {
public:
    MOCK_METHOD0(createDatabase, bool());
    MOCK_METHOD0(open, bool());
    MOCK_METHOD0(close, void());
    MOCK_METHOD2(store, bool(const std::string&, const std::string&));
    MOCK_METHOD1(store, bool(const std::unordered_map<std::string, std::string>&));
    MOCK_METHOD1(load, bool(std::unordered_map<std::string, std::string>*));
    MOCK_METHOD2(load, bool(const std::string&, std::string*));
    MOCK_METHOD1(erase, bool(const std::string&));
    MOCK_METHOD1(erase, bool(const std::unordered_map<std::string, std::string>&));
    MOCK_METHOD0(clearDatabase, bool());
};

}  // namespace test
}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_TEST_MOCKCAPABILITIESSTORAGE_H_
