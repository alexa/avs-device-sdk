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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_STATEPROVIDERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_STATEPROVIDERINTERFACE_H_

#include "AVSCommon/SDKInterfaces/ContextRequestToken.h"

#include <AVSCommon/AVS/CapabilityTag.h>
#include <AVSCommon/AVS/NamespaceAndName.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * A @c StateProvider may be any client component whose state needs to be sent to the AVS.
 * This specifies the interface to a @c StateProvider.
 */
class StateProviderInterface {
public:
    /**
     * Destructor.
     */
    virtual ~StateProviderInterface() = default;

    /**
     * A request to a @c StateProvider to provide the state. The @c StateProvider should perform minimum processing
     * and return quickly, otherwise it will block the processing of updating the states of other @c StateProviders.
     * The @c ContextManager specifies a token which it uses to track the @c getContext request associated with this
     * @c provideState request. The @c stateProviderInterface must use the same token when it updates its state via the
     * @c setState call.
     *
     * @Note: The setState method MUST be called from a different thread from where the provideState method is being
     * called from.
     *
     * @param stateProviderName The name of the state provider.
     * @param stateRequestToken The token to use in the @c setState call.
     * @deprecated @c NamespaceAndName is being deprecated. Use the CapabilityMessageIdentifier version instead.
     */
    virtual void provideState(
        const avs::NamespaceAndName& stateProviderName,
        const ContextRequestToken stateRequestToken);

    /**
     * A request to a @c StateProvider to provide the state. The @c StateProvider should perform minimum processing
     * and return quickly, otherwise it will block the processing of updating the states of other @c StateProviders.
     * The @c ContextManager specifies a token which it uses to track the @c getContext request associated with this
     * @c provideState request. The @c stateProviderInterface must use the same token when it updates its state via the
     * @c setState call.
     *
     * @Note: The setState method MUST be called from a different thread from where the provideState method is being
     * called from.
     *
     * @param stateProviderIdentifier The identifier of the state provider.
     * @param stateRequestToken The token to use in the @c setState call.
     *
     * @note In future versions, this method will be made pure virtual.
     */
    virtual void provideState(const avs::CapabilityTag& stateProviderName, const ContextRequestToken stateRequestToken);

    /**
     * Returns whether the provider can be queried for its state / properties.
     * If not, the provider is omitted from the context altogether. ContextManager will not query or report its state.
     *
     * @return Whether this provider can be queried about its state when a new context request arrives.
     * @note In future versions, this method will be made pure virtual.
     */
    virtual bool canStateBeRetrieved();

    /**
     * Returns whether the provider has Reportable State Properties.
     * @see https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/reportable-state-properties.html
     *
     * @return Whether this provider has reportable state properties.
     */
    virtual bool hasReportableStateProperties();

    /**
     * Returns whether the provider should be queried for its state / properties.
     * If this returns false the last cached state will be reported to the context requester.
     *
     * @return whether the provider should be queried for its state / properties.
     */
    virtual bool shouldQueryState();
};

inline void StateProviderInterface::provideState(
    const avs::NamespaceAndName& stateProviderName,
    const ContextRequestToken stateRequestToken) {
    utils::logger::acsdkError(
        utils::logger::LogEntry("ContextRequesterInterface", __func__).d("reason", "methodDeprecated"));
}

inline void StateProviderInterface::provideState(
    const avs::CapabilityTag& stateProviderName,
    const ContextRequestToken stateRequestToken) {
    provideState(avs::NamespaceAndName(stateProviderName), stateRequestToken);
}

inline bool StateProviderInterface::canStateBeRetrieved() {
    return true;
}

inline bool StateProviderInterface::hasReportableStateProperties() {
    return false;
}

inline bool StateProviderInterface::shouldQueryState() {
    return true;
}

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_STATEPROVIDERINTERFACE_H_
