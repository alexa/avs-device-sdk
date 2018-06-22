/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <cstdio>
#include <iomanip>
#include <iostream>

#include "AVSCommon/Utils/Logger/LoggerUtils.h"
#include "AVSCommon/Utils/Logger/Logger.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {

void acsdkDebug9(const LogEntry& entry) {
    logEntry(Level::DEBUG9, entry);
}

void acsdkDebug8(const LogEntry& entry) {
    logEntry(Level::DEBUG8, entry);
}

void acsdkDebug7(const LogEntry& entry) {
    logEntry(Level::DEBUG7, entry);
}

void acsdkDebug6(const LogEntry& entry) {
    logEntry(Level::DEBUG6, entry);
}

void acsdkDebug5(const LogEntry& entry) {
    logEntry(Level::DEBUG5, entry);
}

void acsdkDebug4(const LogEntry& entry) {
    logEntry(Level::DEBUG4, entry);
}

void acsdkDebug3(const LogEntry& entry) {
    logEntry(Level::DEBUG3, entry);
}

void acsdkDebug2(const LogEntry& entry) {
    logEntry(Level::DEBUG2, entry);
}

void acsdkDebug1(const LogEntry& entry) {
    logEntry(Level::DEBUG1, entry);
}

void acsdkDebug0(const LogEntry& entry) {
    logEntry(Level::DEBUG0, entry);
}

void acsdkDebug(const LogEntry& entry) {
    logEntry(Level::DEBUG0, entry);
}

void acsdkInfo(const LogEntry& entry) {
    logEntry(Level::INFO, entry);
}

void acsdkWarn(const LogEntry& entry) {
    logEntry(Level::WARN, entry);
}

void acsdkError(const LogEntry& entry) {
    logEntry(Level::ERROR, entry);
}

void acsdkCritical(const LogEntry& entry) {
    logEntry(Level::CRITICAL, entry);
}

void logEntry(Level level, const LogEntry& entry) {
    Logger& loggerInstance = ACSDK_GET_LOGGER_FUNCTION();
    loggerInstance.log(level, entry);
}

void dumpBytesToStream(std::ostream& stream, const char* prefix, size_t width, const unsigned char* data, size_t size) {
    std::ios incomingFormat(nullptr);
    incomingFormat.copyfmt(stream);

    stream << std::hex << std::right << std::setfill('0');

    for (size_t ix = 0; ix < size; ix += width) {
        // Output byte offset for this row.
        stream << prefix << std::setw(8) << ix;
        stream << " : ";

        // Output bytes for this row in hex.
        auto columnLimit = ix + width;
        auto byteLimit = columnLimit < size ? columnLimit : size;
        for (size_t iy = ix; iy < columnLimit; iy++) {
            if (iy < byteLimit) {
                stream << std::setw(2) << static_cast<unsigned int>(data[iy]);
            } else {
                stream << "  ";
            }
            // Split the output in to 4 byte (8 hex digit) wide columns.
            if ((iy < columnLimit - 1) && (iy & 0x03) == 0x03) {
                stream << " ";
            }
        }

        stream << " : ";

        // Output bytes as visible characters.
        for (size_t iy = ix; iy < byteLimit; iy++) {
            auto ch = data[iy];
            if (std::isprint(ch)) {
                stream << ch;
            } else {
                stream << '.';
            }
        }

        stream << std::endl;
    }

    stream.copyfmt(incomingFormat);
}

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
