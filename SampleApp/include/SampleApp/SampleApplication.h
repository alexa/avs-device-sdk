/*
 * SampleApplication.h
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

#ifndef ALEXA_CLIENT_SDK_SAMPLE_APP_INCLUDE_SAMPLE_APP_SAMPLE_APPLICATION_H_
#define ALEXA_CLIENT_SDK_SAMPLE_APP_INCLUDE_SAMPLE_APP_SAMPLE_APPLICATION_H_

#include "ConsolePrinter.h"
#include "UserInputManager.h"

#ifdef KWD
#include <KWD/AbstractKeywordDetector.h>
#endif

#include <memory>

namespace alexaClientSDK {
namespace sampleApp {

/// Class to manage the top-level components of the AVS Client Application
class SampleApplication {
public:
    /**
     * Create a SampleApplication.
     *
     * @param pathToConfig The path to the SDK configuration file.
     * @param pathToInputFolder The path to the inputs folder containing data files needed by this application.
     * @param logLevel The level of logging to enable.  If this parameter is an empty string, the SDK's default
     *     logging level will be used.
     * @return A new @c SampleApplication, or @c nullptr if the operation failed.
     */
    static std::unique_ptr<SampleApplication> create(
        const std::string& pathToConfig,
        const std::string& pathToInputFolder,
        const std::string& logLevel = "");

    /// Runs the application, blocking until the user asks the application to quit.
    void run();

private:
    /**
     * Initialize a SampleApplication.
     *
     * @param pathToConfig The path to the SDK configuration file.
     * @param pathToInputFolder The path to the inputs folder containing data files needed by this application.
     * @param logLevel The level of logging to enable.  If this parameter is an empty string, the SDK's default
     *     logging level will be used.
     * @return @c true if initialization succeeded, else @c false.
     */
    bool initialize(const std::string& pathToConfig, const std::string& pathToInputFolder, const std::string& logLevel);

    /// The @c UserInputManager which controls the client.
    std::unique_ptr<UserInputManager> m_userInputManager;

#ifdef KWD
    /// The Wakeword Detector which can wake up the client using audio input.
    std::unique_ptr<kwd::AbstractKeywordDetector> m_keywordDetector;
#endif
};

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLE_APP_INCLUDE_SAMPLE_APP_SAMPLE_APPLICATION_H_
