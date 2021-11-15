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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_INITIALIZATION_ALEXACLIENTSDKINIT_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_INITIALIZATION_ALEXACLIENTSDKINIT_H_

#include <atomic>
#include <iostream>
#include <memory>
#include <vector>

#include <AVSCommon/AVS/Initialization/InitializationParameters.h>
#include <AVSCommon/Utils/Timing/TimerDelegateFactory.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace initialization {

/**
 * Class encapsulating the lifetime of initialization of the Alexa Client SDK.
 */
class AlexaClientSDKInit {
public:
    /**
     * @deprecated v1.21.0. This method does not support some new features, such as low power mode.
     * Get a function to create an instance of AlexaClientSDKInit.
     *
     * @param jsonStreams Vector of @c istreams containing JSON documents from which
     * to parse configuration parameters. Streams are processed in the order they appear in the vector. When a
     * value appears in more than one JSON stream the last processed stream's value overwrites the previous value
     * (and a debug log entry will be created). This allows for specifying default settings (by providing them
     * first) and specifying the configuration from multiple sources (e.g. a separate stream for each component).
     * Documentation of the JSON configuration format and methods to access the resulting global configuration
     * can be found here: avsCommon::utils::configuration::ConfigurationNode.
     * @return A  function to create an instance of AlexaClientSDKInit.
     */
    static std::function<std::shared_ptr<AlexaClientSDKInit>(std::shared_ptr<utils::logger::Logger>)>
    getCreateAlexaClientSDKInit(const std::vector<std::shared_ptr<std::istream>>& jsonStreams);

    /**
     * Get a function to create an instance of AlexaClientSDKInit.
     *
     * @note To enable low power mode, the PowerResourceManager must be added to the @c InitializationParameters.
     *
     * @param initParams The @c InitializationParameters.
     * @return A function to create an instance of AlexaClientSDKInit.
     */
    static std::function<std::shared_ptr<AlexaClientSDKInit>(std::shared_ptr<utils::logger::Logger>)>
    getCreateAlexaClientSDKInit(const std::shared_ptr<InitializationParameters>& initParams);

    /**
     * Destructor.
     */
    ~AlexaClientSDKInit();

    /**
     * Checks whether the Alexa Client SDK has been initialized.
     *
     * @return Whether the Alexa Client SDK has been initialized.
     */
    static bool isInitialized();

    /**
     * @deprecated v1.21.0
     * Initialize the Alexa Client SDK. This must be called before any Alexa Client SDK modules are created.
     *
     * This function must be called before any threads in the process have been created by the
     * program; this function is not thread safe. This requirement is present because initialize()
     * calls functions of other libraries that have the same requirements and thread safety.
     * terminate() must be called for each initialize() called.
     *
     * @param jsonStreams Vector of @c istreams containing JSON documents from which
     * to parse configuration parameters. Streams are processed in the order they appear in the vector. When a
     * value appears in more than one JSON stream the last processed stream's value overwrites the previous value
     * (and a debug log entry will be created). This allows for specifying default settings (by providing them
     * first) and specifying the configuration from multiple sources (e.g. a separate stream for each component).
     * Documentation of the JSON configuration format and methods to access the resulting global configuration
     * can be found here: avsCommon::utils::configuration::ConfigurationNode.
     *
     * @return Whether the initialization was successful.
     */
    static bool initialize(const std::vector<std::shared_ptr<std::istream>>& jsonStreams);

    /**
     * Initialize the Alexa Client SDK. This must be called before any Alexa Client SDK modules are created.
     *
     * This function must be called before any threads in the process have been created by the
     * program; this function is not thread safe. This requirement is present because initialize()
     * calls functions of other libraries that have the same requirements and thread safety.
     * terminate() must be called for each initialize() called.
     *
     * @param initParams The @c InitializationParameters.
     * @return Whether the initialization was successful.
     */
    static bool initialize(const std::shared_ptr<InitializationParameters>& initParams);

    /**
     * Uninitialize the Alexa Client SDK.
     *
     * You should call uninitialize() once for each call you make to initialize(), after you are done
     * using the Alexa Client SDK.
     *
     * This function must be called when no other threads in the process are running. this function
     * is not thread safe. This requirement is present because uninitialize() calls functions of other
     * libraries that have the same requirements and thread safety.
     */
    static void uninitialize();

private:
    /**
     * Cleanup resources activated during the initialization of the Alexa Client SDK.
     *
     * You should call cleanup() when resources must be released before exiting.
     *
     * This function must be called when no other threads in the process are running. this function
     * is not thread safe. This requirement is present because cleanup() calls functions of other
     * libraries that have the same requirements and thread safety.
     */
    static void cleanup();

    /// Tracks whether we've initialized the Alexa Client SDK or not
    static std::atomic_int g_isInitialized;
};

}  // namespace initialization
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_INITIALIZATION_ALEXACLIENTSDKINIT_H_
