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

#ifndef ALEXA_CLIENT_SDK_ACSDKALERTSINTERFACES_INCLUDE_ACSDKALERTSINTERFACES_ALERTOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_ACSDKALERTSINTERFACES_INCLUDE_ACSDKALERTSINTERFACES_ALERTOBSERVERINTERFACE_H_

#include <iomanip>
#include <string>
#include <sstream>

#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace acsdkAlertsInterfaces {

/**
 * An interface for observing state changes on an Alert object.
 */
class AlertObserverInterface {
public:
    /// The minimum value for the field in @c OriginalTime.
    static const int ORIGINAL_TIME_FIELD_MIN = 0;
    /// The maximum value for the hour field in @c OriginalTime.
    static const int ORIGINAL_TIME_HOUR_MAX = 23;
    /// The maximum value for the minute field in @c OriginalTime.
    static const int ORIGINAL_TIME_MINUTE_MAX = 59;
    /// The maximum value for the second field in @c OriginalTime.
    static const int ORIGINAL_TIME_SECOND_MAX = 59;
    /// The maximum value for the millisecond field in @c OriginalTime.
    static const int ORIGINAL_TIME_MILLISECOND_MAX = 999;

    /**
     * Check whether a value is within the bounds.
     *
     * @param value The value to check.
     * @param minVal The minimum value.
     * @param maxVal The maximum value.
     * @param true if the value is within the bounds, false otherwise.
     */
    template <class T>
    static bool withinBounds(T value, T minVal, T maxVal) {
        return value >= minVal && value <= maxVal;
    }

    /**
     * An enum class to represent the states an alert can be in.
     */
    enum class State {
        /// The alert is ready to start, and is waiting for channel focus.
        READY,
        /// The alert has started.
        STARTED,
        /// The alert has stopped due to user or system intervention.
        STOPPED,
        /// The alert has snoozed.
        SNOOZED,
        /// The alert has completed on its own.
        COMPLETED,
        /// The alert has been determined to be past-due, and will not be rendered.
        PAST_DUE,
        /// The alert has entered the foreground.
        FOCUS_ENTERED_FOREGROUND,
        /// The alert has entered the background.
        FOCUS_ENTERED_BACKGROUND,
        /// The alert has encountered an error.
        ERROR,
        /// The alert has been deleted.
        DELETED,
        /// The alert has been scheduled to trigger at a future time.
        SCHEDULED_FOR_LATER
    };

    /**
     * An enum class to represent the type of an alert.
     */
    enum class Type {
        /// The alarm type.
        ALARM,
        /// The timer type.
        TIMER,
        /// The reminder type.
        REMINDER
    };

    /**
     * Struct that represents the local time in current timezone when the alert was originally set. If the timezone is
     * updated after the alert was set, the value of @c OriginalTime remains unchanged. Users have to check if all
     * alerts match the desired times after a timezone change. Also, snoozing or deferring an alert will not modify
     * the value of this struct. This struct is supposed to be read only by the alert observers and can be used for
     * display purpose on a screen-based device, e.g. displaying the original time on a ringing screen for an alarm.
     * For example, when a user says "Alexa, set an alarm at 5PM"(PST timezone), the originalTime string included in
     * SetAlert directive would be "17:00:00.000" and the corresponding scheduledTime in ISO 8601 format is
     * "2021-08-08T01:00:00+0000"(UTC timezone). When the alert is triggered and snoozed for 9 minutes, the
     * scheduledTime would be updated to "2021-08-08T01:09:00+0000" (UTC timezone) and the originalTime remains
     * unchanged as "17:00:00.000".
     * The @c OriginalTime should not be used to infer the date of the alert. The "scheduledTime" in @c AlertInfo should
     * be used for this purpose. The original time is an optional field in SetAlert directive and currently used for
     * ALARM and REMINDER types.
     */
    struct OriginalTime {
        /**
         * Constructor. All fields will be set to ORIGINAL_TIME_FIELD_MIN if an invalid value is provided.
         *
         * @param hour Hour within [ORIGINAL_TIME_FIELD_MIN, ORIGINAL_TIME_HOUR_MAX]
         * @param minute Minute within [ORIGINAL_TIME_FIELD_MIN, ORIGINAL_TIME_MINUTE_MAX]
         * @param second Second within [ORIGINAL_TIME_FIELD_MIN, ORIGINAL_TIME_SECOND_MAX]
         * @param millisecond Millisecond within [ORIGINAL_TIME_FIELD_MIN, ORIGINAL_TIME_MILLISECOND_MAX]
         */
        OriginalTime(
            int hour = ORIGINAL_TIME_FIELD_MIN,
            int minute = ORIGINAL_TIME_FIELD_MIN,
            int second = ORIGINAL_TIME_FIELD_MIN,
            int millisecond = ORIGINAL_TIME_FIELD_MIN) :
                hour{hour},
                minute{minute},
                second{second},
                millisecond{millisecond} {
            if (!withinBounds(hour, ORIGINAL_TIME_FIELD_MIN, ORIGINAL_TIME_HOUR_MAX) ||
                !withinBounds(minute, ORIGINAL_TIME_FIELD_MIN, ORIGINAL_TIME_MINUTE_MAX) ||
                !withinBounds(second, ORIGINAL_TIME_FIELD_MIN, ORIGINAL_TIME_SECOND_MAX) ||
                !withinBounds(millisecond, ORIGINAL_TIME_FIELD_MIN, ORIGINAL_TIME_MILLISECOND_MAX)) {
                this->hour = ORIGINAL_TIME_FIELD_MIN;
                this->minute = ORIGINAL_TIME_FIELD_MIN;
                this->second = ORIGINAL_TIME_FIELD_MIN;
                this->millisecond = ORIGINAL_TIME_FIELD_MIN;
            }
        }

        /**
         * Operator overload to compare two @c OriginalTime objects.
         *
         * @param rhs The right hand side of the == operation.
         * @return Whether or not this instance and @c rhs are equivalent.
         */
        bool operator==(const OriginalTime& rhs) const {
            return hour == rhs.hour && minute == rhs.minute && second == rhs.second && millisecond == rhs.millisecond;
        }

        /// Hours in [0-23].
        int hour;
        /// Minutes in [0-59].
        int minute;
        /// Seconds in [0-59].
        int second;
        /// Milliseconds in [0-999].
        int millisecond;
    };

    /**
     * This struct includes the information of an alert.
     * Note that attributes originalTime and label reflect the optional fields in the SetAlert directive.
     * @see https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/alerts.html
     * Refer to documentation of @c OriginalTime struct for details about originalTime. Attribute label includes the
     * content of the alert. For example, when the user creates a named timer "Alexa, set a coffee timer for 3 minutes",
     * "coffee" would be the label of the timer. When the user create a regular timer "Alexa, set a timer for 3
     * minutes", the label field would be empty in SetAlert directive. The label attribute can be used for display
     * purpose on a screen-based device, e.g. showing the label of the timer on an alert ringing screen.
     */
    struct AlertInfo {
        /**
         * Constructor.
         *
         * @param token An opaque token that uniquely identifies the alert.
         * @param type The type of the alert.
         * @param state The state of the alert.
         * @param scheduledTime The UTC timestamp for when the alert is scheduled.
         * @param originalTime An optional @c OriginalTime for the local time when the alert was originally set.
         * @param label An optional label for the content of an alert.
         * @param reason The reason for the state change.
         */
        AlertInfo(
            const std::string& token,
            const Type type,
            const State state,
            const std::chrono::system_clock::time_point& scheduledTime,
            const avsCommon::utils::Optional<OriginalTime>& originalTime = avsCommon::utils::Optional<OriginalTime>(),
            const avsCommon::utils::Optional<std::string>& label = avsCommon::utils::Optional<std::string>(),
            const std::string& reason = "") :
                token{token},
                type{type},
                state{state},
                scheduledTime{scheduledTime},
                originalTime{originalTime},
                label{label},
                reason{reason} {
        }

        /// An opaque token that uniquely identifies the alert.
        std::string token;
        /// The type of the alert.
        Type type;
        /// The state of the alert.
        State state;
        /// UTC timestamp for when the alert is scheduled.
        std::chrono::system_clock::time_point scheduledTime;
        /// An optional @c OriginalTime for the local time when the alert was originally set. This value remains
        /// unchanged when the alert is snoozed.
        avsCommon::utils::Optional<OriginalTime> originalTime;
        /// An optional label for the content of an alert.
        avsCommon::utils::Optional<std::string> label;
        /// The reason for the state change.
        std::string reason;
    };

    /**
     * Destructor.
     */
    virtual ~AlertObserverInterface() = default;

    /**
     * A callback function to notify an object that an alert has updated its state.
     *
     * @param alertInfo The information of the updated alert.
     */
    virtual void onAlertStateChange(const AlertInfo& alertInfo) = 0;

    /**
     * Convert a @c State to a @c std::string.
     *
     * @param state The @c State to convert.
     * @return The string representation of @c state.
     */
    static std::string stateToString(State state);

    /**
     * Convert a @c Type to a @c std::string.
     *
     * @param type The @c Type to convert.
     * @return The string representation of a @c Type.
     */
    static std::string typeToString(Type type);

    /**
     * Convert a @c OriginalTime to a @c std::string.
     *
     * @param type The @c OriginalTime to convert.
     * @return The string representation of a @c OriginalTime.
     */
    static std::string originalTimeToString(const OriginalTime& originalTime);
};

inline std::string AlertObserverInterface::stateToString(State state) {
    switch (state) {
        case State::READY:
            return "READY";
        case State::STARTED:
            return "STARTED";
        case State::STOPPED:
            return "STOPPED";
        case State::SNOOZED:
            return "SNOOZED";
        case State::COMPLETED:
            return "COMPLETED";
        case State::PAST_DUE:
            return "PAST_DUE";
        case State::FOCUS_ENTERED_FOREGROUND:
            return "FOCUS_ENTERED_FOREGROUND";
        case State::FOCUS_ENTERED_BACKGROUND:
            return "FOCUS_ENTERED_BACKGROUND";
        case State::ERROR:
            return "ERROR";
        case State::DELETED:
            return "DELETED";
        case State::SCHEDULED_FOR_LATER:
            return "SCHEDULED_FOR_LATER";
    }
    return "Unknown State";
}

inline std::string AlertObserverInterface::typeToString(Type type) {
    switch (type) {
        case Type::ALARM:
            return "ALARM";
        case Type::TIMER:
            return "TIMER";
        case Type::REMINDER:
            return "REMINDER";
    }
    return "Unknown Type";
}

inline std::string AlertObserverInterface::originalTimeToString(const OriginalTime& originalTime) {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << originalTime.hour << ":" << std::setfill('0') << std::setw(2)
       << originalTime.minute << ":" << std::setfill('0') << std::setw(2) << originalTime.second << "."
       << std::setfill('0') << std::setw(3) << originalTime.millisecond;
    return ss.str();
}

/**
 * Write a @c State value to an @c ostream.
 *
 * @param stream The stream to write the value to.
 * @param state The @c State value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const AlertObserverInterface::State& state) {
    return stream << AlertObserverInterface::stateToString(state);
}

/**
 * Write a @c Type value to an @c ostream.
 *
 * @param stream The stream to write the value to.
 * @param state The @c Type value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const AlertObserverInterface::Type& type) {
    return stream << AlertObserverInterface::typeToString(type);
}

/**
 * Write a @c OriginalTime value to an @c ostream.
 *
 * @param stream The stream to write the value to.
 * @param state The @c OriginalTime value to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const AlertObserverInterface::OriginalTime& originalTime) {
    return stream << AlertObserverInterface::originalTimeToString(originalTime);
}

}  // namespace acsdkAlertsInterfaces
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACSDKALERTSINTERFACES_INCLUDE_ACSDKALERTSINTERFACES_ALERTOBSERVERINTERFACE_H_
