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

#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGPROTOCOLINTERFACE_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGPROTOCOLINTERFACE_H_

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include <AVSCommon/Utils/Threading/Executor.h>

#include "Settings/SetSettingResult.h"
#include "Settings/SettingObserverInterface.h"

namespace alexaClientSDK {
namespace settings {

/**
 * Interface for the multiple setting management protocols.
 *
 * The setting protocol should ensure that events are sent to AVS as expected and the setting value is persisted. The
 * setting protocol also MUST only execute one request at a time.
 */
class SettingProtocolInterface {
public:
    /**
     * Callback function type used for applying new values.
     *
     * @return A pair where the first element indicates if the operation succeeded or not, and the second element is
     * a string representation of the final value.
     */
    using ApplyChangeFunction = std::function<std::pair<bool, std::string>()>;

    /**
     * Callback function type used for applying value retrieved from the database.
     *
     * @param dbValue A string representation of the value retrieved from the database. An empty string will be used
     * to indicate that no value was found in the database.
     * @return A pair where the first element indicates if the operation succeeded or not, and the second element is
     * a string representation of the final value.
     */
    using ApplyDbChangeFunction = std::function<std::pair<bool, std::string>(const std::string& dbValue)>;

    /**
     * Callback function type used to notify observers of whether the request succeeded or failed.
     *
     * @param notification The operation result.
     */
    using SettingNotificationFunction = std::function<void(SettingNotifications notification)>;

    /**
     * Callback function type used to revert the last value change.
     *
     * @return A string representation of the setting value after the revert operation.
     */
    using RevertChangeFunction = std::function<std::string()>;

    /**
     * Implements the protocol for changing a setting value through local UI.
     *
     * @param applyChange Function that can be used to apply the new value.
     * @param revertChange Function that can be used to revert the change. This function should only be called if the
     * apply change succeeded but some other part of the protocol has failed.
     * @param notifyObservers Function used to notify the observers of the protocol result.
     * @return Returns what was the set result.
     */
    virtual SetSettingResult localChange(
        ApplyChangeFunction applyChange,
        RevertChangeFunction revertChange,
        SettingNotificationFunction notifyObservers) = 0;

    /**
     * Implements the protocol for changing a setting value triggered by an AVS directive.
     *
     * @param applyChange Function that can be used to apply the new value.
     * @param revertChange Function that can be used to revert the change. This function should only be called if the
     * apply change succeeded but some other part of the protocol has failed.
     * @param notifyObservers Function used to notify the observers of the protocol result.
     * @return @c true if the directive was enqueued correctly; @c false otherwise.
     */
    virtual bool avsChange(
        ApplyChangeFunction applyChange,
        RevertChangeFunction revertChange,
        SettingNotificationFunction notifyObservers) = 0;

    /**
     * Implements the protocol for restoring a value from the storage.
     *
     * @param applyChange Function that can be used to apply the value restored from the storage. This function will
     * be called with empty string if no value was found in the database.
     * @param notifyObservers Function used to notify the observers of the protocol result.
     * @return @c true if the directive was enqueued correctly; @c false otherwise.
     */
    virtual bool restoreValue(ApplyDbChangeFunction applyChange, SettingNotificationFunction notifyObservers) = 0;

    /**
     * Destructor.
     */
    virtual ~SettingProtocolInterface() = default;
};

}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGPROTOCOLINTERFACE_H_
