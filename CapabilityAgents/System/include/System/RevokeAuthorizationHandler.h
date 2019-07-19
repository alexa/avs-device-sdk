/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_REVOKEAUTHORIZATIONHANDLER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_REVOKEAUTHORIZATIONHANDLER_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/SDKInterfaces/RevokeAuthorizationObserverInterface.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

/**
 * This class implements a @c CapabilityAgent that handles the @c RevokeAuthorization directive.
 */
class RevokeAuthorizationHandler : public avsCommon::avs::CapabilityAgent {
public:
    /**
     * Create an instance of @c RevokeAuthorizationHandler.
     *
     * @param exceptionEncounteredSender The interface that sends exceptions.
     * @return @c nullptr if the inputs are not defined, else a new instance of @c RevokeAuthorizationHandler.
     */
    static std::shared_ptr<RevokeAuthorizationHandler> create(
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender);

    /// @name DirectiveHandlerInterface and CapabilityAgent Functions
    /// @{
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    /// @}

    /**
     * Adds an observer to be notified when the System.RevokeAuthorization Directive has been received.
     *
     * @param observer The observer to be notified when the System.RevokeAuthorization Directive has been received.
     * @return true if the observer was added, false if invalid observer or already an existing observer.
     */
    bool addObserver(std::shared_ptr<avsCommon::sdkInterfaces::RevokeAuthorizationObserverInterface> observer);

    /**
     * Removes an observer from the collection of observers which will be notified when the System.RevokeAuthorization
     * Directive has been received.
     *
     * @param observer The observer that should no longer be notified when the System.RevokeAuthorization Directive has
     * been sent.
     * @return true if the observer was removed, false if invalid observer or wasn't a registered observer.
     */
    bool removeObserver(std::shared_ptr<avsCommon::sdkInterfaces::RevokeAuthorizationObserverInterface> observer);

private:
    /**
     * Constructor.
     *
     * @param exceptionEncounteredSender The interface that sends exceptions.
     */
    RevokeAuthorizationHandler(
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

    /// Mutex to synchronize access to @c m_revokeObservers.
    std::mutex m_mutex;

    /// Observers to be notified when the System.RevokeAuthorization Directive has been received.
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::RevokeAuthorizationObserverInterface>>
        m_revokeObservers;
};

}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_REVOKEAUTHORIZATIONHANDLER_H_
