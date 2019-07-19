/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/error/en.h>

#include "PlaybackController/PlaybackCommand.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace playbackController {

using namespace avsCommon::avs;
using namespace rapidjson;

/// String to identify log entries originating from this file.
static const std::string TAG("PlaybackCommand");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) utils::logger::LogEntry(TAG, event)

// PlaybackController interface 1.1 buttons event name.
static const std::string BUTTON_COMMAND_EVENT_NAME = "ButtonCommandIssued";

// PlaybackController interface 1.1 toggles event name.
static const std::string TOGGLE_COMMAND_EVENT_NAME = "ToggleCommandIssued";

// String to identify the AVS action SELECT string in @c ToggleCommandIssued event.
static const std::string PLAYBACK_TOGGLE_ACTION_SELECT = "SELECT";

// String to identify the AVS action DESELECT string in @c ToggleCommandIssued event.
static const std::string PLAYBACK_TOGGLE_ACTION_DESELECT = "DESELECT";

// String to identify the AVS name SHUFFLE string in @c ToggleCommandIssued event.
static const std::string PLAYBACK_TOGGLE_NAME_SHUFFLE = "SHUFFLE";

// String to identify the AVS name LOOP string in @c ToggleCommandIssued event.
static const std::string PLAYBACK_TOGGLE_NAME_LOOP = "LOOP";

// String to identify the AVS name REPEAT string in @c ToggleCommandIssued event.
static const std::string PLAYBACK_TOGGLE_NAME_REPEAT = "REPEAT";

// String to identify the AVS name THUMBS_UP string in @c ToggleCommandIssued event.
static const std::string PLAYBACK_TOGGLE_NAME_THUMBSUP = "THUMBSUP";

// String to identify the AVS name THUMBS_DOWN string in @c ToggleCommandIssued event.
static const std::string PLAYBACK_TOGGLE_NAME_THUMBSDOWN = "THUMBSDOWN";

// String to identify the AVS name UNKNOWN string in any event
static const std::string PLAYBACK_NAME_UNKNOWN = "UNKNOWN";

// event payload key for playback controller 1.1 buttons and toggles
static const std::string PLAYBACK_CONTROLLER_EVENT_NAME_KEY = "name";

// event payload key for playback controller 1.1 toggles
static const std::string PLAYBACK_CONTROLLER_EVENT_ACTION_KEY = "action";

// json empty object
static const std::string JSON_EMPTY_PAYLOAD = "{}";

// json open bracket
static const std::string JSON_BEGIN = "{\"";

// json colon sepeartor
static const std::string JSON_COLON = "\": \"";

// json comma seperator
static const std::string JSON_COMMA = "\", \"";

// json end bracket
static const std::string JSON_END = "\"}";

// In Playback Controller 1.0 -> 1.1, different button commands have different event payloads
/// @c PlayCommandIssued event.
static const ButtonCommand_v1_0 g_playButton_v1_0 = ButtonCommand_v1_0("PlayCommandIssued");

/// @c PauseCommandIssued event.
static const ButtonCommand_v1_0 g_pauseButton_v1_0 = ButtonCommand_v1_0("PauseCommandIssued");

/// @c NextCommandIssued event.
static const ButtonCommand_v1_0 g_nextButton_v1_0 = ButtonCommand_v1_0("NextCommandIssued");

/// @c PreviousCommandIssued event.
static const ButtonCommand_v1_0 g_previousButton_v1_0 = ButtonCommand_v1_0("PreviousCommandIssued");

/// @c SKIPFORWARD command.
static const ButtonCommand_v1_1 g_skipForwardButton_v1_0 = ButtonCommand_v1_1("SKIPFORWARD");

/// @c SKIPBACKWARD command.
static const ButtonCommand_v1_1 g_skipBackwardButton_v1_0 = ButtonCommand_v1_1("SKIPBACKWARD");

/// Unknown command
static const ButtonCommand_v1_1 g_unknownButton_v1_0 = ButtonCommand_v1_1(PLAYBACK_NAME_UNKNOWN);

/// @c SHUFFLE command with action = @c SELECT
static const ToggleCommand g_shuffleSelectToggle = ToggleCommand(PLAYBACK_TOGGLE_NAME_SHUFFLE, true);

/// @c SHUFFLE command with action = @c DESELECT
static const ToggleCommand g_shuffleDeselectToggle = ToggleCommand(PLAYBACK_TOGGLE_NAME_SHUFFLE, false);

/// @c LOOP command with action = @c SELECT
static const ToggleCommand g_loopSelectToggle = ToggleCommand(PLAYBACK_TOGGLE_NAME_LOOP, true);

/// @c LOOP command with action = @c DESELECT
static const ToggleCommand g_loopDeselectToggle = ToggleCommand(PLAYBACK_TOGGLE_NAME_LOOP, false);

/// @c REPEAT command with action = @c SELECT
static const ToggleCommand g_repeatSelectToggle = ToggleCommand(PLAYBACK_TOGGLE_NAME_REPEAT, true);

/// @c REPEAT command with action = @c DESELECT
static const ToggleCommand g_repeatDeselectToggle = ToggleCommand(PLAYBACK_TOGGLE_NAME_REPEAT, false);

/// @c THUMBSUP command with action = @c SELECT
static const ToggleCommand g_thumbsUpSelectToggle = ToggleCommand(PLAYBACK_TOGGLE_NAME_THUMBSUP, true);

/// @c THUMBSUP command with action = @c DESELECT
static const ToggleCommand g_thumbsUpDeselectToggle = ToggleCommand(PLAYBACK_TOGGLE_NAME_THUMBSUP, false);

/// @c THUMBSDOWN command with action = @c SELECT
static const ToggleCommand g_thumbsDownSelectToggle = ToggleCommand(PLAYBACK_TOGGLE_NAME_THUMBSDOWN, true);

/// @c THUMBSDOWN command with action = @c DESELECT
static const ToggleCommand g_thumbsDownDeselectToggle = ToggleCommand(PLAYBACK_TOGGLE_NAME_THUMBSDOWN, false);

/// Unknown toggle
static const ToggleCommand g_unknownToggle = ToggleCommand(PLAYBACK_NAME_UNKNOWN, false);

PlaybackCommand::PlaybackCommand(const std::string& name) : m_name(name) {
}

const PlaybackCommand& PlaybackCommand::buttonToCommand(PlaybackButton button) {
    switch (button) {
        case PlaybackButton::PLAY:
            return g_playButton_v1_0;
        case PlaybackButton::PAUSE:
            return g_pauseButton_v1_0;
        case PlaybackButton::NEXT:
            return g_nextButton_v1_0;
        case PlaybackButton::PREVIOUS:
            return g_previousButton_v1_0;
        case PlaybackButton::SKIP_FORWARD:
            return g_skipForwardButton_v1_0;
        case PlaybackButton::SKIP_BACKWARD:
            return g_skipBackwardButton_v1_0;
    }

    return g_unknownButton_v1_0;
}

const PlaybackCommand& PlaybackCommand::toggleToCommand(PlaybackToggle toggle, bool state) {
    switch (toggle) {
        case PlaybackToggle::LOOP:
            return (state ? g_loopSelectToggle : g_loopDeselectToggle);
        case PlaybackToggle::REPEAT:
            return (state ? g_repeatSelectToggle : g_repeatDeselectToggle);
        case PlaybackToggle ::SHUFFLE:
            return (state ? g_shuffleSelectToggle : g_shuffleDeselectToggle);
        case PlaybackToggle::THUMBS_DOWN:
            return (state ? g_thumbsDownSelectToggle : g_thumbsDownDeselectToggle);
        case PlaybackToggle::THUMBS_UP:
            return (state ? g_thumbsUpSelectToggle : g_thumbsUpDeselectToggle);
    }

    return g_unknownToggle;
}

std::string PlaybackCommand::toString() const {
    return m_name;
}

std::string ToggleCommand::toString() const {
    return m_name + "_" + getActionString();
}

std::ostream& operator<<(std::ostream& stream, const PlaybackCommand& command) {
    return stream << command.toString();
}

ButtonCommand_v1_0::ButtonCommand_v1_0(const std::string& name) : PlaybackCommand(name) {
}

std::string ButtonCommand_v1_0::getEventName() const {
    return m_name;
}

std::string ButtonCommand_v1_0::getEventPayload() const {
    return JSON_EMPTY_PAYLOAD;
}

ButtonCommand_v1_1::ButtonCommand_v1_1(const std::string& name) : PlaybackCommand(name) {
}

std::string ButtonCommand_v1_1::getEventName() const {
    return BUTTON_COMMAND_EVENT_NAME;
}

std::string ButtonCommand_v1_1::getEventPayload() const {
    return JSON_BEGIN + PLAYBACK_CONTROLLER_EVENT_NAME_KEY + JSON_COLON + m_name + JSON_END;
}

ToggleCommand::ToggleCommand(const std::string& name, bool action) : PlaybackCommand(name), m_action(action) {
}

std::string ToggleCommand::getEventName() const {
    return TOGGLE_COMMAND_EVENT_NAME;
}

std::string ToggleCommand::getEventPayload() const {
    return JSON_BEGIN + PLAYBACK_CONTROLLER_EVENT_NAME_KEY + JSON_COLON + m_name + JSON_COMMA +
           PLAYBACK_CONTROLLER_EVENT_ACTION_KEY + JSON_COLON + getActionString() + JSON_END;
}

std::string ToggleCommand::getActionString() const {
    return (m_action ? PLAYBACK_TOGGLE_ACTION_SELECT : PLAYBACK_TOGGLE_ACTION_DESELECT);
}

}  // namespace playbackController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
