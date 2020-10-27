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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_EXPECTSPEECHTIMEOUTHANDLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_EXPECTSPEECHTIMEOUTHANDLERINTERFACE_H_

#include <chrono>
#include <functional>
#include <future>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * Offers implementations the ability to handle the @c ExpectSpeech timeout. As an example, this may be useful to
 * applications with remote microphones.
 */
class ExpectSpeechTimeoutHandlerInterface {
public:
    /**
     * Destructor.
     */
    virtual ~ExpectSpeechTimeoutHandlerInterface() = default;

    /**
     * This function allows applications to tell the @c AudioInputProcessor that the @c ExpectSpeech directive's timeout
     * will be handled externally and stops the @c AudioInputProcessor from starting an internal timer to handle it.
     *
     * @param timeout The timeout of the @c ExpectSpeech directive.
     * @param expectSpeechTimedOut An @c std::function that applications may call if the timeout expires. This results
     * in an @c ExpectSpeechTimedOut event being sent to AVS if no @c recognize() call is made prior to the timeout
     * expiring. This function will return a future which is @c true if called in the correct state and an
     * @c ExpectSpeechTimeout Event was sent successfully, or @c false otherwise.
     * @return @c true if the @c ExpectSpeech directive's timeout will be handled externally and should not be handled
     * via an internal timer owned by the @c AudioInputProcessor.
     *
     * @note This function will be called after any calls to the @c AudioInputProcessorObserverInterface's
     * onStateChanged() method to notify of a state change to @c EXPECTING_SPEECH.
     * @note Implementations are not required to be thread-safe.
     * @note If the @cAudioInputProcessor object that this call came from is destroyed, the @c expectSpeechTimedOut
     * parameter is no longer valid.
     */
    virtual bool handleExpectSpeechTimeout(
        std::chrono::milliseconds timeout,
        const std::function<std::future<bool>()>& expectSpeechTimedOut) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_EXPECTSPEECHTIMEOUTHANDLERINTERFACE_H_
