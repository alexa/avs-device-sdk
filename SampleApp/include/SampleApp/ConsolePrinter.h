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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_CONSOLEPRINTER_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_CONSOLEPRINTER_H_

#include <mutex>
#include <string>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Logger/LogStringFormatter.h>

namespace alexaClientSDK {
namespace sampleApp {

/**
 * A simple class that prints to the screen.
 */
class ConsolePrinter : public avsCommon::utils::logger::Logger {
public:
    /**
     * Constructor.
     */
    ConsolePrinter();

    /**
     * Prints a simple message along with an \n.
     *
     * @param stringToPrint The string to print.
     */
    static void simplePrint(const std::string& stringToPrint);

    /**
     * Prints a message with a pretty format with a \n after.
     *
     * @param stringToPrint The string to print.
     */
    static void prettyPrint(const std::string& stringToPrint);

    void emit(
        avsCommon::utils::logger::Level level,
        std::chrono::system_clock::time_point time,
        const char* threadMoniker,
        const char* text) override;

private:
    /// Used to serialize access to std::cout.
    static std::shared_ptr<std::mutex> m_globalMutex;

    /**
     * Holding a shared pointer to the mutex
     * to make sure the mutex is not already destroyed
     * when called from global's destructor
     */
    std::shared_ptr<std::mutex> m_mutex;

    /**
     * Object used to format strings for log messages.
     */
    avsCommon::utils::logger::LogStringFormatter m_logFormatter;
};

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_CONSOLEPRINTER_H_
