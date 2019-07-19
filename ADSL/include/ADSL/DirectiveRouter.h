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

#ifndef ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_DIRECTIVEROUTER_H_
#define ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_DIRECTIVEROUTER_H_

#include <map>
#include <set>
#include <unordered_map>

#include <AVSCommon/AVS/NamespaceAndName.h>
#include <AVSCommon/AVS/DirectiveHandlerConfiguration.h>
#include <AVSCommon/AVS/HandlerAndPolicy.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace adsl {
/**
 * Class to maintain a mapping from @c NamespaceAndName to @c HandlerAndPolicy, and to invoke
 * @c DirectiveHandlerInterface methods on the @c DirectiveHandler registered for a given @c AVSDirective.
 */
class DirectiveRouter : public avsCommon::utils::RequiresShutdown {
public:
    /// Constructor.
    DirectiveRouter();

    /**
     * Add mappings from from handler's @c NamespaceAndName values to @c BlockingPolicy values, gotten through the
     * handler's getConfiguration() method. If a mapping for any of the specified @c NamespaceAndName values already
     * exists the entire call is refused.
     *
     * @param handler The handler to add.
     * @return Whether the handler was added.
     */
    bool addDirectiveHandler(std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> handler);

    /**
     * Remove the specified mappings from @c NamespaceAndName values to @c BlockingPolicy values, gotten through the
     * handler's getConfiguration() method. If any of the specified mappings do not match an existing mapping, the
     * entire operation is refused.
     *
     * @param handler The handler to remove.
     * @return Whether the configuration was removed.
     */
    bool removeDirectiveHandler(std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> handler);

    /**
     * Invoke @c handleDirectiveImmediately() on the handler registered for the given @c AVSDirective.
     *
     * @param directive The directive to be handled immediately.
     * @return Whether or not the handler was invoked.
     */
    bool handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive);

    /**
     * Invoke @c preHandleDirective() on the handler registered for the given @c AVSDirective.
     *
     * @param directive The directive to be preHandled.
     * @param result A result object to receive notification about the completion (or failure) of handling
     * the @c AVSDirective.
     * @return Whether or not the handler was invoked.
     */
    bool preHandleDirective(
        std::shared_ptr<avsCommon::avs::AVSDirective> directive,
        std::unique_ptr<avsCommon::sdkInterfaces::DirectiveHandlerResultInterface> result);

    /**
     * Invoke @c handleDirective() on the handler registered for the given @c AVSDirective.
     *
     * @param directive The directive to be handled.
     * @return @c true if the the registered handler returned @c true.  @c false if there was no registered handler
     * or the registered handler returned @c false (indicating that the directive was not recognized.
     */
    bool handleDirective(const std::shared_ptr<avsCommon::avs::AVSDirective>& directive);

    /**
     * Invoke cancelDirective() on the handler registered for the given @c AVSDirective.
     *
     * @param directive The directive to be handled.
     * @return Whether or not the handler was invoked.
     */
    bool cancelDirective(std::shared_ptr<avsCommon::avs::AVSDirective> directive);

    /**
     * Get the policy associated with the given directive.
     *
     * @param directive The directive for which the policy is required.
     * @return The corresponding @c BlockingPolicy value for the directive.
     */
    avsCommon::avs::BlockingPolicy getPolicy(const std::shared_ptr<avsCommon::avs::AVSDirective>& directive);

private:
    void doShutdown() override;

    /**
     * The lifecycle of instances of this class are used to set-up and tear-down around a call to a
     * @c DirectiveHandlerInterface method.  In particular, while instantiated it increments the reference count of
     * uses of the handler and releases m_mutex via a @c std::unique_lock so the mutex is not held during the call.
     * When destroyed m_mutex is re-acquired and the reference count for the handler is decremented.  When the
     * reference count goes to zero, the handler's @c onDeregistered() method in invoked.
     */
    class HandlerCallScope {
    public:
        /**
         * Constructor.
         * @note This constructor must only be called by threads that have acquired @c m_mutex.  When this constructor
         * exits @c m_mutex will be unlocked.
         *
         * @param lock The @c std::unique_lock to use to release and re-acquire @c m_mutex.
         * @param router The @c DirectiveRouter instance the will make the call.
         * @param handler The @c DirectiveHandlerInterface instance to call.
         */
        HandlerCallScope(
            std::unique_lock<std::mutex>& lock,
            DirectiveRouter* router,
            std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> handler);

        /**
         * Destructor.
         * @note This destructor must be called with @c m_mutex released.  When this destructor exits @c m_mutex will
         * have been re-acquired.
         */
        ~HandlerCallScope();

    private:
        /// The lock used to release and re-acquire @c m_mutex.
        std::unique_lock<std::mutex>& m_lock;

        /// The @c DirectiveRouter instance the will make the call.
        DirectiveRouter* m_router;

        /// The @c DirectiveHandlerInterface instance to call.
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> m_handler;
    };

    /**
     * Look up the configured @c HandlerAndPolicy value for the specified @c AVSDirective.
     * @note The calling thread must have already acquired @c m_mutex.
     *
     * @param directive The directive to look up a value for.
     * @return The corresponding @c HandlerAndPolicy value for the specified directive.
     */
    avsCommon::avs::HandlerAndPolicy getdHandlerAndPolicyLocked(
        const std::shared_ptr<avsCommon::avs::AVSDirective>& directive);

    /**
     * Get the @c DirectiveHandler for this directive.
     * @note The calling thread must have already acquired @c m_mutex.
     *
     * @param directive The @c AVSDirective for which we're looking for handler.
     * @return The directive handler for success. @c nullptr in failure.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> getHandlerLocked(
        std::shared_ptr<avsCommon::avs::AVSDirective> directive);

    /**
     * Increment the reference count for the specified handler.
     *
     * @param handler The handler for which the reference count should be incremented.
     */
    void incrementHandlerReferenceCountLocked(
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> handler);

    /**
     * Decrement the reference count for the specified handler.  If the reference count goes to zero, call
     * the handlers onDeregistered() method.
     *
     * @param lock The @c std::unique_lock used to release and re-acquire @c m_mutex if @c onDeregistered() is called.
     * @param handler The @c DirectiveHandlerInterface instance whose reference count is to be decremented.
     */
    void decrementHandlerReferenceCountLocked(
        std::unique_lock<std::mutex>& lock,
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> handler);

    /**
     * Remove the specified mappings from @c NamespaceAndName values to @c BlockingPolicy values, gotten through the
     * handler's getConfiguration() method. If any of the specified mappings do not match an existing mapping, the
     * entire operation is refused.  This function should be called while holding m_mutex.
     *
     * @param handler The handler to remove.
     * @return @c true if @c handler was removed successfully, else @c false indicates an error occurred.
     */
    bool removeDirectiveHandlerLocked(std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> handler);

    /// A mutex used to serialize access to @c m_configuration and @c m_handlerReferenceCounts.
    std::mutex m_mutex;

    /// Mapping from @c NamespaceAndName to @c PolicyAndHandler.
    std::unordered_map<avsCommon::avs::NamespaceAndName, avsCommon::avs::HandlerAndPolicy> m_configuration;

    /**
     * Instances of DirectiveHandlerInterface may receive calls after @c removeDirectiveHandlers() because
     * @ removeDirectiveHandlers() does not wait for any outstanding calls to complete.  To provide notification
     * that no more calls will be received, a reference count is maintained for each directive handler.  These
     * counts are incremented when a handler is added to @c m_configuration or when a call to a handler is in
     * progress.  These counts are decremented when the handler is removed from @c m_configuration or a call to
     * a handler returns.  When these count goes to zero the handler's @c onDeRegistered() method is invoked,
     * indicating that the handler will no longer be called (unless, of course, it is re-registered).
     */
    std::unordered_map<std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface>, int>
        m_handlerReferenceCounts;
};

}  // namespace adsl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_DIRECTIVEROUTER_H_
