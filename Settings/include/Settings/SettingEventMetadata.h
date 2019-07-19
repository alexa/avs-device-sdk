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

#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGEVENTMETADATA_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGEVENTMETADATA_H_

#include <string>

namespace alexaClientSDK {
namespace settings {

/**
 * Specifies the parameters needed to construct setting changed and report events.
 */
struct SettingEventMetadata {
    /// The event namespace placed in the header.
    std::string eventNamespace;

    /// The name for the event indicating a value changed in the setting.
    std::string eventChangedName;

    /// The name for the event for reporting the value of a setting.
    std::string eventReportName;

    /// The string key in the payload to specify the value of the setting.
    std::string settingName;
};

}  // namespace settings
}  // namespace alexaClientSDK

#endif /* ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGEVENTMETADATA_H_ */
