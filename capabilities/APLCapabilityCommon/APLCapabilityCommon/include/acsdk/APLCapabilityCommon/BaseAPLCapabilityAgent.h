/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Amazon Software License (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     https://aws.amazon.com/asl/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#ifndef ACSDK_APLCAPABILITYCOMMON_BASEAPLCAPABILITYAGENT_H_
#define ACSDK_APLCAPABILITYCOMMON_BASEAPLCAPABILITYAGENT_H_

#include <chrono>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <unordered_set>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/Timing/Timer.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/Metrics/DataPointDurationBuilder.h>
#include <AVSCommon/Utils/Metrics/DataPointCounterBuilder.h>
#include <AVSCommon/Utils/Optional.h>
#include <acsdk/Notifier/Notifier.h>

#include <acsdk/APLCapabilityCommonInterfaces/APLCapabilityAgentInterface.h>
#include <acsdk/APLCapabilityCommonInterfaces/APLCapabilityAgentNotifierInterface.h>
#include <acsdk/APLCapabilityCommonInterfaces/APLCapabilityAgentObserverInterface.h>
#include <acsdk/APLCapabilityCommonInterfaces/PresentationSession.h>
#include <acsdk/APLCapabilityCommonInterfaces/VisualStateProviderInterface.h>

namespace alexaClientSDK {
namespace aplCapabilityCommon {
/**
 * This base class for an Alexa Presentation Language @c CapabilityAgent that handles rendering APL documents.
 *
 * Clients interested in APL events can subscribe themselves as an observer, and the
 * clients will be notified via the Capability Agent Observer interface.
 */
class BaseAPLCapabilityAgent
        : public alexaClientSDK::avsCommon::avs::CapabilityAgent
        , public alexaClientSDK::avsCommon::utils::RequiresShutdown
        , public alexaClientSDK::avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public alexaClientSDK::aplCapabilityCommonInterfaces::APLCapabilityAgentInterface
        , public alexaClientSDK::notifier::Notifier<aplCapabilityCommonInterfaces::APLCapabilityAgentObserverInterface>
        , public std::enable_shared_from_this<BaseAPLCapabilityAgent> {
public:
    /**
     * Constructor.
     *
     * @param avsNamepace The AVS namespace interface this CA operations within.
     * @param exceptionSender The object to use for sending AVS Exception messages.
     * @param metricRecorder The object to use for recording metrics.
     * @param messageSender The @c MessageSenderInterface that sends events to AVS.
     * @param contextManager The @c ContextManagerInterface used to generate system context for events.
     * @param APLMaxVersion The APL version supported.
     * @param visualStateProvider The @c VisualStateProviderInterface used to request visual context.
     */
    BaseAPLCapabilityAgent(
        const std::string& avsNamespace,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        const std::string& APLMaxVersion,
        std::shared_ptr<alexaClientSDK::aplCapabilityCommonInterfaces::VisualStateProviderInterface>
            visualStateProvider = nullptr);

    /**
     * Destructor.
     */
    virtual ~BaseAPLCapabilityAgent() = default;

    /// Device SDK Facing Interfaces
    /// @name CapabilityAgent/DirectiveHandlerInterface Functions
    /// @{
    void handleDirectiveImmediately(std::shared_ptr<alexaClientSDK::avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<DirectiveInfo> info) override;
    alexaClientSDK::avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    /// @}

    /// @name CapabilityConfigurationInterface Functions
    /// @{
    std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityConfiguration>>
    getCapabilityConfigurations() override;
    /// @}

    /// @name ContextRequesterInterface Functions
    /// @{
    void onContextAvailable(const std::string& jsonContext) override;
    void onContextFailure(const alexaClientSDK::avsCommon::sdkInterfaces::ContextRequestError error) override;
    /// @}

    /// @name StateProviderInterface Functions
    /// @{
    void provideState(
        const alexaClientSDK::avsCommon::avs::NamespaceAndName& stateProviderName,
        unsigned int stateRequestToken) override;
    /// @}

    // @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /// @name APLCapabilityAgentInterface functions
    /// @{
    void onActiveDocumentChanged(
        const aplCapabilityCommonInterfaces::PresentationToken& token,
        const alexaClientSDK::aplCapabilityCommonInterfaces::PresentationSession& session) override;
    void clearExecuteCommands(
        const aplCapabilityCommonInterfaces::PresentationToken& token = std::string(),
        const bool markAsFailed = true) override;
    void sendUserEvent(const aplCapabilityCommonInterfaces::aplEventPayload::UserEvent& eventPayload) override;
    void sendDataSourceFetchRequestEvent(
        const aplCapabilityCommonInterfaces::aplEventPayload::DataSourceFetch& fetchPayload) override;
    void sendRuntimeErrorEvent(const aplCapabilityCommonInterfaces::aplEventPayload::RuntimeError& errors) override;
    void onVisualContextAvailable(
        avsCommon::sdkInterfaces::ContextRequestToken requestToken,
        const aplCapabilityCommonInterfaces::aplEventPayload::VisualContext& context) override;
    void processRenderDocumentResult(
        const aplCapabilityCommonInterfaces::PresentationToken& token,
        const bool result,
        const std::string& error) override;
    void processExecuteCommandsResult(
        const aplCapabilityCommonInterfaces::PresentationToken& token,
        aplCapabilityCommonInterfaces::APLCommandExecutionEvent event,
        const std::string& error) override;
    void recordRenderComplete(const std::chrono::steady_clock::time_point& timestamp) override;
    void proactiveStateReport() override;
    /// @}

    /**
     * Intialize APL CA based on the configurations
     */
    virtual bool initialize();

    /// Tests Facing interfaces
    /**
     * Set the executor used as the worker thread
     * @param executor The @c Executor to set
     * @note This function should only be used for testing purposes. No call to any other method should
     * be done prior to this call.
     */
    void setExecutor(const std::shared_ptr<alexaClientSDK::avsCommon::utils::threading::Executor>& executor);

    /// Concrete Implementation Facing Interfaces
protected:
    /// Directive Types that could be received from AVS
    enum class DirectiveType {
        /// Directive contains an APL document to be rendered
        RENDER_DOCUMENT,

        /// Directive indicates that a previously received document should be now displayed
        SHOW_DOCUMENT,

        /// Directive contains one or multiple APL commands to be executed
        EXECUTE_COMMAND,

        /// Directive indicates that token should be updated
        DYNAMIC_TOKEN_DATA_SOURCE_UPDATE,

        /// Directive indicates that index should be updated
        DYNAMIC_INDEX_DATA_SOURCE_UPDATE,

        /// Unknown directive received
        UNKNOWN
    };

    /// Enumeration of timer metrics events that could be emitted
    enum class MetricEvent {
        /// Metric to record time-taken to render document
        RENDER_DOCUMENT
    };

    /// Enumeration of timer metric activity names that could be emitted
    enum class MetricActivity {
        /// When render document has completed successfully
        ACTIVITY_RENDER_DOCUMENT,

        /// When render document fails
        ACTIVITY_RENDER_DOCUMENT_FAIL,
    };

    /**
     * PresentationSession field names in RenderDocumentDirective
     */
    struct PresentationSessionFieldNames {
        /// SkillId field name
        std::string skillId;
        /// Presentation session id field name
        std::string presentationSessionId;
    };

    /// Template Methods to be implemented in Concrete Implementations
    /**
     * Get specific directive handler configuration for this APL Capability Agent
     */
    virtual alexaClientSDK::avsCommon::avs::DirectiveHandlerConfiguration getAPLDirectiveConfiguration() const = 0;

    /**
     * Get specific capability configuration for this APL Capability Agent
     */
    virtual std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityConfiguration>>
    getAPLCapabilityConfigurations(const std::string& APLMaxVersion) = 0;

    /**
     * Get @c DirectiveType from Directive header information
     */
    virtual DirectiveType getDirectiveType(std::shared_ptr<DirectiveInfo> info) = 0;

    /**
     * Get root key for the configuration values in the AVS json configuration
     */
    virtual const std::string& getConfigurationRootKey() = 0;

    /**
     * Given a Metric Event, provide the metric data point name to publish.
     */
    virtual const std::string& getMetricDataPointName(MetricEvent event) = 0;

    /**
     * Given a Metric Activity, provide the metric data point name to publish.
     */
    virtual const std::string& getMetricActivityName(MetricActivity activity) = 0;

    /**
     * In presentation session part of RenderDocument directives, there are some field names that
     * differ slightly between CAs, the derived class may override this function to provide the field names that
     * represent these fields in the directive.
     */
    virtual PresentationSessionFieldNames getPresentationSessionFieldNames() = 0;

    /**
     * Whether AVS events should include presentationSession in their payload.
     */
    virtual const bool shouldPackPresentationSessionToAvsEvents() = 0;

    /**
     * This function handles any unknown directives received.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void handleUnknownDirective(std::shared_ptr<DirectiveInfo> info);

    /**
     * Send the handling completed notification and clean up the resources.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void setHandlingCompleted(std::shared_ptr<DirectiveInfo> info);

    /**
     * Get the executor used as the worker thread
     * @return The @c Executor shared_ptr
     */
    std::shared_ptr<alexaClientSDK::avsCommon::utils::threading::Executor> getExecutor();

private:
    /**
     * Remove a directive from the map of message IDs to DirectiveInfo instances.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective whose message ID is to be removed.
     */
    void removeDirective(std::shared_ptr<DirectiveInfo> info);

    /**
     * This function handles a @c RenderDocument directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void handleRenderDocumentDirective(std::shared_ptr<DirectiveInfo> info);

    /**
     * This function handles a @c ShowDocument directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void handleShowDocumentDirective(std::shared_ptr<DirectiveInfo> info);

    /**
     * This function handles a @c ExecuteCommand directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void handleExecuteCommandDirective(std::shared_ptr<DirectiveInfo> info);

    /**
     * This function handles a @c Dynamic source data related directives.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param sourceType Dynamic source type.
     */
    void handleDynamicListDataDirective(std::shared_ptr<DirectiveInfo> info, const std::string& sourceType);

    /**
     * This function handles the notification of the ExecuteCommands callbacks to all the observers.  This function
     * is intended to be used in the context of @c m_executor worker thread.
     *
     * @param info The directive to be handled.
     */
    void executeExecuteCommand(std::shared_ptr<DirectiveInfo> info);

    /**
     * This function handles the notification of the DataSourceUpdate callbacks to all the observers.  This
     * function is intended to be used in the context of @c m_executor worker thread.
     *
     * @param info The directive to be handled.
     * @param sourceType Data source type.
     */
    void executeDataSourceUpdate(std::shared_ptr<DirectiveInfo> info, const std::string& sourceType);

    /**
     * * This function handles the notification of the RenderDocument callbacks to all the observers.  This function
     * is intended to be used in the context of @c m_executor worker thread.
     *
     * @param info The directive to be handled.
     */
    void executeRenderDocument(
        const std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent::DirectiveInfo> info);

    /**
     *
     * Stops the execution of all pending @c ExecuteCommand directive
     * @param reason reason for clearing commands
     * @param token The token. This should be passed in if we are clearing execute commands due to APL-specific trigger
     * (eg. Finish command). This should be left empty if we are clearing due to global triggers (eg. back navigation)
     * @param markAsFailed Whether to mark the cleared commands as failed.
     */
    void executeClearExecuteCommands(
        const std::string& reason,
        const aplCapabilityCommonInterfaces::PresentationToken& token = std::string(),
        const bool markAsFailed = true);

    /**
     * Queue an AVS event to be sent when context is available
     * @param avsNamespace namespace of the event
     * @param name name of the event
     * @param payload payload of the event
     */
    void executeSendEvent(const std::string& avsNamespace, const std::string& name, const std::string& payload);

    /**
     * Internal function for handling @c ContextManager request for context
     * @param stateRequestToken The request token.
     */
    void executeProvideState(unsigned int stateRequestToken);

    /**
     * Checks if a proactive state report is required and requests state if necessary
     */
    void executeProactiveStateReport();

    /**
     * Extracts and process visual context received from clients. The funciton
     * constructs the RenderDocumentState payload and is intended to be used
     * in the context of @c m_executor worker thread.
     *
     * @param token presentationToken of document
     * @param requestToken token correlating context to provideState request
     * @param context The visual state to be passed to AVS.
     */
    void executeOnVisualContextAvailable(
        const avsCommon::sdkInterfaces::ContextRequestToken requestToken,
        const aplCapabilityCommonInterfaces::aplEventPayload::VisualContext& context);

    /**
     * Adds presentationSession payload to provided document.
     * @param document the document to add the payload to.
     */
    void addPresentationSessionPayload(rapidjson::Document* document);

    /**
     * Returns string payload of presentationSession object.
     * @return the presentationSession object payload.
     */
    std::string getPresentationSessionPayload();

    /**
     * @name Executor Thread Variables
     *
     * These member variables are only accessed by functions in the @c m_executor worker thread, and do not require any
     * synchronization.
     */
    /// @{
    /// The directive corresponding to the RenderDocument directive.
    std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent::DirectiveInfo> m_lastDisplayedDirective;

    /// The last executeCommand directive.
    std::pair<std::string, std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent::DirectiveInfo>>
        m_lastExecuteCommandTokenAndDirective;
    /// @}

    /// The object to use for sending events.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /// Token of the last template if it was an APL one. Otherwise, empty
    aplCapabilityCommonInterfaces::PresentationToken m_lastRenderedAPLToken;

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The @c VisualStateProvider for requesting visual state.
    std::shared_ptr<alexaClientSDK::aplCapabilityCommonInterfaces::VisualStateProviderInterface> m_visualStateProvider;

    /// The queue of events to be sent to AVS.
    std::queue<std::tuple<std::string, std::string, std::string>> m_events;

    /// The APL version of the runtime.
    const std::string m_APLVersion;

    /**
     * Start recording or update @c metricsEvent
     *
     * @param Metrics event in context
     */
    void startMetricsEvent(MetricEvent metricEvent);

    /**
     * Stops recording and submit @c metricsEvent
     *
     * @param metricsEvent Metrics event in context
     * @param activity Metrics activity to be concluded
     * @param timestamp Timestamp when metric event ends
     */
    void endMetricsEvent(
        MetricEvent metricEvent,
        MetricActivity activity,
        const std::chrono::steady_clock::time_point& timestamp);

    /**
     * Reset @c metricsEvent
     *
     * @param metricsEvent Metrics event in context
     */
    void resetMetricsEvent(MetricEvent metricEvent);

    /// The @c MetricRecorder used to record useful metrics from the presentation layer.
    std::shared_ptr<alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface> m_metricRecorder;

    /// The mutex to ensure exclusivity over @c MetricRecorder
    std::mutex m_MetricsRecorderMutex;

    /// Stores the currently active time data points
    std::map<MetricEvent, std::chrono::steady_clock::time_point> m_currentActiveTimePoints;

    /// The last state which was reported to AVS
    std::string m_lastReportedState;

    /// The time of the last state report
    std::chrono::time_point<std::chrono::steady_clock> m_lastReportTime;

    /// The minimum state reporting interval
    std::chrono::milliseconds m_minStateReportInterval{};

    /// The state reporting check interval
    std::chrono::milliseconds m_stateReportCheckInterval{};

    /// Whether the state has been requested from the state provider and we are awaiting the response
    bool m_stateReportPending;

    /// An internal timer used to check for context changes
    alexaClientSDK::avsCommon::utils::timing::Timer m_proactiveStateTimer;

    /// Whether the current document is fully rendered
    bool m_documentRendered;

    /// The current @c PresentationSession as set by the latest @c RenderDocument directive.
    alexaClientSDK::aplCapabilityCommonInterfaces::PresentationSession m_presentationSession;

    /// Time at which the current document was received
    std::chrono::steady_clock::time_point m_renderReceivedTime;

    /// The AVS Namespace that directives/events/context will be published on
    const std::string m_avsNamespace;

    /// The Namespace/Name combo for RenderedDocumentState device context.  Stored for convenience.
    const alexaClientSDK::avsCommon::avs::NamespaceAndName m_visualContextHeader;

    /// This is the worker thread for the APL CA
    std::shared_ptr<alexaClientSDK::avsCommon::utils::threading::Executor> m_executor;
};

}  // namespace aplCapabilityCommon
}  // namespace alexaClientSDK

#endif  // ACSDK_APLCAPABILITYCOMMON_BASEAPLCAPABILITYAGENT_H_
