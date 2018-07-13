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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_ENDPOINTHANDLER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_ENDPOINTHANDLER_H_

#include <memory>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/SDKInterfaces/AVSEndpointAssignerInterface.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

/**
 * This class implementes a @c CapabilityAgent that handles the @c SetEndpoint directive.
 */
class EndpointHandler : public avsCommon::avs::CapabilityAgent {
public:
    /**
     * Create an instance of @c EndpointHandler.
     *
     * @param avsEndpointAssigner The interface to be notified when a new @c SetEndpoint directive comes.
     * @param exceptionEncounteredSender The interface that sends exceptions.
     * @return @c nullptr if the inputs are not defined, else a new instance of @c EndpointHandler.
     */
    static std::shared_ptr<EndpointHandler> create(
        std::shared_ptr<avsCommon::sdkInterfaces::AVSEndpointAssignerInterface> avsEndpointAssigner,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender);

    /// @name DirectiveHandlerInterface and CapabilityAgent Functions
    /// @{
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param avsEndpointAssigner The interface to be notified when a new @c SetEndpoint directive comes.
     * @param exceptionEncounteredSender The interface that sends exceptions.
     */
    EndpointHandler(
        std::shared_ptr<avsCommon::sdkInterfaces::AVSEndpointAssignerInterface> avsEndpointAssigner,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender);

    /**
     * Remove the directive (if possible) while invoking callbacks to @c DirectiveHandlerResultInterface.
     *
     * @param info The @c DirectiveInfo we are trying to remove.
     * @param isFailure Boolean flag set to @c true if something went wrong before removing the directive.
     * @param report The report that we will pass to @c setFailed in case @c isFailure is @c true.
     */
    void removeDirectiveGracefully(
        std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
        bool isFailure = false,
        const std::string& report = "");

    /// The @c AVSEndpointAssignerInterface used to signal endpoint change.
    std::shared_ptr<avsCommon::sdkInterfaces::AVSEndpointAssignerInterface> m_avsEndpointAssigner;
};

}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_ENDPOINTHANDLER_H_
