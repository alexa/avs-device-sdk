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

#ifndef ACSDKALEXALAUNCHERINTERFACES_ALEXALAUNCHERINTERFACE_H_
#define ACSDKALEXALAUNCHERINTERFACES_ALEXALAUNCHERINTERFACE_H_

#include <string>
#include <memory>

#include "acsdkAlexaLauncherInterfaces/AlexaLauncherTargetState.h"
#include "acsdkAlexaLauncherInterfaces/AlexaLauncherObserverInterface.h"

namespace alexaClientSDK {
namespace acsdkAlexaLauncherInterfaces {

/**
 * The AlexaLauncherInterface carries out launcher actions such as 'LaunchTarget'
 *
 * An implementation of the AlexaLauncherInterface lets the user control the launcher related interactions.
 * This can be called by multiple callers; for example the AlexaLauncher Capability Agent or the application’s
 * GUI.
 *
 * This interface can report to subscribers implementing the @c AlexaLauncherObserverInterface when there is a
 * target state change.
 *
 * @note Implementations of this interface must be thread-safe.
 */
class AlexaLauncherInterface {
public:
    /**
     * Class for holding AlexaLauncher response for AlexaLauncher interface
     */
    struct Response {
        /**
         * Enum for the different error types understood by the AlexaLauncher capability agent.
         */
        enum class Type {
            /// Device processed command successfully without any errors
            SUCCESS,

            /// Indicates an additional confirmation must occur before the requested action can be completed.
            CONFIRMATION_REQUIRED,

            /// The operation can't be performed because the endpoint is already in operation.
            ALREADY_IN_OPERATION,

            /// The device does not have permissions to perform the specified action.
            INSUFFICIENT_PERMISSIONS,

            /// An error occurred that can't be described by one of the other error types. For example, a runtime
            /// exception occurred. We recommend that you always send a more specific error type.
            INTERNAL_ERROR,

            /// Indicates the target state value is not supported.
            INVALID_VALUE,

            /// The device can't be set to the specified target state because of its current mode of operation. For
            /// example, if the device is in airplane mode,
            NOT_SUPPORTED_IN_CURRENT_MODE
        };

        /// Default Constructor
        Response() : type(Type::INTERNAL_ERROR), errorMessage("") {
        }

        /**
         * Constructor.
         *
         * @param responseType The response type @c Type.
         * @param errorMessage The error message if @c type is other than `SUCCESS`.
         */
        Response(Type responseType, std::string message) : type(responseType), errorMessage(std::move(message)) {
        }

        /// The AlexaLauncher response type
        Type type;

        /// The error message for logging if the AlexaLauncher response type is any other than SUCCESS
        std::string errorMessage;
    };

    /**
     * Virtual destructor to assure proper cleanup of derived types.
     */
    virtual ~AlexaLauncherInterface() = default;

    /**
     * Launch target on the device.
     *
     * @param targetState The target to launch, this can be an app or a GUI shortcut.
     * @return A @c AlexaLauncherResponse to indicate whether target was successfully launched or encountered
     * an error while launching. @c AlexaLauncherResponse.responseType should return SUCCESS if there are no errors
     * while launching the target. Otherwise, it should return the corresponding error code along with a log
     * message @c AlexaLauncherResponse.errorMessage that would be logged in cloud for debugging purposes.
     */
    virtual Response launchTarget(const acsdkAlexaLauncherInterfaces::TargetState& targetState) = 0;

    /**
     * Get the current launcher target on the device.
     *
     * @note If @c AlexaLauncherInterface implementation has configured its instance's property as retrievable,
     * this method should return the current launch target state. Unlike @c AlexaLauncherObserverInterface
     * observer methods, this returns the current target state at any given point when requested, without a change
     * in target state.
     *
     * @return Return the current @c TargetState
     */
    virtual acsdkAlexaLauncherInterfaces::TargetState getLauncherTargetState() = 0;

    /**
     * Adds a @c AlexaLauncherObserverInterface observer.
     *
     * @note If @c AlexaLauncherInterface implementation has configured its instance's property as proactively
     * reported, then it is required to notify observers of @c AlexaLauncherObserverInterface for any change in its
     * property state. This includes notifying the value when the device starts, if it is different from the last
     * reported value.
     *
     * @param observer The reference to the @c AlexaLauncherObserverInterface.
     */
    virtual bool addObserver(const std::weak_ptr<AlexaLauncherObserverInterface>& observer) = 0;

    /**
     * Removes an observer of @c AlexaLauncherObserverInterface.
     *
     * @param observer The reference to the @c AlexaLauncherObserverInterface.
     */
    virtual void removeObserver(const std::weak_ptr<AlexaLauncherObserverInterface>& observer) = 0;
};

}  // namespace acsdkAlexaLauncherInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKALEXALAUNCHERINTERFACES_ALEXALAUNCHERINTERFACE_H_
