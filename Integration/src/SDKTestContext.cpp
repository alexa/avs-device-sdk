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

#include <fstream>
#include <sstream>

#include <gtest/gtest.h>

#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>

#include "Integration/SDKTestContext.h"

namespace alexaClientSDK {
namespace integration {
namespace test {

using namespace avsCommon::avs::initialization;

std::unique_ptr<SDKTestContext> SDKTestContext::create(const std::string& filePath, const std::string& overlay) {
    std::unique_ptr<SDKTestContext> context(new SDKTestContext(filePath, overlay));
    if (AlexaClientSDKInit::isInitialized()) {
        return context;
    }
    return nullptr;
}

SDKTestContext::~SDKTestContext() {
    AlexaClientSDKInit::uninitialize();
}

SDKTestContext::SDKTestContext(const std::string& filePath, const std::string& overlay) {
    std::vector<std::shared_ptr<std::istream>> streams;

    auto infile = std::shared_ptr<std::ifstream>(new std::ifstream(filePath));
    EXPECT_TRUE(infile->good());
    if (!infile->good()) {
        return;
    }
    streams.push_back(infile);

    auto overlayStream = std::shared_ptr<std::stringstream>(new std::stringstream());
    if (!overlay.empty()) {
        (*overlayStream) << overlay;
        streams.push_back(overlayStream);
    }

    EXPECT_TRUE(AlexaClientSDKInit::initialize(streams));
}

}  // namespace test
}  // namespace integration
}  // namespace alexaClientSDK
