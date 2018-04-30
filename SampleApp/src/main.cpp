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
 * Function that evaluates if the SampleApp invocation uses old-style or new-style opt-arg style invocation.
 *
 * @param argc The number of elements in the @c argv array.
 * @param argv An array of @argc elements, containing the program name and all command-line arguments.
 * @return @c true of the invocation uses optarg style argument @c false otherwise.
 */
bool usesOptStyleArgs(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-C") || !strcmp(argv[i], "-K") || !strcmp(argv[i], "-L")) {
            return true;
        }
    }

    return false;
}

/**
 * This serves as the starting point for the application. This code instantiates the @c UserInputManager and processes
 * user input until the @c run() function returns.
 *
 * @param argc The number of elements in the @c argv array.
 * @param argv An array of @argc elements, containing the program name and all command-line arguments.
 * @return @c EXIT_FAILURE if the program failed to initialize correctly, else @c EXIT_SUCCESS.
 */
int main(int argc, char* argv[]) {
    std::vector<std::string> configFiles;
    std::string pathToKWDInputFolder;
    std::string logLevel;

    if (usesOptStyleArgs(argc, argv)) {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-C") == 0) {
                if (i + 1 == argc) {
                    alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("No config specified for -C option");
                    return EXIT_FAILURE;
                }
                configFiles.push_back(std::string(argv[++i]));
                alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("configFile " + std::string(argv[i]));
            } else if (strcmp(argv[i], "-K") == 0) {
                if (i + 1 == argc) {
                    alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("No wakeword input specified for -K option");
                    return EXIT_FAILURE;
                }
                pathToKWDInputFolder = std::string(argv[++i]);
            } else if (strcmp(argv[i], "-L") == 0) {
                if (i + 1 == argc) {
                    alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("No debugLevel specified for -L option");
                    return EXIT_FAILURE;
                }
                logLevel = std::string(argv[++i]);
            } else {
                alexaClientSDK::sampleApp::ConsolePrinter::simplePrint(
                    "USAGE: " + std::string(argv[0]) + " -C <config1.json> -C <config2.json> ... -C <configN.json> " +
                    " -K <path_to_inputs_folder> -L <log_level>");
                return EXIT_FAILURE;
            }
        }
    } else {
#if defined(KWD_KITTAI) || defined(KWD_SENSORY)
        if (argc < 3) {
            alexaClientSDK::sampleApp::ConsolePrinter::simplePrint(
                "USAGE: " + std::string(argv[0]) +
                " <path_to_AlexaClientSDKConfig.json> <path_to_inputs_folder> [log_level]");
            return EXIT_FAILURE;
        } else {
            pathToKWDInputFolder = std::string(argv[2]);
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

        configFiles.push_back(std::string(argv[1]));
        alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("configFile " + std::string(argv[1]));
    }

    auto sampleApplication =
        alexaClientSDK::sampleApp::SampleApplication::create(configFiles, pathToKWDInputFolder, logLevel);
    if (!sampleApplication) {
        alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Failed to create to SampleApplication!");
        return EXIT_FAILURE;
    }

    // This will run until the user specifies the "quit" command.
    sampleApplication->run();

    return EXIT_SUCCESS;
}
