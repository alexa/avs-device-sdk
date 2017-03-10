/*
 * DirectiveHandlerInterface.h
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_DIRECTIVE_HANDLER_INTERFACE_H_
#define ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_DIRECTIVE_HANDLER_INTERFACE_H_

#include <memory>

#include <AVSCommon/AVSDirective.h>
#include "ADSL/DirectiveHandlerResultInterface.h"

namespace alexaClientSDK {
namespace adsl {

/**
 * Interface for handling @c AVSDirectives.  Implementations of this interface should expect either a single
 * handleDirectiveImmediately() all or a call to preHandleDirective() followed by a call to handleDirective()
 * unless cancelDirective() is called first.  cancelDirective() may also be called after handleDirective().
 *
 * NOTE: The implementation of the methods of this interface MUST be thread-safe.
 * NOTE: The implementation of the methods of this interface MUST return quickly.  Failure to do so blocks
 * the processing of subsequent @c AVSDirectives.
 */
class DirectiveHandlerInterface {
public:
    /**
     * Virtual destructor to ensure proper cleanup by derived types.
     */
    virtual ~DirectiveHandlerInterface() = default;

    /**
     * Handle the action specified @c AVSDirective. Once this has been called the @c DirectiveHandler should not
     * expect to receive further calls regarding this directive.
     *
     * NOTE: The implementation of this method MUST be thread-safe.
     * NOTE: The implementation of this method MUST return quickly. Failure to do so blocks the processing
     * of subsequent @c AVSDirectives.
     *
     * @param directive The directive to handle.
     */
    virtual void handleDirectiveImmediately(std::shared_ptr<avsCommon::AVSDirective> directive) = 0;

    /**
     * Notification that a directive has arrived.  This notification gives the DirectiveHandler a chance to
     * prepare for handling the directive.  For example, the @c DirectiveHandler might use this notification
     * to start downloading an asset that will be required when it becomes time to actually handle the directive.
     * The @c DirectiveHandler will be notified when it should start the actual handling of the directive by a
     * subsequent call to handleDirective().  If an error occurs during the pre-Handling phase and that error
     * should cancel the handling of subsequent @c AVSDirectives with the same @c DialogRequestId, the
     * @c DirectiveHandler should call the setFailed() method on the @c result instance passed in to this call.
     *
     * NOTE: The implementation of this method MUST be thread-safe.
     * NOTE: The implementation of this method MUST return quickly. Failure to do so blocks the processing
     * of subsequent @c AVSDirectives.
     *
     * @param directive The directive to pre-handle.
     * @param result An object to receive the result of the operation.
     */
    virtual void preHandleDirective(
            std::shared_ptr<avsCommon::AVSDirective> directive,
            std::shared_ptr<DirectiveHandlerResultInterface> result) = 0;

    /**
     * Handle the action specified by the directive identified by @c messageId. The handling of subsequent directives
     * with the same @c DialogRequestId may be blocked until the @c DirectiveHandler calls the @c setSucceeded()
     * method of the @c DirectiveHandlingResult instance passed in to the preHandleDirective() call for the directive
     * specified by @c messageId.  If handling of this directive fails such that subsequent directives with the same
     * @c DialogRequestId should be cancelled, this @c DirectiveHandler should instead call setFailed() to indicate
     * a failure.
     *
     * NOTE: The implementation of this method MUST be thread-safe.
     * NOTE: The implementation of this method MUST return quickly. Failure to do so blocks the processing
     * of subsequent @c AVSDirectives.
     *
     * @param messageId The message ID of a directive previously passed to preHandleDirective().
     */
    virtual void handleDirective(const std::string& messageId) = 0;

    /**
     * Cancel an ongoing preHandleDirective() or handleDirective() operation for the specified @c AVSDirective. Once
     * this has been called the @c DirectiveHandler should not expect to receive further calls regarding this directive.
     *
     * NOTE: The implementation of this method MUST be thread-safe.
     * NOTE: The implementation of this method MUST return quickly. Failure to do so blocks the processing
     * of subsequent @c AVSDirectives.
     *
     * @param messageId The message ID of a directive previously passed to preHandleDirective().
     */
    virtual void cancelDirective(const std::string& messageId) = 0;

    /**
     * Shut down this @c DirectiveHandler. This handler will not receive any more calls. All references from this
     * handler to @c DirectiveHandlerResultInterface instances MUST be released before this method returns.
     */
    virtual void shutdown() = 0;
};

} // namespace adsl
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_DIRECTIVE_HANDLER_INTERFACE_H_
