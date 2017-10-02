/*
 * ConsolePrinter.cpp
 *
 * Copyright (c) 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "SampleApp/ConsolePrinter.h"

#include <iostream>

#include <AVSCommon/Utils/Logger/LoggerUtils.h>

namespace alexaClientSDK {
namespace sampleApp {

std::mutex ConsolePrinter::m_mutex;

ConsolePrinter::ConsolePrinter() : avsCommon::utils::logger::Logger(avsCommon::utils::logger::Level::UNKNOWN) {
}

void ConsolePrinter::simplePrint(const std::string& stringToPrint) {
    std::lock_guard<std::mutex> lock{m_mutex};
    std::cout << stringToPrint << std::endl;
}

void ConsolePrinter::prettyPrint(const std::string& stringToPrint) {
    std::lock_guard<std::mutex> lock{m_mutex};
    std::string line(stringToPrint.size() + 16, '#');
    std::cout << line << std::endl;
    std::cout << "#       " << stringToPrint << "       #" << std::endl;
    std::cout << line << std::endl;
}

void ConsolePrinter::emit(
    avsCommon::utils::logger::Level level,
    std::chrono::system_clock::time_point time,
    const char* threadMoniker,
    const char* text) {
    std::lock_guard<std::mutex> lock{m_mutex};
    std::cout << avsCommon::utils::logger::formatLogString(level, time, threadMoniker, text) << std::endl;
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
