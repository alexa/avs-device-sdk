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

#include "SampleApp/ConsoleReader.h"
#include "SampleApp/SampleApplication.h"
#include "SampleApp/SampleApplicationReturnCodes.h"

#ifdef DIAGNOSTICS
#include "SampleApp/SDKDiagnostics.h"
#endif

#include <cstdlib>
#include <string>

using namespace alexaClientSDK::sampleApp;
using namespace alexaClientSDK::sampleApplications::common;

/**
 * Function that evaluates if the SampleApp invocation uses old-style or new-style opt-arg style invocation.
 *
 * @param argc The number of elements in the @c argv array.
 * @param argv An array of @a argc elements, containing the program name and all command-line arguments.
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
 * @param argv An array of @a argc elements, containing the program name and all command-line arguments.
 * @return @c EXIT_FAILURE if the program failed to initialize correctly, else @c EXIT_SUCCESS.
 */
int main(int argc, char* argv[]) {
    std::vector<std::string> configFiles;
    std::string logLevel;

    if (usesOptStyleArgs(argc, argv)) {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-C") == 0) {
                if (i + 1 == argc) {
                    ConsolePrinter::simplePrint("No config specified for -C option");
                    return SampleAppReturnCode::ERROR;
                }
                configFiles.push_back(std::string(argv[++i]));
                ConsolePrinter::simplePrint("configFile " + std::string(argv[i]));
            } else if (strcmp(argv[i], "-L") == 0) {
                if (i + 1 == argc) {
                    ConsolePrinter::simplePrint("No debugLevel specified for -L option");
                    return SampleAppReturnCode::ERROR;
                }
                logLevel = std::string(argv[++i]);
            } else {
                ConsolePrinter::simplePrint(
                    "USAGE: " + std::string(argv[0]) + " -C <config1.json> -C <config2.json> ... -C <configN.json> " +
                    " -L <log_level>");
                return SampleAppReturnCode::ERROR;
            }
        }
    } else {
        if (argc < 2) {
            ConsolePrinter::simplePrint(
                "USAGE: " + std::string(argv[0]) + " <path_to_AlexaClientSDKConfig.json> [log_level]");
            return SampleAppReturnCode::ERROR;
        }
        if (3 == argc) {
            logLevel = std::string(argv[2]);
        }

        configFiles.push_back(std::string(argv[1]));
        ConsolePrinter::simplePrint("configFile " + std::string(argv[1]));
    }

    auto consoleReader = std::make_shared<ConsoleReader>();

    std::unique_ptr<SampleApplication> sampleApplication;
    SampleAppReturnCode returnCode = SampleAppReturnCode::OK;

#ifdef DIAGNOSTICS
    std::shared_ptr<SDKDiagnostics> diagnostics = SDKDiagnostics::create();
    if (!diagnostics) {
        return SampleAppReturnCode::ERROR;
    }
#endif

    do {
        sampleApplication = SampleApplication::create(
            consoleReader,
            configFiles,
            logLevel
#ifdef DIAGNOSTICS
            ,
            diagnostics
#endif
        );

        if (!sampleApplication) {
            ConsolePrinter::simplePrint("Failed to create SampleApplication!");
            return SampleAppReturnCode::ERROR;
        }
        returnCode = sampleApplication->run();
        sampleApplication.reset();
    } while (SampleAppReturnCode::RESTART == returnCode);

    return returnCode;
}
