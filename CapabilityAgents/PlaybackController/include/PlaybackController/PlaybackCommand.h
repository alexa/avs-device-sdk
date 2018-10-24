/*
 *
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_PLAYBACKCONTROLLER_INCLUDE_PLAYBACKCONTROLLER_PLAYBACKCOMMAND_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_PLAYBACKCONTROLLER_INCLUDE_PLAYBACKCONTROLLER_PLAYBACKCOMMAND_H_

#include <string>
#include <ostream>
#include <AVSCommon/AVS/PlaybackButtons.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace playbackController {

/**
 * This class represents a specific @c PlaybackController interface command.
 */
class PlaybackCommand {
public:
    /**
     * Constructor
     *
     * @param name The distinguishing command name. It is either the event name(v1.0), or the payload name(v1.1).
     */
    PlaybackCommand(const std::string& name);

    /**
     * Destructor.
     */
    virtual ~PlaybackCommand() = default;

    /**
     * Get the event name associated with this command.
     *
     * @return Event name.
     */
    virtual std::string getEventName() const = 0;

    /**
     * Get the @c Event's payload of this command
     *
     * @return @c Event's payload
     */
    virtual std::string getEventPayload() const = 0;

    /**
     * Write a @c command value to an @c ostream as a string.
     *
     * @param stream The stream to write the value to.
     * @param command The @c PlaybackCommand value to write to the @c ostream as a string.
     * @return The @c ostream that was passed in and written to.
     */
    friend std::ostream& operator<<(std::ostream&, const PlaybackCommand& command);

    /**
     * Convert a @c Button to a @c PlaybackCommand
     *
     * @param button a @c Button
     * @return The matched @c PlaybackCommand
     */
    static const PlaybackCommand& buttonToCommand(avsCommon::avs::PlaybackButton button);

    /**
     * Convert a @c Toggle to a @c PlaybackCommand
     *
     * @param toggle a @c Toggle
     * @return The matched @c PlaybackCommand
     */
    static const PlaybackCommand& toggleToCommand(avsCommon::avs::PlaybackToggle toggle, bool action);

protected:
    /// The distinct AVS name of a command
    const std::string m_name;

private:
    /**
     * Change the command objetc into a string
     *
     * @return The @c std::string name.
     */
    virtual std::string toString() const;
};

/**
 *  This class represents the @c PlaybackController 1.0 interface commands.
 */
class ButtonCommand_v1_0 : public PlaybackCommand {
public:
    /**
     * Constructor
     *
     * @param name Event name.
     */
    ButtonCommand_v1_0(const std::string& name);

    /// @name PlaybackCommand functions
    /// @{
    std::string getEventName() const override;

    std::string getEventPayload() const override;
    /// @}
};

/**
 *  This class represents a @c PlaybackController 1.1 interface ButtonCommandIssued.
 */
class ButtonCommand_v1_1 : public PlaybackCommand {
public:
    /**
     * Constructor
     *
     * @param name Event name.
     */
    ButtonCommand_v1_1(const std::string& name);

    /// @name PlaybackCommand functions
    /// @{
    std::string getEventName() const override;

    std::string getEventPayload() const override;
    /// @}
};

/**
 * This class represents a @c PlaybackController 1.1 interface @c ToggleCommandIssued.
 */
class ToggleCommand : public PlaybackCommand {
public:
    /**
     * Constructor
     *
     * @param name Command name.
     * @param action The action for this command. @ true indicates the *_SELECT variant of a command, and @ false
     * indicates the *_DESELECT variant.
     */
    ToggleCommand(const std::string& name, bool action);

    /// @name PlaybackCommand functions.
    /// @{
    std::string getEventName() const override;

    std::string getEventPayload() const override;
    ///@}

private:
    /**
     * Get the AVS string describing the action.
     * Only used for Toggle v1.1 interface
     * @return  The action string.
     */
    std::string getActionString() const;

    /**
     * Get the Toggle, and action string name
     *
     * @return  The action string.
     */
    std::string toString() const override;

    /**
     * The toggle action associated with this object. The action for this command. @ true indicates the *_SELECT variant
     * of a command, and @ false indicates the *_DESELECT variant.
     */
    bool m_action;
};

}  // namespace playbackController
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_PLAYBACKCONTROLLER_INCLUDE_PLAYBACKCONTROLLER_PLAYBACKCOMMAND_H_
