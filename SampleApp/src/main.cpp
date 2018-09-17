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

#include "SampleApp/ConsoleReader.h"
#include "SampleApp/SampleApplication.h"
#include "SampleApp/SampleApplicationReturnCodes.h"

#ifdef OBIGO_AIDAEMON
#include "AIDaemon/AIDaemon-dbus-generated.h"
#include "AIDaemon/base64.h"
#include "AIDaemon/AIDaemon-IPC.h"
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>

#include <thread>
#endif //OBIGO_AIDAEMON

#include <cstdlib>
#include <string>

using namespace alexaClientSDK::sampleApp;

#ifdef OBIGO_AIDAEMON
std::thread                     *m_pServerThread = nullptr;
guint                           m_idBus;
GMainLoop                       *m_pLoop;
AIDaemon                        *m_pDBusInterface = nullptr;
AIDaemonSkeleton                *m_pskeleton = nullptr;
std::shared_ptr<InteractionManager> m_gInteractionManager;
#endif //OBIGO_AIDAEMON


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

#ifdef OBIGO_AIDAEMON
void sendMessagesIPC(std::string MethodID, rapidjson::Document *data) {
    ConsolePrinter::simplePrint(__PRETTY_FUNCTION__);

    rapidjson::Document ipcdata(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& allocator = ipcdata.GetAllocator();

    ipcdata.AddMember(rapidjson::StringRef(AIDAEMON::IPC_METHODID), MethodID, allocator);
    ipcdata.AddMember(rapidjson::StringRef(AIDAEMON::IPC_DATA), *data, allocator);

    rapidjson::StringBuffer ipcdataBuf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(ipcdataBuf);
    if (!ipcdata.Accept(writer)) {
        ConsolePrinter::simplePrint("ERROR: Build json");
        return;
    }

    std::string tempipcdata = ipcdataBuf.GetString();
    std::string encoded = base64_encode(reinterpret_cast<const unsigned char*>(tempipcdata.c_str()), tempipcdata.length());
    ConsolePrinter::simplePrint(encoded.c_str());

    aidaemon__emit_send_messages(m_pDBusInterface, encoded.c_str());
}

gboolean on_handle_send_messages(
    AIDaemon *object,
    GDBusMethodInvocation *invocation,
    const gchar *arg_Data,
    gpointer user_data ) {

    using namespace alexaClientSDK::avsCommon::utils::json;

    std::stringstream logbuffer;

    ConsolePrinter::simplePrint(__PRETTY_FUNCTION__);

    std::string decoded = base64_decode((char*)(arg_Data));
    ConsolePrinter::simplePrint(decoded);

    rapidjson::Document payload;
    rapidjson::ParseResult result = payload.Parse(decoded);
    if (!result) {
        ConsolePrinter::simplePrint("ERROR: IPC data not parseable");
        return false;
    }

    std::string Method;
    if (!jsonUtils::retrieveValue(payload, AIDAEMON::IPC_METHODID, &Method)) {
        ConsolePrinter::simplePrint("ERROR: Cannot get IPC methodid");
        return false;
    } else {
        logbuffer << "IPC methodid : " << Method;
        ConsolePrinter::simplePrint(logbuffer.str());
        logbuffer.str("");
    }

    std::string IPCData;
    if (!jsonUtils::retrieveValue(payload, AIDAEMON::IPC_DATA, &IPCData)) {
        ConsolePrinter::simplePrint("There is not IPC data");
    } else {
        logbuffer << "IPC data : " << IPCData;
        ConsolePrinter::simplePrint(logbuffer.str());
        logbuffer.str("");
    }

    aidaemon__complete_send_messages(object, invocation);

    if (Method == AIDAEMON::IPC_METHODID_REQ_VR_START) {
        m_gInteractionManager->tap();
    } else if (Method == AIDAEMON::IPC_METHODID_REQ_VR_STOP) {
        m_gInteractionManager->stopForegroundActivity();
    } else {
        ConsolePrinter::simplePrint("ERROR: Cannot Handle this Method");
    }



    return true;
}

void OnBusAcquired( GDBusConnection * connection, const gchar * name, gpointer user_data ) {
    ConsolePrinter::simplePrint(__PRETTY_FUNCTION__);

    std::stringstream logbuffer;

    logbuffer << "DBus Connection : " << connection;
    ConsolePrinter::simplePrint(logbuffer.str());
    logbuffer.str("");

    logbuffer << "DBus Name : " << name;
    ConsolePrinter::simplePrint(logbuffer.str());
    logbuffer.str("");

    if (m_pDBusInterface != nullptr) return;
    if (m_pskeleton != nullptr) return;

    m_pDBusInterface = aidaemon__skeleton_new() ;
    logbuffer << "DBus skeleton : " << m_pDBusInterface;
    ConsolePrinter::simplePrint(logbuffer.str());
    logbuffer.str("");

    gulong handlerID = g_signal_connect (m_pDBusInterface, "handle-send-messages", G_CALLBACK (on_handle_send_messages), nullptr);
    logbuffer << "DBus HandlerID : " << handlerID;
    ConsolePrinter::simplePrint(logbuffer.str());
    logbuffer.str("");

    m_pskeleton = AIDAEMON__SKELETON (m_pDBusInterface);
    logbuffer << "AIDaemon skeleton : " << m_pskeleton;
    ConsolePrinter::simplePrint(logbuffer.str());
    logbuffer.str("");

    gboolean result = g_dbus_interface_skeleton_export(
        G_DBUS_INTERFACE_SKELETON(m_pskeleton),
        connection,
        AIDAEMON::SERVER_OBJECT.c_str(),
        nullptr);
    logbuffer << "Export Result : " << result;
    ConsolePrinter::simplePrint(logbuffer.str());
    logbuffer.str("");
}

bool makeDBusServer() {
    ConsolePrinter::simplePrint(__PRETTY_FUNCTION__);

    if (m_pServerThread != nullptr) return false;

    m_pServerThread = new std::thread([&] {
          m_idBus = g_bus_own_name(G_BUS_TYPE_SESSION, AIDAEMON::SERVER_INTERFACE.c_str(),
                                  G_BUS_NAME_OWNER_FLAGS_NONE, OnBusAcquired,
                                  nullptr, nullptr, nullptr, nullptr);
          if (! m_idBus) {
              ConsolePrinter::simplePrint("Fail to get bus");
          } else {
              ConsolePrinter::simplePrint("Success to get bus");
              m_pLoop = g_main_loop_new(nullptr, false);
            if (m_pLoop) {
                ConsolePrinter::simplePrint("Success to create a new GMainLoop");
    			g_main_loop_run(m_pLoop);
    		} else {
                ConsolePrinter::simplePrint("Fail to create a new GMainLoop");
    		}
          }
        });
    return true;
}
#endif //OBIGO_AIDAEMON

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
                    ConsolePrinter::simplePrint("No config specified for -C option");
                    return SampleAppReturnCode::ERROR;
                }
                configFiles.push_back(std::string(argv[++i]));
                ConsolePrinter::simplePrint("configFile " + std::string(argv[i]));
            } else if (strcmp(argv[i], "-K") == 0) {
                if (i + 1 == argc) {
                    ConsolePrinter::simplePrint("No wakeword input specified for -K option");
                    return SampleAppReturnCode::ERROR;
                }
                pathToKWDInputFolder = std::string(argv[++i]);
            } else if (strcmp(argv[i], "-L") == 0) {
                if (i + 1 == argc) {
                    ConsolePrinter::simplePrint("No debugLevel specified for -L option");
                    return SampleAppReturnCode::ERROR;
                }
                logLevel = std::string(argv[++i]);
            } else {
                ConsolePrinter::simplePrint(
                    "USAGE: " + std::string(argv[0]) + " -C <config1.json> -C <config2.json> ... -C <configN.json> " +
                    " -K <path_to_inputs_folder> -L <log_level>");
                return SampleAppReturnCode::ERROR;
            }
        }
    } else {
#if defined(KWD_KITTAI) || defined(KWD_SENSORY)
        if (argc < 3) {
            ConsolePrinter::simplePrint(
                "USAGE: " + std::string(argv[0]) +
                " <path_to_AlexaClientSDKConfig.json> <path_to_inputs_folder> [log_level]");
            return SampleAppReturnCode::ERROR;
        } else {
            pathToKWDInputFolder = std::string(argv[2]);
            if (4 == argc) {
                logLevel = std::string(argv[3]);
            }
        }
#else
        if (argc < 2) {
            ConsolePrinter::simplePrint(
                "USAGE: " + std::string(argv[0]) + " <path_to_AlexaClientSDKConfig.json> [log_level]");
            return SampleAppReturnCode::ERROR;
        }
        if (3 == argc) {
            logLevel = std::string(argv[2]);
        }
#endif

        configFiles.push_back(std::string(argv[1]));
        ConsolePrinter::simplePrint("configFile " + std::string(argv[1]));
    }

    auto consoleReader = std::make_shared<ConsoleReader>();

    std::unique_ptr<SampleApplication> sampleApplication;
    SampleAppReturnCode returnCode = SampleAppReturnCode::OK;

    #ifdef OBIGO_AIDAEMON
        makeDBusServer();
    #endif //OBIGO_AIDAEMON

    do {
        sampleApplication = SampleApplication::create(consoleReader, configFiles, pathToKWDInputFolder, logLevel);
        if (!sampleApplication) {
            ConsolePrinter::simplePrint("Failed to create to SampleApplication!");
            return SampleAppReturnCode::ERROR;
        }
        #ifdef OBIGO_AIDAEMON
        m_gInteractionManager = sampleApplication->getInteractionManager();
        #endif //OBIGO_AIDAEMON
        returnCode = sampleApplication->run();
        sampleApplication.reset();
    } while (SampleAppReturnCode::RESTART == returnCode);

    return returnCode;
}
