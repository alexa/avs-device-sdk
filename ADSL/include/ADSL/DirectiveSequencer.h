/*
 * DirectiveSequencer.h
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

#ifndef ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_DIRECTIVE_SEQUENCER_H_
#define ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_DIRECTIVE_SEQUENCER_H_

#include <condition_variable>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <AVSCommon/ExceptionEncounteredSenderInterface.h>
#include "ADSL/DirectiveHandlerConfiguration.h"
#include "ADSL/BlockingPolicy.h"
#include "ADSL/DirectiveRouter.h"

namespace alexaClientSDK {
namespace adsl {

/**
 * Class for controlling the sequencing and handling of a stream of @c AVSDirective instances.
 *
 * Customers of this class specify a mapping of @c AVSDirectives specified by (namespace, name) pairs to
 * instances of the @c AVSDirectiveHandlerInterface via calls to @c setDirectiveHandlers(). Changes to this
 * mapping can be made at any time by specifying a new mapping. Customers pass @c AVSDirectives in to this
 * interface for processing via calls to @c onDirective(). @c AVSDirectives are processed in the order that
 * they are received. @c AVSDirectives with non-empty @c DialogRequestId values are filtered by the
 * @c DirectiveSequencer's current @c DialogRequestId value (specified by calls to @c setDialogRequestId().
 * Only @c AVSDirectives with a @c DialogRequestId that is empty or which matches the last setting of the
 * @c DialogRequestId are handled. All others are ignored. Specifying a new @c DialogRequestId value while
 * @c AVSDirectives are already being handled will cancel the handling of @c AVSDirectives that have the
 * previous @c DialogRequestId.
 */
class DirectiveSequencer {
public:
    /**
     * Create a new @c DirectiveSequencer instance.
     *
     * @param sender The instance of the @c ExceptionEncounteredSenderInterface to use to notify AVS
     * when we could not process a directive.
     * @return Returns a new DirectiveSequencer, or nullptr if the operation failed.
     */
    static std::shared_ptr<DirectiveSequencer> create(
            std::shared_ptr<avsCommon::ExceptionEncounteredSenderInterface> exceptionSender);

    /**
     * Destructor.
     */
    ~DirectiveSequencer();

    /**
     * Shut down the DirectiveSequencer.  All calls to @c onDirective() subsequent to a call to shutdown() will fail
     * and a suitable notification will be delivered such that ExceptionEncountered can be sent back to AVS.  This
     * method blocks until all @c AVSDirectives that have been passed to the @c DirectiveSequencer have been
     * canceled and all @c DirectiveHandlers have been shut down.
     */
    void shutdown();

    /**
     * Add mappings from from (@c namespace, @c name) pairs to (@c DirectiveHandler, @c BlockingPolicy) pairs.
     * The addition of a mapping for a (@c namespace, @c name) pair that already has a mapping replaces the
     * existing mapping.  The addition of mapping to (@c DirectiveHandler, @c BlockingPolicy) pairs with either
     * a nullptr @c DirectiveHandler or a @c BlockingPolicy of NONE removes the mapping for that
     * (@c namespace, @c name) pair.  Existing mappings not matching entries of @c configuration are unchanged.
     *
     * NOTE: Calling this method cancels any unhandled @c AVSDirectives already passed into calls to @c onDirective().
     * All subsequent @c AVSDirectives will be ignored until the value of the @c std::future returned from this
     * method has become available.
     *
     * @param configuration The set of mappings to add to the current mappings.
     * @return A future whose value becomes available once the new mapping(s) are active.  The value indicates
     * whether or not the operation was successful.
     */
    std::future<bool> setDirectiveHandlers(const DirectiveHandlerConfiguration& configuration);

    /**
     * Set the current @c DialogRequestId. This value can be set at any time. Setting this value causes a
     * @c DirectiveSequencerInterface to ignore or cancel any unhandled @c AVSDirectives with different
     * (and non-empty) DialogRequestId values.
     *
     * @param dialogRequestId The new value for the current @c DialogRequestId.
     */
    void setDialogRequestId(const std::string& dialogRequestId);

    /**
     * Handle an @c AVSDirective.  Handling is typically deferred to whatever @c DirectiveHandler is associated
     * with the @c AVSDirective's (namespace, name) pair.  If no handler is registered for the @c AVSDirective
     * this method returns false.
     *
     * @param directive The @c AVSDirective to handle.
     * @return Whether or not the directive was accepted.
     */
    bool onDirective(std::shared_ptr<avsCommon::AVSDirective> directive);

private:
    /**
     * Implementation of @c DirectiveHandlerResultInterface that forwards the completion / failure status
     * to the @c DirectiveSequencer from which it originated.
     */
    class DirectiveHandlerResult : public DirectiveHandlerResultInterface {
    public:
        /**
         * Constructor.
         *
         * @param directiveSequencer The @c DirectiveSequencer to forward to the result to.  This is passed
         * and retained as a @c std::weak_ptr to avoid a cyclic std::shared_ptr relationship between
         * @c DirectiveHandlers and @c DirectiveSequencers.
         * @param directive The @c AVSDirective whose handling result will be specified by this instance.
         * @param blockingPolicy The @c BlockingPolicy configured for handling this directive.
         */
        DirectiveHandlerResult(
                std::weak_ptr<DirectiveSequencer> directiveSequencer,
                std::shared_ptr<avsCommon::AVSDirective> directive,
                BlockingPolicy blockingPolicy);

        void setCompleted() override;

        void setFailed(const std::string& description) override;

    private:
        /// The @c DirectiveSequencer to forward to the result to.
        std::weak_ptr<DirectiveSequencer> m_directiveSequencer;

        /// The @c AVSDirective whose handling result will be specified by this instance.
        std::shared_ptr<avsCommon::AVSDirective> m_directive;

        /// The @c BlockingPolicy for this directive.
        BlockingPolicy m_blockingPolicy;
    };

    /**
     * Constructor.
     * @param exceptionSender An instance of the @c ExceptionEncounteredSenderInterface to send exceptions to.
     * will not be handled.
     */
    DirectiveSequencer(std::shared_ptr<avsCommon::ExceptionEncounteredSenderInterface> exceptionSender);

    /**
     * Deleted copy constructor to prevent copying instances of DirectiveSequencer.
     *
     * @param The instance to copy.
     */
    DirectiveSequencer(const DirectiveSequencer& rhs) = delete;

    /**
     * Deleted assignment operator to prevent copying instances of DirectiveSequencer.
     *
     * @param rhs The instance to copy.
     * @return The instance to copy to.
     */
    DirectiveSequencer& operator=(const DirectiveSequencer& rhs) = delete;

    /**
     * Initialize a new instance of @c DirectiveSequencer.
     *
     * @param thisDirectiveSequencer A @c weak_ptr to @c this @c DirectiveSequencer.
     */
    void init(std::weak_ptr<DirectiveSequencer> thisDirectiveSequencer);

    /**
     * Set @c m_isStopping to @c true.
     */
    void setIsStopping();

    /**
     * Clear all handlers from @c m_directiveRouter() and tell all the removed handlers to shut down.
     */
    void clearDirectiveRouter();

    /**
     * Receive notification that the handling of an @c AVSDirective has completed.
     * @param directive The @c AVSDirective whose handling has completed.
     */
    void onHandlingCompleted(std::shared_ptr<avsCommon::AVSDirective> directive);

    /**
     * Receive notification that the handling of an @c AVSDirective has failed.
     * @param directive The @c AVSDirective whose handling has failed.
     * @param description A description (suitable for logging diagnostics) that indicates the nature of the failure.
     */
    void onHandlingFailed(std::shared_ptr<avsCommon::AVSDirective> directive, const std::string& description);

    /**
     * The thread method for the @c m_receivingThread.  Loops, popping directives from @c m_receivingQueue,
     * performing initial processing and forwarding some directives to @c m_processingThread for handling
     * or canceling.
     */
    void receiveDirectivesLoop();

    /**
     * Get the next directive from the m_receivingQueue.
     * @return The next @c AVSDirective from @c m_receivingQueue to process, or nullptr of ths @c DirectiveSequencer
     * is stopping and @c m_receivingQueue is empty.
     */
    std::shared_ptr<avsCommon::AVSDirective> receiveDirective();

    /**
     * Invoke @c handleDirective(AVSDirective, DirectiveHandlingResult) or @c preHandleDirective() with the provided
     * @c AVSDirective, if possible. Return @c true if preHandleDirective() was invoked and the provided directive
     * should be pushed to @c m_handlingQueue for further processing.  If @c true is returned,
     * @c m_directiveBeingPreHandled will be set to the provided directive.
     *
     * @param directive The @c AVSDirective to start processing.
     * @return Whether or not the specified @c AVSDirective requires further processing.
     */
    bool shouldProcessDirective(std::shared_ptr<avsCommon::AVSDirective> directive);

    /**
     * If the @c AVSDirective specified by @c m_directiveBeingPreHandled is not nullptr, push it on to
     * @c m_handlingQueue or @c m_cancelingQueue, and reset @c m_directiveBeingPreHandled.
     */
    void queueDirectiveForProcessing();

    /**
     * The thread method for the @c m_processingThread.  Loops, popping @c AVSDirectives from @c m_handlingQueue and
     * @c m_cancelingQueue and processing them.
     */
    void processDirectivesLoop();

    /**
     * Shut down the @c setDirectiveHandlers() in the set @c m_removedHandlers, and empty the set when done.
     * NOTE: m_processingMutex must be locked by the thread that calls this method.
     *
     * @param lock A lock on m_processingMutex.
     */
    void shutdownRemovedHandlersLocked(std::unique_lock<std::mutex>& lock);

    /**
     * Handles processing of directives when @c m_isStopping is @c true.  If @c AVSDirectives are in m_handlingQueue,
     * moves those directives to @c m_cancelingQueue.  If m_cancelingQueue is not empty, returns false to indicate
     * that there are still @c AVSDirectives that need processing (i.e. canceling).
     *
     * @return Whether or not @c processDirectivesLoop() should exit.
     */
    bool shouldProcessingStop();

    /**
     * Call @c cancelDirective() on each @c AVSDirective in @c m_cancelingQueue.
     */
    void cancelDirectives();

    /**
     * Call @c handleDirective(MessageId, bool isBlocking) on the next @c AVSDirective in m_handlingQueue, unless an
     * @c AVSDirective is already being handled (i.e. @c handleDirective() wad called with @c isBlocking = @c true
     * and the result of the operation has yet to be indicated by the handler).
     */
    void handleDirective();

    /**
     * Remove the specified directive from @c m_handlingQueue or @c m_cancelingQueue.  If @c cancelAllIfFound
     * is @c true and the specified @c AVSDirective was found in either queue, move all @c AVSDirectives from
     * m_handlingQueue to @c m_cancelingQueue (to cancel the entire group of @c AVSDirectives with the same
     * @c DialogrequestId.
     *
     * @param caller Text used to specify the caller of this function when creating log messages.
     * @param directive The directive ot remove from processing.
     * @param cancelAllIfFound Whether the @c AVSDirectives in m_handlingQueue should be cancelled if the
     * specified @c AVSDirective is still in @c m_handlingQueue or @c m_cancelingQueue.
     */
    void removeDirectiveFromProcessing(
            const std::string& caller, std::shared_ptr<avsCommon::AVSDirective> directive, bool cancelAllIfFound);

    /**
     * Cancel handling of directives in m_handlingQueue. Moves the contents of m_handlingQueue to the
     * end of m_cancelingQueue.
     * NOTE: MUST be called with m_processingMutex acquired.
     */
    void cancelPreHandledDirectivesLocked();

    /**
     * Check if the DirectiveSequencer has drained of @c AVSDirectives.  If so, and there is an outstanding promise
     * to complete a @c setDirectiveHandlers() operation, complete the operation and fulfill that promise.
     *
     * NOTE: This is the only method that acquires both @c m_receivingMutex and @c m_handlingMutex.  It does so
     * in that order.  So, any future methods that need to acquire both mutexes must do so in the same order to
     * avoid a deadlock.
     */
    void maybeSetDirectiveHandlers();

    /**
     * Send ExceptionEncountered message top AVS for @c AVSDirectives that were not processed.
     *
     * @param directive The @c AVSDirective that was not processed.
     * @param error The type of error that prevented the @c AVSDirective from being processed.
     * @param message Text used to describe why the @c AVSDirective was not processed.
     */
    void sendExceptionEncountered(
            std::shared_ptr<avsCommon::AVSDirective> directive,
            avsCommon::ExceptionErrorType error,
            const std::string& message);

    /// The @c ExceptionEncounteredSenderInterface instance to send exceptions to.
    std::shared_ptr<avsCommon::ExceptionEncounteredSenderInterface> m_exceptionSender;

    /// Whether the DirectiveSequencer is shutting down.
    std::atomic<bool> m_isStopping;

    /// Weak pointer to @c this, used to create links back to @c this without creating a @c shared_ptr cycle.
    std::weak_ptr<DirectiveSequencer> m_thisDirectiveSequencer;

    /// Mutex to serialize access to @c m_receivingQueue.
    std::mutex m_receivingMutex;

    /// Condition variable used to wake m_receivingThread.
    std::condition_variable m_receivingNotifier;

    /// Thread to process @c AVSDirectives in @c m_receivingQueue.
    std::thread m_receivingThread;

    /// Queue of @c AVSDirectives waiting to be received.
    std::deque<std::shared_ptr<avsCommon::AVSDirective>> m_receivingQueue;

    /**
     * Mutex to serialize access to @c m_dialogRequestId, @c m_directiveBeingPreHandled, @c m_isHandlingDirective,
     * @c m_handlingQueue, @c m_cancellingQueue.
     */
    std::mutex m_processingMutex;

    /// Condition variable used to wake m_processingThread.
    std::condition_variable m_processingNotifier;

    /// Thread to process @c AVSDirectives in @c m_handlingQueue and @c m_cancelingQueue.
    std::thread m_processingThread;

    /// The DialogRequestID used to filter out @c AVSDirectives (unless their DialogRequestId is empty).
    std::string m_dialogRequestId;

    /**
     * Whether or not a call to @c handleDirectiveImmediately() is in progress. This is used to help with the
     * case where @c setDirectiveHandlers() is called and cannot complete while the @c DirectiveSequencer is
     * actively processing @c AVSDirectives.
     */
    bool m_handlingReceivedDirective;

    /**
     * The directive (if any) that has been passed to an active call to preHandleDirective(). This is used to
     * handle the case where @ onDirectiveError() is called before the @c AVSDirective has been added to
     * m_handlingQueue.  @c m_receivingThread sets this value before calling preHandleDirective(), makes the
     * call and then checks the value once it has regained m_processingMutex.  If a call to onDirectiveError()
     * has occurred for this @c AVSDirective in the mean time, it will clear @c m_directiveBeingPreHandled,
     * indicating to @c m_receivingThread that it should discard the @c AVSDirective, instead of push it onto
     * @c m_handlingQueue or @c m_cancelingQueue.
     */
    std::shared_ptr<avsCommon::AVSDirective> m_directiveBeingPreHandled;

    /// Whether the m_processingThread is currently handling the @c AVSDirective at the front of @c m_handlingQueue.
    bool m_isHandlingDirective;

    /// Queue of @c AVSDirectives waiting to be handled.
    std::deque<std::shared_ptr<avsCommon::AVSDirective>> m_handlingQueue;

    /// Queue of @c AVSDirectives waiting to be cancelled.
    std::deque<std::shared_ptr<avsCommon::AVSDirective>> m_cancellingQueue;

    /// Whether the DirectiveSequencer is processing a call to setDirectiveHandlers().
    std::atomic<bool> m_isSettingDirectiveHandlers;

    /**
     * @c promise for @c future returned from @c setDirectiveHandlers().  When set, the value indicates whether
     * or not the operation was successful.
     */
    std::shared_ptr<std::promise<bool>> m_setDirectiveHandlersPromise;

    /**
     * The @c DirectiveHandlerConfiguration to add to the current mapping once the @c DireciveSequencer has no
     * @c AVSDirectives to process.
     */
    DirectiveHandlerConfiguration m_newDirectiveHandlerConfiguration;

    /// Set of removed @c DirectiveHandlers that need to be shut down.
    std::set<std::shared_ptr<DirectiveHandlerInterface>> m_removedHandlers;

    /// Object used to map (@c namespace, @c name) pairs to (@c DirectiveHandlerInterface, @c BlockingPolicy) pairs.
    DirectiveRouter m_directiveRouter;
};

} // namespace adsl
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_DIRECTIVE_SEQUENCER_H_
