/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_SDKTESTCONTEXT_H_
#define ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_SDKTESTCONTEXT_H_

#include <memory>
#include <string>

namespace alexaClientSDK {
namespace integration {
namespace test {

/**
 * Class providing lifecycle management of resources needed for testing the @c Alexa @c Client @c SDK.
 */
class SDKTestContext {
public:
    /**
     * Create an SDKTestContext
     *
     * @note Only one instance of this class should exist at a time - but it is okay (and expected)
     * that multiple instances of this class will be created (and destroyed) during one execution
     * of the application using this class.
     *
     * Creating an instance of this class provides initialization of the @c Alexa @c Client @c SDK
     * (which includes initialization of @ libcurl and @c ConfigurationNode.
     *
     * @param filePath The path to a config file.
     * @param overlay A @c JSON string containing values to overlay on the contents of the configuration file.
     * @return An SDKTestContext instance or nullptr if the operation failed.
     */
    static std::unique_ptr<SDKTestContext> create(const std::string& filePath, const std::string& overlay = "");

    /**
     * Destructor de-initializes the @c Alexa @c Client @c SDK.
     */
    ~SDKTestContext();

private:
    /**
     * Constructor.
     *
     * @param filePath The path to a config file.
     * @param overlay A @c JSON string containing values to overlay on the contents of the configuration file.
     */
    SDKTestContext(const std::string& filePath, const std::string& overlay);
};

}  // namespace test
}  // namespace integration
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_SDKTESTCONTEXT_H_
