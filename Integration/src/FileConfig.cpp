/*
 * FileConfig.cpp
 *
 * Copyright 2016-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include "Integration/FileConfig.h"
#include <iostream>

namespace alexaClientSDK {
namespace integration {

FileConfig::FileConfig(const std::string& path) {
    std::ifstream infile(path);
    if (!infile.good()) {
        std::cerr << "Integration tests require credentials placed in " << path << std::endl;
    }
    for (std::string line; infile.good(); std::getline(infile, line)) {
        size_t index = line.find("=");
        if (std::string::npos != index) {
            std::string key = line.substr(0, index);
            std::string value = line.substr(index + 1);
            if ("clientId" == key) {
                m_clientId = value;
            } else if ("refreshToken" == key) {
                m_refreshToken = value;
            } else if ("clientSecret" == key) {
                m_clientSecret = value;
            }
        }
    }
}

} // namespace integration
} // namespace alexaClientSDK
