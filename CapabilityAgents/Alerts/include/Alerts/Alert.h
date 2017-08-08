/*
 * Alert.h
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_ALERTS_INCLUDE_ALERTS_ALERT_H_
#define ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_ALERTS_INCLUDE_ALERTS_ALERT_H_

#include "Alerts/AlertObserverInterface.h"
#include "Alerts/Renderer/Renderer.h"
#include "Alerts/Renderer/RendererObserverInterface.h"

#include <AVSCommon/AVS/FocusState.h>
#include <AVSCommon/Utils/Timing/Timer.h>

#include <map>
#include <string>
#include <vector>

#include <rapidjson/document.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {

// forward declaration for the friend relationship.
namespace storage {
class SQLiteAlertStorage;
}

/**
 * A class to manage the concept of an AVS Alert.
 *
 * This class is decoupled from the Renderer, which is set by its owning object.  This class encpasulates and
 * translates all renderer states, so that an owning object need only know if the alert object is 'active', for
 * example, rather than also query rendering state.  An alert object in an 'active' state implies the user
 * perceivable rendering is occurring (whether that means audible, visual, or other perceivable stimulus).
 */
class Alert : public renderer::RendererObserverInterface {
public:
    /**
     * An enum class which captures the state an alert object can be in.
     */
    enum class State {
        /// An uninitialized value.
        UNSET,
        /// The alert is set and is expected to become active at some point in the future.
        SET,
        /// The alert has been activated, but is not yet active.
        ACTIVATING,
        /// The alert is active from a user's point of view.
        ACTIVE,
        /// The alert is active, but has been asked to snooze.
        SNOOZING,
        /// The alert is active, but is now stopping due to user interaction.
        SNOOZED,
        /// The renderer is now stopped due to a snooze request.
        STOPPING,
        /// The alert has stopped in response to user interaction.
        STOPPED,
        /// The alert has completed on its own, without user interaction.
        COMPLETED
    };

    /**
     * An enum class which captures the reasons an alert may have stopped.
     */
    enum class StopReason {
        /// An uninitalized value.
        UNSET,
        /// The alert has been stopped due to a Directive from AVS.
        AVS_STOP,
        /// The alert has been stopped due to a local user action.
        LOCAL_STOP,
        /// The alert is being stopped due to an SDK shutdown operation.
        SHUTDOWN
    };

    /**
     * An enum class which captures the various JSON parse states which may occur.
     */
    enum class ParseFromJsonStatus {
        /// Parsing was successful.
        OK,
        /// A required property was missing from the JSON.
        MISSING_REQUIRED_PROPERTY,
        /// An invalid value was detected while parsing the JSON.
        INVALID_VALUE
    };

    /**
     * A utility function to convert an alert state enum value to a string.
     *
     * @param state The alert state.
     * @return The string conversion.
     */
    static std::string stateToString(Alert::State state);

    /**
     * A utility function to convert a StopReason enum value to a string.
     *
     * @param stopReason The stop reason.
     * @return The string conversion.
     */
    static std::string stopReasonToString(Alert::StopReason stopReason);

    /**
     * A utility function to convert a ParseFromJsonStatus enum value to a string.
     *
     * @param parseFromJsonStatus The parse status.
     * @return The string conversion.
     */
    static std::string parseFromJsonStatusToString(Alert::ParseFromJsonStatus parseFromJsonStatus);

    /**
     * A utility struct which allows alert objects to be sorted uniquely by time in STL containers such as sets.
     */
    struct TimeComparator {
        /**
         * Alerts may have the same time stamp, so let's include the token to ensure unique and consistent ordering.
         */
        bool operator () (const std::shared_ptr<Alert>& lhs, const std::shared_ptr<Alert>& rhs) const {
            if (lhs->m_scheduledTime_Unix == rhs->m_scheduledTime_Unix) {
                return (lhs->m_token < rhs->m_token);
            }

            return (lhs->m_scheduledTime_Unix < rhs->m_scheduledTime_Unix);
        }
    };

    /**
     * Constructor.
     */
    Alert();

    /**
     * Destructor.
     */
    virtual ~Alert();

    /**
     * Returns a string to identify the type of the class.  Required for persistent storage.
     *
     * @return The type name of the alert.
     */
    virtual std::string getTypeName() const = 0;

    /**
     * Returns the file path for the alert's default audio file.
     *
     * @return The file path for the alert's default audio file.
     */
    virtual std::string getDefaultAudioFilePath() const = 0;

    /**
     * Returns the file path for the alert's short (backgrounded) audio file.
     *
     * @return The file path for the alert's short (backgrounded) audio file.
     */
    virtual std::string getDefaultShortAudioFilePath() const = 0;

    /**
     * Given a rapidjson pre-parsed Value, parse the fields required for a valid alert.
     *
     * @param payload The pre-parsed rapidjson::Value.
     * @param[out] errorMessage An output parameter where a parse error message may be stored.
     * @return The status of the parse.
     */
    ParseFromJsonStatus parseFromJson(const rapidjson::Value & payload, std::string* errorMessage);

    /**
     * Set the renderer on the alert.
     *
     * @param renderer The renderer to set on the alert.
     */
    void setRenderer(std::shared_ptr<renderer::RendererInterface> renderer);

    /**
     * Set an observer on the alert.  An alert may have only one observer - repeated calls to this function will
     * replace any previous value with the new one.
     *
     * @param observer The observer to set on this alert.
     */
    void setObserver(AlertObserverInterface* observer);

    /**
     * Sets the focus state for the alert.
     *
     * @param focusState The focus state.
     */
    void setFocusState(avsCommon::avs::FocusState focusState);

    /**
     * Sets the state of this alert to active.  Only has effect if the Alert's state is State::ACTIVATING.
     */
    bool setStateActive();

    /**
     * Sets the alert back to being set to go off in the future.
     */
    void reset();

    /**
     * Activate the alert.
     */
    void activate();

    /**
     * Deactivate the alert.
     *
     * @param reason The reason why the alert is being stopped.
     */
    void deActivate(StopReason reason);

    void onRendererStateChange(renderer::RendererObserverInterface::State state, const std::string & reason) override;

    /**
     * Returns the AVS token for the alert.
     *
     * @return The AVS token for the alert.
     */
    std::string getToken() const;

    /**
     * Gets the time the alert should occur, in Unix epoch time.
     *
     * @return The time the alert should occur, in Unix epoch time.
     */
    int64_t getScheduledTime_Unix() const;

    /**
     * Gets the time the alert should occur, in ISO-8601 format.
     *
     * @return The time the alert should occur, in ISO-8601 format.
     */
    std::string getScheduledTime_ISO_8601() const;

    /**
     * Returns the state of the alert.
     *
     * @return The state of the alert.
     */
    Alert::State getState() const;

    /**
     * Returns the reason the alert stopped.
     *
     * @return The reason the alert stopped.
     */
    StopReason getStopReason() const;

    /**
     * Returns the database id for the alert, if one is set.
     *
     * @return The database id for the alert, if one is set.
     */
    int getId() const;

    /**
     * A utility function to print the internals of an alert.
     */
    void printDiagnostic();

    /**
     * Performs relevant operations to snooze this alarm to the new time provided.
     *
     * @param updatedScheduledTime_ISO_8601 The new time for the alarm to occur, in ISO-8601 format.
     */
    void snooze(const std::string & updatedScheduledTime_ISO_8601);

private:
    /// A friend relationship, since our storage interface needs access to all fields.
    friend class storage::SQLiteAlertStorage;

    /**
     * Utility function to begin the alert's renderer.
     */
    void startRenderer();

    /**
     * A utility function to be invoked when the maximum time for an alert has expired.
     */
    void onMaxTimerExpiration();

    /// The AVS token for the alert.
    std::string m_token;
    /// The database id for the alert.
    int m_dbId;
    /// The scheduled time for the alert in ISO-8601 format.
    std::string m_scheduledTime_ISO_8601;
    /// The scheduled time for the alert in Unix epoch format.
    /// TODO : ACSDK-392 to investigate updating this to std::chrono types for better portability & robustness.
    int64_t m_scheduledTime_Unix;
    /// The state of the alert.
    State m_state;
    /// The render state of the alert.
    renderer::RendererObserverInterface::State m_rendererState;
    /// The reason the alert has been stopped.
    StopReason m_stopReason;
    /// the AVS-context string for the alert.
    std::string m_avsContextString;
    /// The current focus state of the alert.
    avsCommon::avs::FocusState m_focusState;
    /// The renderer for the alert.
    std::shared_ptr<renderer::RendererInterface> m_renderer;
    /// The observer of the alert.
    AlertObserverInterface* m_observer;
    /// A flag to capture if the maximum time timer has expired for this alert.
    bool m_hasTimerExpired;
    /// The timer to ensure this alert is not active longer than a maximum length of time.
    avsCommon::utils::timing::Timer m_maxLengthTimer;
};

} // namespace alerts
} // namespace capabilityAgents
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_ALERTS_INCLUDE_ALERTS_ALERT_H_
