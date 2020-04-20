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

#ifndef ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_SYSTEMCLOCKDELAY_H_
#define ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_SYSTEMCLOCKDELAY_H_

#include "DelayInterface.h"

namespace alexaClientSDK {
namespace captions {

/**
 * Concrete implementation of the @c DelayInterface, using @c std::this_thread::sleep_for().
 */
class SystemClockDelay : public DelayInterface {
public:
    /// @name DelayInterface methods
    /// @{
    void delay(std::chrono::milliseconds milliseconds) override;
    ///@}
};

}  // namespace captions
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_SYSTEMCLOCKDELAY_H_
