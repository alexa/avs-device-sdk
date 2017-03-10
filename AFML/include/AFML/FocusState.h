/*
 * FocusState.h
 *
 * Copyright (c) 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AFML_INCLUDE_AFML_FOCUS_STATE_H_
#define ALEXA_CLIENT_SDK_AFML_INCLUDE_AFML_FOCUS_STATE_H_

namespace alexaClientSDK {
namespace afml {

/**
 * An enum class used to specify the levels of focus that a Channel can have.
 */
enum class FocusState {

    /// Represents the highest focus a Channel can have.
    FOREGROUND,

    /// Represents the intermediate level focus a Channel can have.
    BACKGROUND,
    
    /// This focus is used to represent when a Channel is not being used or when an observer should stop.
    NONE
};

} // namespace afml
} // namespace alexaClientSDK

#endif //ALEXA_CLIENT_SDK_AFML_INCLUDE_AFML_FOCUS_STATE_H_
