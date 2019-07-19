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

#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGEVENTSENDERINTERFACE_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGEVENTSENDERINTERFACE_H_

#include <future>
#include <string>

namespace alexaClientSDK {
namespace settings {

/*
 * A utility class used to send events to AVS in the goal of synchronizing the value of an associated @c
 * SettingInterface value.
 */
class SettingEventSenderInterface {
public:
    /**
     * Destructor.
     */
    virtual ~SettingEventSenderInterface() = default;

    /**
     * Sends a setting changed event to AVS.
     * This event follows the format:
     * @code{.json}
     *   {
     *    "event": {
     *      "header": {
     *        "namespace": "{eventNamespace}",
     *        "name": "{eventChangedName}",
     *        "messageId": "xxxxx"
     *      },
     *      "payload": {
     *        "{settingName}": yyyyy
     *      }
     *    }
     *  }
     * @endcode
     * The setting specific fields should be specified in a @c SettingEventMetadata passed as an argument on creation
     * this object.
     *
     * @param value The value of the setting. It should be a valid JSON string value.
     * @return A future expressing if the event has been guaranteed to be sent to AVS.
     */
    virtual std::shared_future<bool> sendChangedEvent(const std::string& value) = 0;

    /**
     * Sends a report setting event to AVS.
     *
     * The setting report event follows the format:
     * @code{.json}
     *   {
     *    "event": {
     *      "header": {
     *        "namespace": "{eventNamespace}",
     *        "name": "{eventReportName}",
     *        "messageId": "xxxxx"
     *      },
     *      "payload": {
     *        "{settingName}": yyyyy
     *      }
     *    }
     *  }
     * @endcode
     * The setting specific fields should be specified in a @c SettingEventMetadata passed as an argument on creation
     * this object.
     *
     * @param value The value of the setting. It should be a valid JSON string value.
     * @return A future expressing if the event has been guaranteed to be sent to AVS.
     */
    virtual std::shared_future<bool> sendReportEvent(const std::string& value) = 0;
};

}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGEVENTSENDERINTERFACE_H_
