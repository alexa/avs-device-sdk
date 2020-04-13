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

/**
 * @file
 */
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CONTEXTREQUESTERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CONTEXTREQUESTERINTERFACE_H_

#include <cstdint>
#include <string>
#include <ostream>

#include <AVSCommon/AVS/AVSContext.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Logger/LogEntry.h>

#include "AVSCommon/SDKInterfaces/ContextRequestToken.h"
#include "AVSCommon/SDKInterfaces/Endpoints/EndpointIdentifier.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This enum expresses the result of a getContext operation.
 */
enum class ContextRequestError {
    /// @c getContext request failed due to a timeout on one or more @c stateProviderInterfaces.
    STATE_PROVIDER_TIMEDOUT,

    /// @c getContext request failed due to an error building the context.
    BUILD_CONTEXT_ERROR,

    /// @c getContext request failed due to an error while trying to reach out to the endpoint.
    ENDPOINT_UNREACHABLE
};

/**
 * This specifies the interface to a contextRequester.
 */
class ContextRequesterInterface {
public:
    /**
     * Destructor.
     */
    virtual ~ContextRequesterInterface() = default;

    /**
     * This is called by the ContextManager once the context is ready and available.
     *
     * @deprecated This method is being deprecated since it cannot specify the source endpoint.
     *
     * @note The @c ContextRequester should perform minimum processing and return quickly. Otherwise it will block
     * the processing of updating the of other @c ContextProviders.
     *
     * @param jsonContext Context information.Context provided is of the format {"context"[{...}, {...}]}
     */
    virtual void onContextAvailable(const std::string& jsonContext);

    /**
     * This is called by the ContextManager once the context is ready and available.
     *
     * @note The @c ContextRequester should perform minimum processing and return quickly. Otherwise it will block
     * the processing of updating the of other @c ContextProviders.
     *
     * @note In future versions, this method will be made pure virtual.
     *
     * @param endpointId The ID used to identify the endpoint to which this context belong.
     * @param endpointContext The state of all capabilities related to the given endpoint.
     * @param requestToken Token used to identify a specific context request.
     */
    virtual void onContextAvailable(
        const endpoints::EndpointIdentifier& endpointId,
        const avs::AVSContext& endpointContext,
        ContextRequestToken requestToken);

    /**
     * The contextManager calls this if it is unable to process a @c getContext request successfully.
     *
     * @deprecated This method is being deprecated since it cannot specify the optional token.
     *
     * @note The @c ContextRequester should perform minimum processing and return quickly. Otherwise it will block
     * the processing of updating the of other @c ContextProviders.
     *
     * @param error The reason why the getContext request failed.
     */
    virtual void onContextFailure(const ContextRequestError error);

    /**
     * The contextManager calls this if it is unable to process a @c getContext request successfully.
     *
     * @note The @c ContextRequester should perform minimum processing and return quickly. Otherwise it will block
     * the processing of updating the of other @c ContextProviders.
     * @note In future versions, this method will be made pure virtual.
     *
     * @param error The reason why the getContext request failed.
     * @param requestToken Token used to identify a specific context request.
     */
    virtual void onContextFailure(const ContextRequestError error, ContextRequestToken token);
};

/**
 * Write a @c ContextRequestError value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param error The error value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const ContextRequestError& error) {
    switch (error) {
        case ContextRequestError::STATE_PROVIDER_TIMEDOUT:
            stream << "STATE_PROVIDER_TIMEDOUT";
            break;
        case ContextRequestError::BUILD_CONTEXT_ERROR:
            stream << "BUILD_CONTEXT_ERROR";
            break;
        case ContextRequestError::ENDPOINT_UNREACHABLE:
            stream << "ENDPOINT_UNREACHABLE";
            break;
    }
    return stream;
}

inline void ContextRequesterInterface::onContextAvailable(const std::string& jsonContext) {
    utils::logger::acsdkError(
        utils::logger::LogEntry("ContextRequesterInterface", __func__).d("reason", "methodDeprecated"));
}

inline void ContextRequesterInterface::onContextAvailable(
    const endpoints::EndpointIdentifier& endpointId,
    const avs::AVSContext& endpointContext,
    ContextRequestToken requestToken) {
    onContextAvailable(endpointContext.toJson());
}

inline void ContextRequesterInterface::onContextFailure(const ContextRequestError error) {
    utils::logger::acsdkError(
        utils::logger::LogEntry("ContextRequesterInterface", __func__).d("reason", "methodDeprecated"));
}

inline void ContextRequesterInterface::onContextFailure(const ContextRequestError error, ContextRequestToken token) {
    onContextFailure(error);
}

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_CONTEXTREQUESTERINTERFACE_H_
