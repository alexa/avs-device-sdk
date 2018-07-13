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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CONTEXTMANAGERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CONTEXTMANAGERINTERFACE_H_

#include <memory>

#include "AVSCommon/SDKInterfaces/ContextRequesterInterface.h"
#include "AVSCommon/SDKInterfaces/StateProviderInterface.h"
#include "AVSCommon/AVS/StateRefreshPolicy.h"
#include "AVSCommon/AVS/NamespaceAndName.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This enum expresses the result of a @c setState operation.
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
 * @see https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/context.
 */
class ContextManagerInterface {
public:
    /**
     * Destructor.
     */
    virtual ~ContextManagerInterface() = default;

    /**
     * Registers a @c StateProviderInterface with the @c ContextManager. When the context manager receives a
     * @c getContext request, it queries the registered @c StateProviderInterfaces for updated state, if needed.
     * If a @c StateProviderInterface tries to register a @c NamespaceAndName that is already present, the older
     * @c StateProviderInterface will be replaced with the new one.
     * If a @c StateProviderInterface wants to unregister with the @c ContextManager, then it needs to set the
     * pointer to the @c StateProviderInterface to a @c nullptr.
     *
     * @param namespaceAndName The namespace and name of the @c StateProviderInterface.
     * @param stateProvider The @c StateProviderInterface that will be mapped against the @c namespaceAndName.
     */
    virtual void setStateProvider(
        const avs::NamespaceAndName& namespaceAndName,
        std::shared_ptr<StateProviderInterface> stateProvider) = 0;

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
     * @note Token needs to be set only if the @c setState is in response to a @c provideState request. Setting the
     * token to 0 is equivalent to no token.
     *
     * The @c jsonState is the json value that is associated with the key "payload".
     *
     * @param namespaceAndName The namespace and name of the @c StateProviderInterface whose state is being updated.
     * @param jsonState The state of the @c StateProviderInterface.  The @c StateProviderInterface with a @c
     * refreshPolicy of SOMETIMES can pass in an empty string to indicate no contexts needs to be sent by the provider.
     * @param refreshPolicy The refresh policy for the state.
     * @param stateRequestToken The token that was provided in a @c provideState request. Defaults to 0.
     *
     * @return The status of the setState operation.
     */
    virtual SetStateResult setState(
        const avs::NamespaceAndName& namespaceAndName,
        const std::string& jsonState,
        const avs::StateRefreshPolicy& refreshPolicy,
        const unsigned int stateRequestToken = 0) = 0;

    /**
     * Request the @c ContextManager for context. If a request to the @c StateProviderInterfaces for updated states
     * is not in progress, then requests will be sent to the @c StateProviderInterfaces via the @c provideState
     * requests. If updated states have already been requested, this @ getContext request will be put on a queue and
     * updated when the head of queue's request arrives. Once updated states are available, the context requester is
     * informed via @c onContextAvailable. If any error is encountered while updating states, the context requester is
     * informed via @c onContextFailure with the details of the error.
     *
     * @param contextRequester The context requester asking for context.
     */
    virtual void getContext(std::shared_ptr<ContextRequesterInterface> contextRequester) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CONTEXTMANAGERINTERFACE_H_
