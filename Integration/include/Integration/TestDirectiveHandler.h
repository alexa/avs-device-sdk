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

#ifndef ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_TESTDIRECTIVEHANDLER_H_
#define ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_TESTDIRECTIVEHANDLER_H_

#include <condition_variable>
#include <string>
#include <future>
#include <fstream>
#include <chrono>
#include <deque>
#include <mutex>
#include <unordered_map>

#include "AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h"
#include "AVSCommon/AVS/Attachment/AttachmentManager.h"
#include "AVSCommon/Utils/JSON/JSONUtils.h"

using namespace alexaClientSDK::avsCommon;

namespace alexaClientSDK {
namespace integration {
namespace test {

/**
 * TestDirectiveHandler is a mock of the @c DirectiveHandlerInterface and allows tests
 * to wait for invocations upon those interfaces and inspect the parameters of those invocations.
 */
class TestDirectiveHandler : public avsCommon::sdkInterfaces::DirectiveHandlerInterface {
public:
    /**
     * Constructor.
     *
     * @param config The @c avsCommon::avs::DirectiveHandlerConfiguration for the directive handler for registering
     * with a directive sequencer.
     */
    TestDirectiveHandler(avsCommon::avs::DirectiveHandlerConfiguration config);

    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;

    void preHandleDirective(
        std::shared_ptr<avsCommon::avs::AVSDirective> directive,
        std::unique_ptr<avsCommon::sdkInterfaces::DirectiveHandlerResultInterface> result) override;

    bool handleDirective(const std::string& messageId) override;

    void cancelDirective(const std::string& messageId) override;

    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;

    void onDeregistered() override;

    /**
     * Class defining the parameters to calls to the mocked interfaces.
     */
    class DirectiveParams {
    public:
        /**
         * Constructor.
         */
        DirectiveParams();

        /**
         * Return whether this DirectiveParams is of type 'UNSET'.
         *
         * @return Whether this DirectiveParams is of type 'UNSET'.
         */
        bool isUnset() const {
            return Type::UNSET == type;
        }

        /**
         * Return whether this DirectiveParams is of type 'HANDLE_IMMEDIATELY'.
         *
         * @return Whether this DirectiveParams is of type 'HANDLE_IMMEDIATELY'.
         */
        bool isHandleImmediately() const {
            return Type::HANDLE_IMMEDIATELY == type;
        }

        /**
         * Return whether this DirectiveParams is of type 'PREHANDLE'.
         *
         * @return Whether this DirectiveParams is of type 'PREHANDLE'.
         */
        bool isPreHandle() const {
            return Type::PREHANDLE == type;
        }

        /**
         * Return whether this DirectiveParams is of type 'HANDLE'.
         *
         * @return Whether this DirectiveParams is of type 'HANDLE'.
         */
        bool isHandle() const {
            return Type::HANDLE == type;
        }

        /**
         * Return whether this DirectiveParams is of type 'CANCEL'.
         *
         * @return Whether this DirectiveParams is of type 'CANCEL'.
         */
        bool isCancel() const {
            return Type::CANCEL == type;
        }

        /**
         * Return whether this DirectiveParams is of type 'TIMEOUT'.
         *
         * @return Whether this DirectiveParams is of type 'TIMEOUT'.
         */
        bool isTimeout() const {
            return Type::TIMEOUT == type;
        }

        // Enum for the way the directive was passed to DirectiveHandler.
        enum class Type {
            // Not yet set.
            UNSET,
            // Set when handleDirectiveImmediately is called.
            HANDLE_IMMEDIATELY,
            // Set when preHandleDirective is called.
            PREHANDLE,
            // Set when handleDirective is called.
            HANDLE,
            // Set when cancelDirective is called.
            CANCEL,
            // Set when waitForNext times out waiting for a directive.
            TIMEOUT
        };

        // Type of how the directive was passed to DirectiveHandler.
        Type type;
        // AVSDirective passed from the Directive Sequencer to the DirectiveHandler.
        std::shared_ptr<avsCommon::avs::AVSDirective> directive;
        // DirectiveHandlerResult to inform the Directive Sequencer a directive has either successfully or
        // unsuccessfully handled.
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerResultInterface> result;
    };

    /**
     * Function to retrieve the next DirectiveParams in the test queue or time out if the queue is empty. Takes a
     * duration in seconds to wait before timing out.
     */
    DirectiveParams waitForNext(const std::chrono::seconds duration);

private:
    /// Mutex to protect m_queue.
    std::mutex m_mutex;
    /// Trigger to wake up waitForNext calls.
    std::condition_variable m_wakeTrigger;
    /// Queue of received directives that have not been waited on.
    std::deque<DirectiveParams> m_queue;
    /// map of message IDs to result handlers.
    std::unordered_map<std::string, std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerResultInterface>>
        m_results;
    /// map of message IDs to result handlers.
    std::unordered_map<std::string, std::shared_ptr<avsCommon::avs::AVSDirective>> m_directives;
    /// The @c avsCommon::avs::DirectiveHandlerConfiguration of the handler.
    avsCommon::avs::DirectiveHandlerConfiguration m_configuration;
};
}  // namespace test
}  // namespace integration
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_TESTDIRECTIVEHANDLER_H_
