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

#include "acsdk/Sample/Console/ConsolePrinter.h"

#include <iostream>
#include <algorithm>
#include <cmath>

#include <AVSCommon/Utils/CoutMutex.h>

/**
 *  When using pretty print, we pad our strings in the beginning and in the end with the margin representation '#'
 *  and 7 spaces. E.g., if I pass "Hello world!" string, pretty print will look like:
 *  ############################
 *  #       Hello world!       #
 *  ############################
 */
static const size_t PADDING_LENGTH = 8;

static const std::string ALEXA_SAYS_HEADING = " Alexa Says ";

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

using namespace avsCommon::utils;

ConsolePrinter::ConsolePrinter() :
        avsCommon::utils::logger::Logger{avsCommon::utils::logger::Level::UNKNOWN},
        m_mutex{getCoutMutex()} {
}

void ConsolePrinter::simplePrint(const std::string& stringToPrint) {
    auto mutex = getCoutMutex();
    if (!mutex) {
        return;
    }

    std::lock_guard<std::mutex> lock{*mutex};
    std::cout << stringToPrint << std::endl;
}

void ConsolePrinter::prettyPrint(std::initializer_list<std::string> lines) {
    size_t maxLength = 0;
    for (auto& line : lines) {
        maxLength = std::max(line.size(), maxLength);
    }

    const std::string headingBorder(maxLength + (2 * PADDING_LENGTH), '#');
    std::ostringstream oss;
    oss << headingBorder << std::endl;

    // Write each line starting and ending with '#'
    auto padBegin = std::string("#");
    padBegin.append(PADDING_LENGTH - 1, ' ');
    for (auto& line : lines) {
        auto padEnd = std::string("#");
        padEnd.insert(padEnd.begin(), maxLength - line.size() + (PADDING_LENGTH - 1), ' ');
        oss << padBegin << line << padEnd << std::endl;
    }

    oss << headingBorder << std::endl;
    simplePrint(oss.str());
}

void ConsolePrinter::captionsPrint(const std::vector<std::string>& lines) {
    size_t maxLength = 0;
    for (auto& line : lines) {
        maxLength = std::max(line.size(), maxLength);
    }

    // The line length should be even here to allow for alignment of the header string and '#' boundaries
    if (maxLength % 2 != 0) {
        maxLength += 1;
    }

    size_t maxBorderLength = (2 * PADDING_LENGTH) + maxLength;
    size_t availableLength = (maxBorderLength - ALEXA_SAYS_HEADING.length());
    size_t headerLength = availableLength / 2 + availableLength % 2;

    const std::string headingBorder(headerLength, '#');
    std::ostringstream oss;
    oss << headingBorder << ALEXA_SAYS_HEADING << headingBorder << std::endl;

    // Write each line starting and ending with '#'
    auto padBegin = std::string("#");
    padBegin.append(PADDING_LENGTH - 1, ' ');
    for (auto& line : lines) {
        auto padEnd = std::string("#");
        padEnd.insert(padEnd.begin(), maxLength - line.size() + (PADDING_LENGTH - 1), ' ');
        oss << padBegin << line << padEnd << std::endl;
    }

    const std::string footingBorder(maxBorderLength, '#');
    oss << footingBorder << std::endl;
    simplePrint(oss.str());
}

void ConsolePrinter::prettyPrint(const std::string& stringToPrint) {
    prettyPrint({stringToPrint});
}

void ConsolePrinter::emit(
    avsCommon::utils::logger::Level level,
    std::chrono::system_clock::time_point time,
    const char* threadMoniker,
    const char* text) {
    std::lock_guard<std::mutex> lock{*m_mutex};
    std::cout << m_logFormatter.format(level, time, threadMoniker, text) << std::endl;
}

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK
