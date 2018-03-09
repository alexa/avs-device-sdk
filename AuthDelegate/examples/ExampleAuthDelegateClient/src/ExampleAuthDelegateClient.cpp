/*
 * Copyright 2016-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <cstdlib>
#include <fstream>

#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "AuthDelegate/AuthDelegate.h"

using namespace alexaClientSDK;
using namespace alexaClientSDK::avsCommon::sdkInterfaces;
using authDelegate::AuthDelegate;

/// String to identify log entries originating from this file.
static const std::string TAG("AlexAuthDelegateClient");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Simple implementation of the AuthDelegateObserverInterface.
class Observer : public AuthObserverInterface {
public:
    /// Construct an Observer, initialized as not authorized.
    Observer() : m_state(AuthObserverInterface::State::UNINITIALIZED) {
    }

    void onAuthStateChange(
        AuthObserverInterface::State newState,
        AuthObserverInterface::Error error = AuthObserverInterface::Error::SUCCESS) override {
        if (error == AuthObserverInterface::Error::SUCCESS) {
            ACSDK_DEBUG(LX("onAuthStateChange").d("newState", newState));
        } else {
            ACSDK_ERROR(LX("onAuthStateChangeError").d("newState", newState).d("error", error));
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        m_state = newState;
        m_error = error;
        m_wakeTrigger.notify_all();
    }

    /**
     * Wait until we are authorized.
     * @return true if we are authorized.
     */
    bool wait() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_wakeTrigger.wait(lock, [this]() {
            return AuthObserverInterface::State::REFRESHED == m_state ||
                   AuthObserverInterface::State::UNRECOVERABLE_ERROR == m_state;
        });
        return AuthObserverInterface::State::REFRESHED == m_state;
    }

private:
    /// Last state from authDelegate
    AuthObserverInterface::State m_state;

    /// Last error from authDelegate
    AuthObserverInterface::Error m_error;

    /// mutex for waiting
    std::mutex m_mutex;

    /// Trigger for waking from wait.
    std::condition_variable m_wakeTrigger;
};

/// Instantiate an AuthDelegate and fetch an authToken every couple of seconds.
int exerciseAuthDelegate() {
    auto authDelegate = AuthDelegate::create();

    if (!authDelegate) {
        ACSDK_ERROR(LX("exerciseAuthDelegateFailed").d("reason", "failedToCreateAuthDelegate"));
        return EXIT_FAILURE;
    }

    auto observer = std::make_shared<Observer>();
    authDelegate->addAuthObserver(observer);

    // Wait until we know we are authorized or know we will never be authorized.
    if (!observer->wait()) {
        // We will never be authorized, so exit now.
        return EXIT_FAILURE;
    }

    for (int ix = 0; ix < 100; ix++) {
        std::string accessToken = authDelegate->getAuthToken();
        ACSDK_DEBUG(LX("getAccessToken").sensitive("token", accessToken));
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    return EXIT_SUCCESS;
}

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        ACSDK_ERROR(LX("ExampleAuthDelegateClientFailed")
                        .d("reason", "missingConfigurationFilePath")
                        .d("usage", "ExampleAuthDelegateClient <path-to-SDK-config-file>"));
        return EXIT_FAILURE;
    }
    std::ifstream infile(argv[1]);
    if (!infile.good()) {
        ACSDK_ERROR(
            LX("ExampleAuthDelegateClientFailed").d("reason", "openConfigurationFileFailed").d("path", argv[1]));
        return EXIT_FAILURE;
    }
    if (!avsCommon::avs::initialization::AlexaClientSDKInit::initialize({&infile})) {
        ACSDK_ERROR(LX("ExampleAuthDelegateClientFailed").d("reason", "alexaClientSDKInitFailed"));
        return EXIT_FAILURE;
    }
    auto result = exerciseAuthDelegate();
    avsCommon::avs::initialization::AlexaClientSDKInit::uninitialize();
    return result;
}
