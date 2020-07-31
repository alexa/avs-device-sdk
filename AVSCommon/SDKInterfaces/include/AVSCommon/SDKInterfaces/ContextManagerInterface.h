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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CONTEXTMANAGERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CONTEXTMANAGERINTERFACE_H_

#include <chrono>
#include <memory>
#include <string>

#include "AVSCommon/AVS/CapabilityTag.h"
#include "AVSCommon/AVS/StateRefreshPolicy.h"
#include "AVSCommon/SDKInterfaces/AlexaStateChangeCauseType.h"
#include "AVSCommon/SDKInterfaces/ContextManagerObserverInterface.h"
#include "AVSCommon/SDKInterfaces/ContextRequesterInterface.h"
#include "AVSCommon/SDKInterfaces/StateProviderInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This enum expresses the result of a @c setState operation.
 *
 * @deprecated The @c setState operation has been deprecated and so is this enumeration.
 */
enum class SetStateResult {
    /// @c setState request was successful.
    SUCCESS,

    /**
     * @c setState request failed because the @c StateProviderInterface is not registered with the @c ContextManager.
     */
    STATE_PROVIDER_NOT_REGISTERED,

    /**
     * @c setState request failed because the @c StateProviderInterface provided the wrong token to the @c
     * ContextManager.
     */
    STATE_TOKEN_OUTDATED
};

/**
 * Interface to get the context and set the state.
 * State refers to the client component's state. Context is a container used to communicate the state
 * of the client components to AVS.
 * @see https://developer.amazon.com/docs/alexa/alexa-voice-service/context.html.
 *
 * @note Implementations must be thread-safe.
 */
class ContextManagerInterface {
public:
    /**
     * Destructor.
     */
    virtual ~ContextManagerInterface() = default;

    /**
     * Registers a @c StateProviderInterface with the @c ContextManager. When the context manager receives a
     * @c getContext request, it queries the registered queryable @c StateProviderInterfaces for updated state.
     * If a @c StateProviderInterface tries to register a @c CapabilityMessageIdentifier that is already present, the
     * older @c StateProviderInterface will be replaced with the new one.
     *
     * @deprecated Use @c addStateProvider to add or replace the state provider and @c removeStateProvider to remove
     * a state provider.
     *
     * @note If a @c StateProviderInterface wants to unregister with the @c ContextManager, then it needs to set the
     * pointer to the @c StateProviderInterface to a @c nullptr.
     *
     * @param capabilityIdentifier The capability message identifier of the @c StateProviderInterface.
     * @param stateProvider The @c StateProviderInterface that will be mapped against the @c capabilityIdentifier.
     */
    virtual void setStateProvider(
        const avs::CapabilityTag& capabilityIdentifier,
        std::shared_ptr<StateProviderInterface> stateProvider) = 0;

    /**
     * Registers a @c StateProviderInterface with the @c ContextManager. When the context manager receives a
     * @c getContext request, it queries the registered @c StateProviderInterfaces for updated state, if needed.
     * If a @c StateProviderInterface tries to register a @c CapabilityMessageIdentifier that is already present, the
     * older @c StateProviderInterface will be replaced with the new one.
     *
     * @param capabilityIdentifier The capability message identifier of the @c StateProviderInterface.
     * @param stateProvider The @c StateProviderInterface that will be mapped against the @c capabilityIdentifier.
     */
    virtual void addStateProvider(
        const avsCommon::avs::CapabilityTag& capabilityIdentifier,
        std::shared_ptr<avsCommon::sdkInterfaces::StateProviderInterface> stateProvider) = 0;

    /**
     * Remove the state provider for the given capability with the @c ContextManager.
     *
     * @param capabilityIdentifier The capability message identifier of the @c StateProviderInterface.
     */
    virtual void removeStateProvider(const avs::CapabilityTag& capabilityIdentifier) = 0;

    /**
     * Sets the state information. The refresh policy indicates to the @c ContextManager whether on a @c getContext
     * request the state needs to be updated. If the @c refreshPolicy is @c ALWAYS, then the @c StateProviderInterface
     * needs to be registered with the @c ContextManager, else @c setState returns an error.
     *
     * For the states for which the refresh policy is @c ALWAYS, the @c ContextManager requests for @c provideStates
     * from the @c StateProviderInterfaces. When a @c provideState request is sent to a @c StateProviderInterface,
     * the @c ContextManager will provide a @c stateRequestToken. The same token needs to be sent on a @c setState in
     * response to the @c provideState. If the token sent in the @c setState request does not match the token in the
     * @c ContextManager, @c setState will return an error.
     *
     * @deprecated Use the provideStateResponse for responding to @c provideState request and @c reportStateChange to
     * proactively report to the ContextManager that the state has changed.
     *
     * @note Token needs to be set only if the @c setState is in response to a @c provideState request. Setting the
     * token to 0 is equivalent to no token.
     *
     * The @c jsonState is the json value that is associated with the key "payload".
     *
     * @param capabilityIdentifier The capability message identifier of the @c StateProviderInterface whose state is
     * being updated.
     * @param jsonState The state of the @c StateProviderInterface.  The @c StateProviderInterface with a @c
     * refreshPolicy of SOMETIMES can pass in an empty string to indicate no contexts needs to be sent by the provider.
     * @param refreshPolicy The refresh policy for the state.
     * @param stateRequestToken The token that was provided in a @c provideState request. Defaults to 0.
     *
     * @return The status of the setState operation.
     */
    virtual SetStateResult setState(
        const avs::CapabilityTag& capabilityIdentifier,
        const std::string& jsonState,
        const avs::StateRefreshPolicy& refreshPolicy,
        const ContextRequestToken stateRequestToken = 0) = 0;

    /**
     * Function used to proactively notify the context manager that the state of a capability has changed.
     *
     * @param capabilityIdentifier Identifies which capability has an updated state.
     * @param capabilityState The new state being reported.
     * @param cause The reason for the state change.
     */
    virtual void reportStateChange(
        const avs::CapabilityTag& capabilityIdentifier,
        const avs::CapabilityState& capabilityState,
        AlexaStateChangeCauseType cause) = 0;

    /**
     * Provides the capability state information as a response to @c provideState request.
     *
     * When a @c provideState request is sent to a @c StateProviderInterface, the @c ContextManager will
     * provide a @c stateRequestToken. The same token needs to be sent on a @c provideState in response to the @c
     * provideState. If the token sent in the @c provideState request does not match the token in the @c
     * ContextManager, the response will be dropped.
     *
     * @param capabilityIdentifier Identifies which capability state is being provided.
     * @param capabilityState The new state being provided.
     * @param stateRequestToken The token that was provided in a @c provideState request.
     */
    virtual void provideStateResponse(
        const avs::CapabilityTag& capabilityIdentifier,
        const avs::CapabilityState& capabilityState,
        ContextRequestToken stateRequestToken) = 0;

    /**
     * Response method used to inform that the capability state is not available.
     *
     * The same token needs to be sent on a @c setState in response to the @c provideState. If the token sent in the
     * @c setState request does not match the token in the @c ContextManager, @c setState will return an error.
     *
     * @param capabilityIdentifier The capability message identifier of the @c StateProviderInterface whose state is
     * not available.
     * @param stateRequestToken The token that was provided in a @c provideState request.
     * @param isEndpointUnreachable Whether the failure was due to the endpoint being unreachable.
     */
    virtual void provideStateUnavailableResponse(
        const avs::CapabilityTag& capabilityIdentifier,
        ContextRequestToken stateRequestToken,
        bool isEndpointUnreachable) = 0;

    /**
     * Request the @c ContextManager for context. If a request to the @c StateProviderInterfaces for updated states
     * is not in progress, then requests will be sent to the @c StateProviderInterfaces via the @c provideState
     * requests. If updated states have already been requested, this @ getContext request will be put on a queue and
     * updated when the head of queue's request arrives. Once updated states are available, the context requester is
     * informed via @c onContextAvailable. If any error is encountered while updating states, the context requester is
     * informed via @c onContextFailure with the details of the error.
     *
     * @note If you are using RequestToken to track the context response, make sure that the access is synchronized
     * with the @c onContextAvailable response.
     *
     * @param contextRequester The context requester asking for context.
     * @param endpointId The @c endpointId used to select which context is being requested.
     * @param timeout The maximum time this request should take. After the timeout, the context manager will abort the
     * request.
     * @return A token that can be used to correlate this request with the context response.
     *
     * @warning An empty endpointId will select the defaultEndpoint context for now. This argument will become
     * required in future versions of the SDK.
     */
    virtual ContextRequestToken getContext(
        std::shared_ptr<ContextRequesterInterface> contextRequester,
        const std::string& endpointId = "",
        const std::chrono::milliseconds& timeout = std::chrono::seconds(2)) = 0;

    /**
     * Request the @c ContextManager for context while skipping state from @c StateProviders which have reportable
     * state properties.
     *
     * @see https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/reportable-state-properties.html
     *
     * @note An example use case of when this method can be is used is to get the Context sent in
     * @c SpeechRecognizer.Recognize event. The reason being, sending all state information can make the context
     * bloated which might adversely effect user perceived latency. Additionally, state from reportable state properties
     * can be sent to the cloud either by using @c StateReport event or the @c ChangeReport event.
     *
     * @note This method is functionally similar to the getContext() method except that it skips state information of
     * reportable state properties.
     *
     * @note If you are using RequestToken to track the context response, make sure that the access is synchronized
     * with the @c onContextAvailable response.
     *
     * @param contextRequester The context requester asking for context.
     * @param endpointId The @c endpointId used to select which context is being requested.
     * @param timeout The maximum time this request should take. After the timeout, the context manager will abort the
     * request.
     * @return A token that can be used to correlate this request with the context response.
     *
     * @warning An empty endpointId will select the defaultEndpoint context for now. This argument will become
     * required in future versions of the SDK.
     */
    virtual ContextRequestToken getContextWithoutReportableStateProperties(
        std::shared_ptr<ContextRequesterInterface> contextRequester,
        const std::string& endpointId = "",
        const std::chrono::milliseconds& timeout = std::chrono::seconds(2)) = 0;

    /**
     * Adds an observer to be notified of context changes.
     *
     * @param observer The observer to add.
     */
    virtual void addContextManagerObserver(std::shared_ptr<ContextManagerObserverInterface> observer) = 0;

    /**
     * Removes an observer from being notified of context changes.
     *
     * @param observer The observer to remove.
     */
    virtual void removeContextManagerObserver(const std::shared_ptr<ContextManagerObserverInterface>& observer) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CONTEXTMANAGERINTERFACE_H_
