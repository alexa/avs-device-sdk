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

#ifndef ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_DELAYINTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_DELAYINTERFACE_H_

#include <chrono>

namespace alexaClientSDK {
namespace captions {

/**
 * A wrapper class to abstract away the delay call.
 */
class DelayInterface {
public:
    /**
     * Destructor.
     */
    virtual ~DelayInterface() = default;

    /**
     * When called, this function should cause a delay for the given number of milliseconds. This function can be
     * overridden to allow for slower or faster delays if desired. The @c milliseconds parameter should be checked that
     * it is positive; no delay should occur if it is negative.
     *
     * @param milliseconds The amount of time to delay.
     */
    virtual void delay(std::chrono::milliseconds milliseconds) = 0;
};

}  // namespace captions
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_DELAYINTERFACE_H_
