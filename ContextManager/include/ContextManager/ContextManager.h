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

#ifndef ALEXA_CLIENT_SDK_CONTEXTMANAGER_INCLUDE_CONTEXTMANAGER_CONTEXTMANAGER_H_
#define ALEXA_CLIENT_SDK_CONTEXTMANAGER_INCLUDE_CONTEXTMANAGER_CONTEXTMANAGER_H_

#include <memory>
#include <chrono>
#include <queue>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <unordered_map>
#include <condition_variable>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ContextRequesterInterface.h>
#include <AVSCommon/SDKInterfaces/StateProviderInterface.h>
#include <AVSCommon/AVS/StateRefreshPolicy.h>
#include <AVSCommon/AVS/NamespaceAndName.h>

namespace alexaClientSDK {
namespace contextManager {

/**
 * Class manages the requests for getting context from @c ContextRequesters and updating the state from
 * @c StateProviders.
 */
class ContextManager : public avsCommon::sdkInterfaces::ContextManagerInterface {
public:
    /**
     * Create a new @c ContextManager instance.
     *
     * @return Returns a new @c ContextManager.
     */
    static std::shared_ptr<ContextManager> create();

    /// Destructor.
    ~ContextManager() override;

    void setStateProvider(
        const avsCommon::avs::NamespaceAndName& stateProviderName,
        std::shared_ptr<avsCommon::sdkInterfaces::StateProviderInterface> stateProvider) override;

    avsCommon::sdkInterfaces::SetStateResult setState(
        const avsCommon::avs::NamespaceAndName& stateProviderName,
        const std::string& jsonState,
        const avsCommon::avs::StateRefreshPolicy& refreshPolicy,
        const unsigned int stateRequestToken = 0) override;

    void getContext(std::shared_ptr<avsCommon::sdkInterfaces::ContextRequesterInterface> contextRequester) override;

private:
    /**
     * This class has all the information about a @c StateProviderInterface needed by the contextManager.
     */
    struct StateInfo {
        /// Pointer to the StateProvider.
        std::shared_ptr<avsCommon::sdkInterfaces::StateProviderInterface> stateProvider;

        /// The state of the @c StateProviderInterface.
        std::string jsonState;

        /// RefreshPolicy for the state of a @c StateProviderInterface.
        avsCommon::avs::StateRefreshPolicy refreshPolicy;

        /**
         * Constructor.
         *
         * @param initStateProvider The @c StateProviderInterface.
         * @param initJsonState The state of the @c StateProviderInterface.
         * @param refreshPolicy The refresh policy of the state of the @c StateProviderInterface.
         */
        StateInfo(
            std::shared_ptr<avsCommon::sdkInterfaces::StateProviderInterface> initStateProvider = nullptr,
            std::string initJsonState = "",
            avsCommon::avs::StateRefreshPolicy initRefreshPolicy = avsCommon::avs::StateRefreshPolicy::ALWAYS);
    };

    /// Constructor.
    ContextManager();

    /**
     * Initialize a new instance of @c ContextManager.
     */
    void init();

    /**
     * Updates the state and refresh policy of the @c StateProviderInterface. The @c m_stateProviderMutex needs to be
     * acquired before this function is called.
     *
     * @param stateProviderName The name of the @c StateProviderInterface whose state is being updated.
     * @param jsonState The state of the @c StateProviderInterface.
     * @param refreshPolicy The refresh policy for the state.
     */
    avsCommon::sdkInterfaces::SetStateResult updateStateLocked(
        const avsCommon::avs::NamespaceAndName& stateProviderName,
        const std::string& jsonState,
        const avsCommon::avs::StateRefreshPolicy& refreshPolicy);

    /**
     * Requests the @c StateProviderInterfaces for state based on the refreshPolicy.
     *
     * @param stateProviderLock The lock acquired on the @c m_stateProviderMutex.
     */
    void requestStatesLocked(std::unique_lock<std::mutex>& stateProviderLock);

    /**
     * Sends the context to all @c ContextRequesterInterfaces in the queue. It sends failure to all the
     * @c ContextRequesterInterfaces if an error was encountered while updating the states or building the context.
     * It removes the @c ContextRequesterInterface from the queue after sending context or failure.
     *
     * @param context The context JSON string. This is an empty string if a failure needs to be reported.
     * @param contextRequestError The error to send to the context requesters. If the context is not an empty string,
     * this field is ignored.
     */
    void sendContextAndClearQueue(
        const std::string& context,
        const avsCommon::sdkInterfaces::ContextRequestError& contextRequestError =
            avsCommon::sdkInterfaces::ContextRequestError::BUILD_CONTEXT_ERROR);

    /**
     * The thread method that initiates @c provideState requests to the @c stateProviderInterfaces based on the
     * refreshPolicy. It also initiates the building of the context once the states have been updated.
     */
    void updateStatesLoop();

    /**
     * Builds the context header.
     *
     * @param namespaceAndName Namespace and name of the state provider.
     * @param allocator The rapidjson allocator to use to build the JSON header.
     * @return Header as a JSON object if successful else empty value.
     */
    rapidjson::Value buildHeader(
        const avsCommon::avs::NamespaceAndName& namespaceAndName,
        rapidjson::Document::AllocatorType& allocator);

    /**
     * Builds a JSON state object. The state includes the header and the payload.
     *
     * @param namespaceAndName Namespace and name of the state provider.
     * @param jsonPayloadValue The payload value associated with the "payload" key.
     * @param allocator The rapidjson allocator to use to build the JSON header.
     * @return A state object if successful else empty value.
     */
    rapidjson::Value buildState(
        const avsCommon::avs::NamespaceAndName& namespaceAndName,
        const std::string jsonPayloadValue,
        rapidjson::Document::AllocatorType& allocator);

    /**
     * Builds the context states from a map of state provider namespace and name to state information provided by each
     * of the @c stateProviderInterfaces and sends the context by calling @c onContextAvailable for each of the context
     * requesters.
     */
    void sendContextToRequesters();

    /**
     * Map of state provider namespace and name to the state information. @c m_stateProviderMutex must be acquired
     * before accessing the map.
     */
    std::unordered_map<avsCommon::avs::NamespaceAndName, std::shared_ptr<StateInfo>> m_namespaceNameToStateInfo;

    /// Queue of contextRequesters. @c m_contextRequesterMutex must be acquired before accessing the queue.
    std::queue<std::shared_ptr<avsCommon::sdkInterfaces::ContextRequesterInterface>> m_contextRequesterQueue;

    /**
     * Maintains a set of namespace and name of the the state providers to whom a @c provideState request has been sent.
     * @c m_stateProviderMutex must be acquired before modifying the set.
     */
    std::unordered_set<avsCommon::avs::NamespaceAndName> m_pendingOnStateProviders;

    /// Mutex to manage writes and reads to and from @c m_namespaceNameToStateInfo.
    std::mutex m_stateProviderMutex;

    /// Mutex to manage the writes and reads to and from the @c m_contextRequesterQueue.
    std::mutex m_contextRequesterMutex;

    /// Thread to request the state updates and request for building the context once the states are available.
    std::thread m_updateStatesThread;

    /**
     * Condition variable used to notify when all the expected @c setState requests in response to @c provideState
     * are complete. @c m_stateProviderMutex must be acquired accessing this variable.
     */
    std::condition_variable m_setStateCompleteNotifier;

    /**
     * Condition variable used to notify when a @c getContext request comes to the @c ContextManager.
     * @c m_contextRequesterMutex must be acquired accessing this variable.
     */
    std::condition_variable m_contextLoopNotifier;

    /*
     * Token to recognize the which @c getContext request the @c setState updates are being provided for.
     * The value is updated before @c provideState request is issued to the @c StateProviderInterfaces. The
     * @c m_stateProviderMutex must be acquired before modifying or reading the value.
     */
    unsigned int m_stateRequestToken;

    /*
     * Whether the contextManager is shutting down. The @c m_contextRequesterMutex is acquired before this value is
     * modified or read.
     */
    bool m_shutdown;
};

}  // namespace contextManager
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CONTEXTMANAGER_INCLUDE_CONTEXTMANAGER_CONTEXTMANAGER_H_
