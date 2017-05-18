/*
 * CapabilityAgent.h
 *
 * Copyright (c) 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVS_COMMON_AVS_INCLUDE_AVS_COMMON_AVS_CAPABILITY_AGENT_H
#define ALEXA_CLIENT_SDK_AVS_COMMON_AVS_INCLUDE_AVS_COMMON_AVS_CAPABILITY_AGENT_H

#include <unordered_map>
#include <string>
#include <memory>

#include "AVSCommon/AVS/NamespaceAndName.h"
#include "AVSCommon/SDKInterfaces/ChannelObserverInterface.h"
#include "AVSCommon/SDKInterfaces/ContextRequesterInterface.h"
#include "AVSCommon/SDKInterfaces/StateProviderInterface.h"
#include "AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h"
#include "AVSCommon/SDKInterfaces/DirectiveHandlerResultInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/**
* @c CapabilityAgent implements methods which most capability agents will need, namely:
* @li @c DirectiveHandlerInterface,
* @li Building the JSON event string given the name, payload and context,
* @li A map of the message Id to @c AVSDirective and @c DirectiveResultInterface.
* Derived capability agents may extend this class. They may have to implement the following interfaces:
* @li @c ChannelObserverInterface: To use the Activity Focus Manager Library,
* @li @c StateProviderInterface: To provide state to the @c ContextManager.
* @li @c ContextRequesterInterface: To request context from the @c ContextManager,
* as necessary.
*/
class CapabilityAgent
        : public sdkInterfaces::DirectiveHandlerInterface, public sdkInterfaces::ChannelObserverInterface,
          public sdkInterfaces::StateProviderInterface, public sdkInterfaces::ContextRequesterInterface {
public:
    /**
     * Destructor.
     */
    virtual ~CapabilityAgent();

    /*
     * DirectiveHandlerInterface functions.
     *
     * The following four functions implement the @c DirectiveHandlerInterface. Only the message Id is passed to the
     * @c handleDirective and @c cancelDirective functions, so we need to maintain a mapping of the message Id to the
     * @c AVSDirective and @c DirectiveHandlerResultInterface, i.e., @c DirectiveAndResultInterface. The
     * @c DirectiveHandlerInterface functions call the functions of the same name with the
     * @c DirectiveAndResultInterface as the argument.
     */
    void handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) override final;

    void preHandleDirective(std::shared_ptr<AVSDirective> directive,
                            std::unique_ptr<sdkInterfaces::DirectiveHandlerResultInterface> result) override final;

    bool handleDirective(const std::string &messageId) override final;

    void cancelDirective(const std::string &messageId) override final;

    void onDeregistered() override;

    void onFocusChanged(sdkInterfaces::FocusState newFocus) override;

    void provideState(const unsigned int stateRequestToken) override;

    void onContextAvailable(const std::string &jsonContext) override;

    void onContextFailure(const sdkInterfaces::ContextRequestError error) override;

protected:
    /**
     * Constructor for a Capability Agent.
     *
     * @param nameSpace The namespace of the capability agent.
     */
    CapabilityAgent(const std::string &nameSpace);

    /**
     *  Contains a directive and handling result for use in mapping to a message id.
     */
    struct DirectiveAndResultInterface {
        /**
         * This function checks whether this is a valid @c DirectiveAndResultInterface.  A
         * @c DirectiveAndResultInterface is valid if it has a valid @c directive and a valid @c resultInterface.
         *
         * @return @c true if @c directive is not @c nullptr and @c resultInterface is not @c nullptr, else @c false.
         */
        operator bool() const;

        /// @c AVSDirective that is passed during preHandle.
        std::shared_ptr<AVSDirective> directive;

        /// @c DirectiveHandlerResultInterface.
        std::shared_ptr<sdkInterfaces::DirectiveHandlerResultInterface> resultInterface;
    };

    /**
      * Handle the action specified in @c AVSDirective. Once this has been called the @c DirectiveHandler should not
      * expect to receive further calls regarding this directive.
      *
      * @note The implementation of this method MUST be thread-safe.
      * @note The implementation of this method MUST return quickly. Failure to do so blocks the processing
      * of subsequent @c AVSDirectives.
      * @note The @c DirectiveHandlerResultInterface @c resultInterface of the @c directiveAndResultInterface
      * will be set to @c nullptr
      *
      * @param directiveAndResultInterface The @c DirectiveAndResultInterface containing the @c AVSDirective to
      *     process.
      */
    virtual void handleDirectiveImmediately(const DirectiveAndResultInterface &directiveAndResultInterface) = 0;

    /**
     * Notification that a directive has arrived.  This notification gives the DirectiveHandler a chance to
     * prepare for handling the @c AVSDirective present in the @c DirectiveAndResultInterface.
     * If an error occurs during the pre-Handling phase and that error should cancel the handling of subsequent
     * @c AVSDirectives with the same @c DialogRequestId, the @c DirectiveHandler should call the @c setFailed method
     * on the @c result instance passed in to this call.
     *
     * @note The implementation of this method MUST be thread-safe.
     * @note The implementation of this method MUST return quickly. Failure to do so blocks the processing
     * of subsequent @c AVSDirectives.
     *
     * @param directiveAndResultInterface The @c DirectiveAndResultInterface containing the @c AVSDirective and the
     * @c DirectiveHandlerResultInterface.
     */
    virtual void preHandleDirective(const DirectiveAndResultInterface &directiveAndResultInterface) = 0;

    /**
     * Handle the action specified by the directive in @c DirectiveAndResultInterface. The handling of subsequent
     * directives with the same @c DialogRequestId may be blocked until the @c DirectiveHandler calls the
     * @c setSucceeded() method of the @c DirectiveHandlingResult present in the @c DirectiveAndResultInterface.
     * If handling of this directive fails @c setFailed should be called to indicate a failure.
     *
     * @note The implementation of this method MUST be thread-safe.
     * @note The implementation of this method MUST return quickly. Failure to do so blocks the processing
     * of subsequent @c AVSDirectives.
     *
     * @param directiveAndResultInterface The @c DirectiveAndResultInterface containing the @c AVSDirective and the
     * @c DirectiveHandlerResultInterface.
     */
    virtual void handleDirective(const DirectiveAndResultInterface &directiveAndResultInterface) = 0;

    /**
     * Cancel an ongoing @c preHandleDirective or @c handleDirective operation for the specified @c AVSDirective. Once
     * this has been called the @c DirectiveHandler should not expect to receive further calls regarding this
     * directive.
     *
     * @note The implementation of this method MUST be thread-safe.
     * @note The implementation of this method MUST return quickly. Failure to do so blocks the processing
     * of subsequent @c AVSDirectives.
     *
     * @param directiveAndResultInterface The @c DirectiveAndResultInterface containing the @c AVSDirective and the
     * @c DirectiveHandlerResultInterface.
     */
    virtual void cancelDirective(const DirectiveAndResultInterface &directiveAndResultInterface) = 0;

    /**
     * This function releases resources associated with the @c AVSDirective which is no longer in use by a
     * @c CapabilityAgent.
     *
     * @note This function should be called at from @c handleDirective() and @c cancelDirective()
     * implementations after the status of @c handleDirective or @c cancelDirective() has been reported to the
     * directive sequencer.
     *
     * @param messageId The message Id of the @c AVSDirective.
     */
    void removeDirective(const std::string &messageId);

    /**
     * Builds a JSON event string which includes the header, the @c payload and an optional @c context.
     * The header includes the namespace, name, message Id and an optional @c dialogRequestId.
     * The message Id required for the header is a random string that is generated and added to the
     * header.
     *
     * @param eventName The name of the event to be include in the header.
     * @param dialogRequestIdString The value associated with the "dialogRequestId" key.
     * @param payload The payload value associated with the "payload" key.
     * @param context Optional @c context to be sent with the event message.
     * @return An event JSON string if successful else an empty string.
     */
    const std::string buildJsonEventString(const std::string &eventName,
                                           const std::string &dialogRequestIdString = "",
                                           const std::string &payload = "{}", const std::string &context = "");

    /// The namespace of the capability agent.
    const std::string m_namespace;

private:
    /**
     * Returns a @c DirectiveAndResultInterface object for the provided @c messageId.
     *
     * If @c directive and @c result are provided (both @c != @c nullptr), this function tries to add a new entry for
     * @c messageId to the map and returns it.  If an entry already exists, an error is logged and and an invalid
     * @c DirectiveAndResultInterface is returned.
     *
     * If @c directive and @c result are not provided (both @c == @c nullptr), this function looks for an existing
     * entry for @c messageId in the map.  If an entry exists, it is returned.  If an entry does not exist, an error
     * is logged and an invalid @c DirectiveAndResultInterface is returned.
     *
     * @param messageId The message id of a @c AVSDirective.
     * @param directive @c AVSDirective that is passed during @c preHandle().  This parameter is optional and defaults
     *     @c nullptr.
     * @param result @c DirectiveHandlerResultInterface that is passed during @c preHandle().  This parameter is
     *     optional and defaults to @c nullptr.
     * @return the @c the DirectiveAndResultInterface if a associated with the messageId else an invalid
     *     @c DirectiveAndResultInterface.
     */
    DirectiveAndResultInterface getDirectiveAndResult(
        const std::string& messageId,
        std::shared_ptr<avsCommon::AVSDirective> directive = nullptr,
        std::unique_ptr<sdkInterfaces::DirectiveHandlerResultInterface> result = nullptr);

    /// Map of message Id to @c DirectiveAndResultInterface.
    std::unordered_map <std::string, DirectiveAndResultInterface> m_directiveAndResultInterfaceMap;

    /// Mutex to protect message Id to @c DirectiveAndResultInterface mapping.
    std::mutex m_mutex;
};

} // namespace avs
} // namespace avsCommon
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_AVS_COMMON_AVS_INCLUDE_AVS_COMMON_AVS_CAPABILITY_AGENT_H
