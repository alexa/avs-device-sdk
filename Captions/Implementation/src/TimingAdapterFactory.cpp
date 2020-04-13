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

#include <memory>

#include <Captions/TimingAdapterFactory.h>
#include <Captions/CaptionTimingAdapter.h>

namespace alexaClientSDK {
namespace captions {

TimingAdapterFactory::TimingAdapterFactory(std::shared_ptr<DelayInterface> delayInterface) :
        m_delayInterface{delayInterface} {
    if (!m_delayInterface) {
        m_delayInterface = std::make_shared<SystemClockDelay>();
    }
}

std::shared_ptr<CaptionTimingAdapterInterface> TimingAdapterFactory::getTimingAdapter(
    std::shared_ptr<CaptionPresenterInterface> presenter) const {
    return std::shared_ptr<CaptionTimingAdapterInterface>(new CaptionTimingAdapter(presenter, m_delayInterface));
}

}  // namespace captions
}  // namespace alexaClientSDK