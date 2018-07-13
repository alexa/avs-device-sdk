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

#include "SampleApp/SampleApplication.h"

#include <cstdlib>
#include <string>

/**
 * This serves as the starting point for the application. This code instantiates the @c UserInputManager and processes
 * user input until the @c run() function returns.
 *
 * @param argc The number of elements in the @c argv array.
 * @param argv An array of @argc elements, containing the program name and all command-line arguments.
 * @return @c EXIT_FAILURE if the program failed to initialize correctly, else @c EXIT_SUCCESS.
 */
int main(int argc, char** argv) {
    std::string pathToConfig;
    std::string pathToInputFolder;
    std::string logLevel;

#if defined(KWD_KITTAI) || defined(KWD_SENSORY)
    if (argc < 3) {
        alexaClientSDK::sampleApp::ConsolePrinter::simplePrint(
            "USAGE: " + std::string(argv[0]) +
            " <path_to_AlexaClientSDKConfig.json> <path_to_inputs_folder> [log_level]");
        return EXIT_FAILURE;
    } else {
        pathToInputFolder = std::string(argv[2]);
        if (4 == argc) {
            logLevel = std::string(argv[3]);
        }
    }
#else
    if (argc < 2) {
        alexaClientSDK::sampleApp::ConsolePrinter::simplePrint(
            "USAGE: " + std::string(argv[0]) + " <path_to_AlexaClientSDKConfig.json> [log_level]");
        return EXIT_FAILURE;
    }
    if (3 == argc) {
        logLevel = std::string(argv[2]);
    }
#endif

    pathToConfig = std::string(argv[1]);

    auto sampleApplication =
        alexaClientSDK::sampleApp::SampleApplication::create(pathToConfig, pathToInputFolder, logLevel);
    if (!sampleApplication) {
        alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Failed to create to SampleApplication!");
        return EXIT_FAILURE;
    }

    // This will run until the user specifies the "quit" command.
    sampleApplication->run();

    return EXIT_SUCCESS;
}
