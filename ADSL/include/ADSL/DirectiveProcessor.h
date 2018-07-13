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

#ifndef ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_DIRECTIVEPROCESSOR_H_
#define ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_DIRECTIVEPROCESSOR_H_

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include <AVSCommon/AVS/AVSDirective.h>
#include <AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h>

#include "ADSL/DirectiveRouter.h"

namespace alexaClientSDK {
namespace adsl {

/**
 * Object to process @c AVSDirectives that have a non-empty @c dialogRequestId.
 * @par
 * @c DirectiveProcessor receives directives via its @c onDirective() method. The @c dialogRequestId property of
 * incoming directives is checked against the current @c dialogRequestId (which is set by @c setDialogRequestId()).
 * If the @c AVSDirective's value is not empty and does not match, the @c AVSDirective is dropped, and
 * @c onDirective() returns @c true to indicate that the @c AVSDirective has been consumed (in this case, because
 * it is not longer relevant).
 * @par
 * After passing this hurdle, the @c AVSDirective is forwarded to the @c preHandleDirective() method of whichever
 * @c DirectiveHandler is registered to handle the @c AVSDirective. If no @c DirectiveHandler is registered, the
 * incoming directive is rejected and any directives with the same dialogRequestId that are already queued for
 * handling by the @c DirectiveProcessor are canceled (because an entire dialog is canceled when the handling of
 * any of its directives fails), and @c onDirective() returns @c false to indicate that the unhandled @c AVDirective
 * was rejected.
 * @par
 * Once an @c AVSDirective has been successfully forwarded for preHandling, it is enqueued awaiting its turn to be
 * handled. Handling is accomplished by forwarding the @c AVSDirective to the @c handleDirective() method of
 * whichever @c DirectiveHandler is registered to handle the @c AVSDirective. The handling of an @c AVSDirective can
 * be configured as @c BLOCKING or @c NON_BLOCKING. If the directive at the head of the handling queue is configured
 * for @c BLOCKING, the handling of subsequent @c AVSDirectives is held up until the @c DirectiveHandler for the
 * @c BLOCKING @c AVSDirective indicates that handling has completed or failed. Otherwise handleDirective() is
 * invoked, the @c AVSDirective is popped from the front of the queue, and processing of queued @c AVSDirective's
 * continues.
 */
class DirectiveProcessor {
public:
    /**
     * Constructor.
     *
     * @param directiveRouter An object used to route directives to their registered handler.
     */
    DirectiveProcessor(DirectiveRouter* directiveRouter);

    /**
     * Destructor.
     */
    ~DirectiveProcessor();

    /**
     * Set the current @c dialogRequestId. If a new value is specified any @c AVSDirective's whose pre-handling
     * or handling is already in progress the directive will be cancelled.
     *
     * @param dialogRequestId The new value for the current @c dialogRequestId.
     */
    void setDialogRequestId(const std::string& dialogRequestId);

    /**
     * Queue an @c AVSDirective for handling by whatever @c DirectiveHandler was registered to handle it.
     *
     * @param directive The @c AVADirective to process.
     * @return Whether the directive was consumed.
     */
    bool onDirective(std::shared_ptr<avsCommon::avs::AVSDirective> directive);

    /**
     * Shut down the DirectiveProcessor.  This queues all outstanding @c AVSDirectives for cancellation and
     * blocks until the processing of all @c AVSDirectives has completed.
     */
    void shutdown();

    /**
     * Disable the DirectiveProcessor, queues all outstanding @c AVSDirectives for cancellation and
     * blocks until the processing of all @c AVSDirectives has completed.
     */
    void disable();

    /**
     * Enable the DirectiveProcessor.
     *
     * @return Whether it succeeded to enable the directive processor.
     */
    bool enable();

private:
    /**
     * Handle used to identify @c DirectiveProcessor instances referenced by @c DirectiveHandlerResult.
     *
     * Handles are used instead of a pointers to decouple the lifecycle of @c DirectiveProcessors from the lifecycle
     * of @c DirectiveHandlerInterface instances.  In the case that a DirectiveHandler outlives the
     * @c DirectiveProcessor it may complete (or fail) the handling of a directive after (or during) the destruction
     * of the @c DirectiveProcessor.  Using a handle instead of a pointer allows delivery of the completion / failure
     * notification to be dropped gracefully if the @c DirectiveProcessor is no longer there to receive it.
     *
     * @c ProcessorHandle values are mapped to @c DirectiveProcessor instances by the static @c m_handleMap.
     * The delivery of completion notifications by @c DirectiveHandlerResult and changes to @c m_handleMap
     * are serialized with the static @c m_handleMapMutex.
     */
    using ProcessorHandle = unsigned int;

    /**
     * Implementation of @c DirectiveHandlerResultInterface that forwards the completion / failure status
     * to the @c DirectiveProcessor from which it originated.
     */
    class DirectiveHandlerResult : public avsCommon::sdkInterfaces::DirectiveHandlerResultInterface {
    public:
        /**
         * Constructor.
         *
         * @param processorHandle handle of the @c DirectiveProcessor to forward to the result to.
         * @param directive The @c AVSDirective whose handling result will be specified by this instance.
         */
        DirectiveHandlerResult(
            ProcessorHandle processorHandle,
            std::shared_ptr<avsCommon::avs::AVSDirective> directive);

        void setCompleted() override;

        void setFailed(const std::string& description) override;

    private:
        /// Handle of the @c DirectiveProcessor to forward notifications to.
        ProcessorHandle m_processorHandle;

        /// The @c AVSDirective whose handling result will be specified by this instance.
        std::shared_ptr<avsCommon::avs::AVSDirective> m_directive;
    };

    /**
     * Receive notification that the handling of an @c AVSDirective has completed.
     *
     * @param directive The @c AVSDirective whose handling has completed.
     */
    void onHandlingCompleted(std::shared_ptr<avsCommon::avs::AVSDirective> directive);

    /**
     * Receive notification that the handling of an @c AVSDirective has failed.
     *
     * @param directive The @c AVSDirective whose handling has failed.
     * @param description A description (suitable for logging diagnostics) that indicates the nature of the failure.
     */
    void onHandlingFailed(std::shared_ptr<avsCommon::avs::AVSDirective> directive, const std::string& description);

    /**
     * Remove an @c AVSDirective from processing.
     * @note This method must only be called by threads that have acquired @c m_mutex.
     *
     * @param directive The @c AVSDirective to remove from processing.
     */
    void removeDirectiveLocked(std::shared_ptr<avsCommon::avs::AVSDirective> directive);

    /**
     * Thread method for m_processingThread.
     */
    void processingLoop();

    /**
     * Process (cancel) all the items in @c m_cancelingQueue.
     * @note This method must only be called by threads that have acquired @c m_mutex.
     *
     * @param lock A @c unique_lock on m_mutex from the callers context, allowing this method to release
     * (and re-acquire) the lock around callbacks that need to be invoked.
     * @return Whether the @c AVSDirectives in @c m_cancelingQueue were processed.
     */
    bool processCancelingQueueLocked(std::unique_lock<std::mutex>& lock);

    /**
     * Process (handle) the next @c AVSDirective in @c m_handlingQueue.
     * @note This method must only be called by threads that have acquired @c m_mutex.
     *
     * @param lock A @c unique_lock on m_mutex from the callers context, allowing this method to release
     * (and re-acquire) the lock around callbacks that need to be invoked.
     * @return  Whether an @c AVSDirective from m_handlingQueue was processed.
     */
    bool handleDirectiveLocked(std::unique_lock<std::mutex>& lock);

    /**
     * Set the current @c dialogRequestId. This cancels the processing of any @c AVSDirectives with a non-empty
     * dialogRequestId.
     * @note This method must only be called by threads that have acquired @c m_mutex.
     *
     * @param dialogRequestId The new value for the current @c dialogRequestId.
     */
    void setDialogRequestIdLocked(const std::string& dialogRequestId);

    /**
     * Cancel the processing any @c AVSDirective with the specified dialogRequestId, and clear the m_dialogRequestID
     * if it matches the specified dialogRequestId.
     * @note This method must only be called by threads that have acquired @c m_mutex.
     *
     * @param dialogRequestId The dialogRequestId to scrub from processing.
     */
    void scrubDialogRequestIdLocked(const std::string& dialogRequestId);

    /**
     * Move all the directives being handled or queued for handling to @c m_cancelingQueue. Also reset the
     * current @c dialogRequestId.
     * @note This method must only be called by threads that have acquired @c m_mutex.
     */
    void queueAllDirectivesForCancellationLocked();

    /// Handle value identifying this instance.
    int m_handle;

    /// A mutex used to serialize @c DirectiveProcessor operations with operations that occur in the creating context.
    std::mutex m_mutex;

    /// Object used to route directives to their assigned handler.
    DirectiveRouter* m_directiveRouter;

    /// Whether or not the @c DirectiveProcessor is shutting down.
    bool m_isShuttingDown;

    /// Whether or not the @c DirectiveProcessor is enabled.
    bool m_isEnabled;

    /// The current @c dialogRequestId
    std::string m_dialogRequestId;

    /// Queue of @c AVSDirectives waiting to be canceled.
    std::deque<std::shared_ptr<avsCommon::avs::AVSDirective>> m_cancelingQueue;

    /// The directive (if any) for which a preHandleDirective() call is in progress.
    std::shared_ptr<avsCommon::avs::AVSDirective> m_directiveBeingPreHandled;

    /// Queue of @c AVSDirectives waiting to be handled.
    std::deque<std::shared_ptr<avsCommon::avs::AVSDirective>> m_handlingQueue;

    /// Whether @c handleDirective() has been called for the directive at the @c front() of @c m_handlingQueue.
    bool m_isHandlingDirective;

    /// Condition variable used to wake @c processingLoop() when it is waiting.
    std::condition_variable m_wakeProcessingLoop;

    /// Thread processing elements on @c m_handlingQueue and @c m_cancelingQueue.
    std::thread m_processingThread;

    /// Mutex serializing the body of @ onDirective() to make the method thread-safe.
    std::mutex m_onDirectiveMutex;

    /// Mutex to serialize access to @c m_handleMap;
    static std::mutex m_handleMapMutex;

    /**
     * Map from @c ProcessorHandle value to @c DirectiveProcessor instance to allow for gracefully dropping a
     * completion (or failure) notification forwarded to the @c DirectiveProcessor during or after its destruction.
     */
    static std::unordered_map<ProcessorHandle, DirectiveProcessor*> m_handleMap;

    /// Next available @c ProcessorHandle value.
    static ProcessorHandle m_nextProcessorHandle;
};

}  // namespace adsl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_DIRECTIVEPROCESSOR_H_
