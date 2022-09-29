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

#include <gtest/gtest.h>

#include <acsdk/PropertiesInterfaces/test/StubPropertiesFactory.h>
#include <acsdk/PropertiesInterfaces/test/StubProperties.h>

namespace alexaClientSDK {
namespace propertiesInterfaces {
namespace test {

std::shared_ptr<StubPropertiesFactory> StubPropertiesFactory::create() noexcept {
    return std::shared_ptr<StubPropertiesFactory>(new StubPropertiesFactory);
}

StubPropertiesFactory::StubPropertiesFactory() noexcept {
}

std::shared_ptr<PropertiesInterface> StubPropertiesFactory::getProperties(const std::string& configUri) noexcept {
    return std::shared_ptr<StubProperties>(new StubProperties(shared_from_this(), configUri));
}

}  // namespace test
}  // namespace propertiesInterfaces
}  // namespace alexaClientSDK
