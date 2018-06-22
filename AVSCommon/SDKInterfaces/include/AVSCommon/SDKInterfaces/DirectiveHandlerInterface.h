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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_DIRECTIVEHANDLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_DIRECTIVEHANDLERINTERFACE_H_

#include <memory>

#include "AVSCommon/AVS/AVSDirective.h"
#include "AVSCommon/AVS/DirectiveHandlerConfiguration.h"
#include "AVSCommon/SDKInterfaces/DirectiveHandlerResultInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * Interface for handling @c AVSDirectives.  For each @c AVSDirective received, implementations of this interface
 * should expect either a single call to @c handleDirectiveImmediately() or a call to @c preHandleDirective()
 * followed by a call to @c handleDirective() unless @c cancelDirective() is called first.  @c cancelDirective()
 * may also be called after handleDirective().
 *
 * @note The implementation of the methods of this interface MUST be thread-safe.
 * @note The implementation of the methods of this interface MUST return quickly.  Failure to do so blocks
 * the processing of subsequent @c AVSDirectives.
 */
class DirectiveHandlerInterface {
public:
    /**
     * Destructor.
     */
    virtual ~DirectiveHandlerInterface() = default;

    /**
     * Handle the action specified @c AVSDirective. Once this has been called the @c DirectiveHandler should not
     * expect to receive further calls regarding this directive.
     *
     * @note The implementation of this method MUST be thread-safe.
     * @note The implementation of this method MUST return quickly. Failure to do so blocks the processing
     * of subsequent @c AVSDirectives.
     * @note If this operation fails, an @c ExceptionEncountered message should be sent to AVS.
     *
     * @param directive The directive to handle.
     */
    virtual void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) = 0;

    /**
     * Notification that a directive has arrived.  This notification gives the DirectiveHandler a chance to
     * prepare for handling the directive.  For example, the @c DirectiveHandler might use this notification
     * to start downloading an asset that will be required when it becomes time to actually handle the directive.
     * The @c DirectiveHandler will be notified when it should start the actual handling of the directive by a
     * subsequent call to handleDirective().  If an error occurs during the pre-Handling phase and that error
     * should cancel the handling of subsequent @c AVSDirectives with the same @c DialogRequestId, the
     * @c DirectiveHandler should call the setFailed() method on the @c result instance passed in to this call.
     *
     * @note If this operation fails, an @c ExceptionEncountered message should be sent to AVS.
     * @note The implementation of this method MUST be thread-safe.
     * @note The implementation of this method MUST return quickly. Failure to do so blocks the processing
     * of subsequent @c AVSDirectives.
     *
     * @param directive The directive to pre-handle.
     * @param result An object to receive the result of the operation.
     */
    virtual void preHandleDirective(
        std::shared_ptr<avsCommon::avs::AVSDirective> directive,
        std::unique_ptr<DirectiveHandlerResultInterface> result) = 0;

    /**
     * Handle the action specified by the directive identified by @c messageId. The handling of subsequent directives
     * with the same @c DialogRequestId may be blocked until the @c DirectiveHandler calls the @c setSucceeded()
     * method of the @c DirectiveHandlingResult instance passed in to the @c preHandleDirective() call for the
     * directive specified by @c messageId.  If handling of this directive fails such that subsequent directives with
     * the same @c DialogRequestId should be cancelled, this @c DirectiveHandler should instead call setFailed() to
     * indicate a failure.
     *
     * @note If this operation fails, an @c ExceptionEncountered message should be sent to AVS.
     * @note The implementation of this method MUST be thread-safe.
     * @note The implementation of this method MUST return quickly. Failure to do so blocks the processing
     * of subsequent @c AVSDirectives.
     *
     * @param messageId The message ID of a directive previously passed to @c preHandleDirective().
     * @return @c false when @c messageId is not recognized, else @c true.  Any errors related to handling of a valid
     * messageId should be reported using @c DirectiveHandlerResultInterface::setFailed().
     */
    virtual bool handleDirective(const std::string& messageId) = 0;

    /**
     * Cancel an ongoing @c preHandleDirective() or @c handleDirective() operation for the specified @c AVSDirective.
     * Once this has been called the @c DirectiveHandler should not expect to receive further calls regarding this
     * directive.
     *
     * @note The implementation of this method MUST be thread-safe.
     * @note The implementation of this method MUST return quickly. Failure to do so blocks the processing
     * of subsequent @c AVSDirectives.
     *
     * @param messageId The message ID of a directive previously passed to preHandleDirective().
     */
    virtual void cancelDirective(const std::string& messageId) = 0;

    /**
     * Notification that this handler has been de-registered and will not receive any more calls.
     */
    virtual void onDeregistered() = 0;

    /**
     * Returns the configuration of the directive handler regarding which namespace and name pairs it should handle and
     * the appropriate policy for each pair.
     *
     * @return The @c avs::DirectiveHandlerConfiguration of the handler.
     */
    virtual avs::DirectiveHandlerConfiguration getConfiguration() const = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_DIRECTIVEHANDLERINTERFACE_H_
