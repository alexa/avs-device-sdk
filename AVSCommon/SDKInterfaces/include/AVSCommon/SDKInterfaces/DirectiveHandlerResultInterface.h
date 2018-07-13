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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_DIRECTIVEHANDLERRESULTINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_DIRECTIVEHANDLERRESULTINTERFACE_H_

#include <memory>

#include "AVSCommon/AVS/AVSDirective.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * Interface for indicating the result of a request to @c preHandle() or @c handle() an @c AVSDirective.
 *
 * Once one of the 'setter' methods have been called the handling of the @c AVSDirective is finished and
 * further calls to 'setter' methods should not be made.
 */
class DirectiveHandlerResultInterface {
public:
    /**
     * Virtual destructor to ensure proper cleanup by derived types.
     */
    virtual ~DirectiveHandlerResultInterface() = default;

    /**
     * Indicate that the handling of the @c AVSDirective successfully ran to successful completion.
     *
     * @note Once this has been called, no other methods of this object should be called.
     */
    virtual void setCompleted() = 0;

    /**
     * Indicate that handling an @c AVSDirective failed such that subsequent @c AVSDirectives with the
     * same @c DialogRequestId should be cancelled.
     *
     * @note Once this has been called, no other methods of this object should be called.
     *
     * @param description A description, suitable for logging, that indicates the nature of the failure.
     */
    virtual void setFailed(const std::string& description) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_DIRECTIVEHANDLERRESULTINTERFACE_H_
