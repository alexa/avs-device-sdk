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

#ifndef ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_DIRECTIVESEQUENCER_H_
#define ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_DIRECTIVESEQUENCER_H_

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>

#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveSequencerInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/Threading/ConditionVariableWrapper.h>

#include "ADSL/DirectiveProcessor.h"
#include "ADSL/DirectiveRouter.h"

namespace alexaClientSDK {
namespace adsl {

/**
 * Class for sequencing and handling a stream of @c AVSDirective instances.
 */
class DirectiveSequencer : public avsCommon::sdkInterfaces::DirectiveSequencerInterface {
public:
    /**
     * Create a DirectiveSequencerInterface.
     *
     * @param exceptionSender An instance of the @c ExceptionEncounteredSenderInterface used to send
     * ExceptionEncountered messages to AVS for directives that are not handled.
     * @param shutdownNotifier The object with which to register for a notification when it is time to shut down.
     * @param metricRecorder The metric recorder.
     * @return Returns a new DirectiveSequencer, or nullptr if the operation failed.
     */
    static std::shared_ptr<DirectiveSequencerInterface> createDirectiveSequencerInterface(
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface> shutdownNotifier,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder);

    /**
     * Create a DirectiveSequencer.
     *
     * @deprecated Use createDirectiveSequencerInterface.
     * @param exceptionSender An instance of the @c ExceptionEncounteredSenderInterface used to send
     * ExceptionEncountered messages to AVS for directives that are not handled.
     * @param metricRecorder The metric recorder.
     * @return Returns a new DirectiveSequencer, or nullptr if the operation failed.
     */
    static std::unique_ptr<DirectiveSequencerInterface> create(
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder = nullptr);

    bool addDirectiveHandler(std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> handler) override;

    bool removeDirectiveHandler(std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> handler) override;

    void setDialogRequestId(const std::string& dialogRequestId) override;

    std::string getDialogRequestId() override;

    bool onDirective(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;

    void disable() override;

    void enable() override;

private:
    /**
     * Constructor.
     * @param exceptionSender An instance of the @c ExceptionEncounteredSenderInterface used to send
     * ExceptionEncountered messages to AVS for directives that are not handled.
     * @param metricRecorder The metric recorder.
     */
    DirectiveSequencer(
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder);

    void doShutdown() override;

    /**
     * Thread method for m_receivingThread.
     */
    void receivingLoop();

    /**
     * Process the next item in @c m_receivingQueue.
     * @note This method must only be called by threads that have acquired @c m_mutex.
     *
     * @param lock A @c unique_lock on m_mutex, allowing this method to release the lock around callbacks
     * that need to be invoked.
     */
    void receiveDirectiveLocked(std::unique_lock<std::mutex>& lock);

    /// Serializes access to data members (besides m_directiveRouter and m_directiveProcessor).
    std::mutex m_mutex;

    /// The @c ExceptionEncounteredSenderInterface instance to send exceptions to.
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> m_exceptionSender;

    /// Whether or not the @c DirectiveReceiver is shutting down.
    bool m_isShuttingDown;

    /// Whether or not the @c DirectiveSequencer is enabled.
    bool m_isEnabled;

    /// Object used to route directives to their assigned handler.
    DirectiveRouter m_directiveRouter;

    /// Object used to drive handling of @c AVSDirectives.
    std::shared_ptr<DirectiveProcessor> m_directiveProcessor;

    /// Queue of @c AVSDirectives waiting to be received.
    std::deque<std::shared_ptr<avsCommon::avs::AVSDirective>> m_receivingQueue;

    /// Condition variable used to wake m_receivingLoop when waiting.
    avsCommon::utils::threading::ConditionVariableWrapper m_wakeReceivingLoop;

    /// The @c PowerResource associated with this thread.
    std::shared_ptr<avsCommon::utils::power::PowerResource> m_powerResource;

    /// Thread to receive directives.
    std::thread m_receivingThread;
};

}  // namespace adsl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_DIRECTIVESEQUENCER_H_