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
#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETSETTINGRESULT_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETSETTINGRESULT_H_

/**
 * Enumerates possible return values for set settings functions.
 */
enum class SetSettingResult {
    /// The set value is already the current setting value and there is no change in progress.
    NO_CHANGE,
    /// The change request has been enqueued. There will be a follow up notification to inform the operation result.
    ENQUEUED,
    /// The request failed because there is already a value change in process and the setter requested change
    /// to abort if busy.
    BUSY,
    /// The request was failed because the setting requested does not exist.
    UNAVAILABLE_SETTING,
    /// The request failed due to some internal error.
    INTERNAL_ERROR
};

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETSETTINGRESULT_H_
