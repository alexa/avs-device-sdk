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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_EXCEPTIONENCOUNTEREDSENDERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_EXCEPTIONENCOUNTEREDSENDERINTERFACE_H_

#include "AVSCommon/AVS/ExceptionErrorType.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This specifies the interface to send an ExceptionEncountered event to AVS.
 */
class ExceptionEncounteredSenderInterface {
public:
    /**
     * Virtual destructor to ensure proper cleanup by derived types.
     */
    virtual ~ExceptionEncounteredSenderInterface() = default;

    /**
     * Send a @c System::ExceptionEncountered message to AVS.
     *
     * @note The implementation of this method MUST return quickly. Failure to do so blocks the processing
     * of subsequent @c AVSDirectives.
     *
     * @param unparsedDirective The unparsed JSON of the directive.
     * @param error The type of error encountered.
     * @param errorDescription Additional error details for logging and troubleshooting.
     */
    virtual void sendExceptionEncountered(
        const std::string& unparsedDirective,
        avs::ExceptionErrorType error,
        const std::string& errorDescription) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_EXCEPTIONENCOUNTEREDSENDERINTERFACE_H_
