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

#include "TestTimingAdapterFactory.h"

namespace alexaClientSDK {
namespace captions {
namespace test {

TestTimingAdapterFactory::~TestTimingAdapterFactory() {
    m_timingAdapter.reset();
}

std::shared_ptr<CaptionTimingAdapterInterface> TestTimingAdapterFactory::getTimingAdapter(
    std::shared_ptr<CaptionPresenterInterface> presenter) const {
    return m_timingAdapter;
}

std::shared_ptr<MockCaptionTimingAdapter> TestTimingAdapterFactory::getMockTimingAdapter() {
    if (!m_timingAdapter) {
        m_timingAdapter = std::make_shared<NiceMock<MockCaptionTimingAdapter>>();
    }
    return m_timingAdapter;
}

}  // namespace test
}  // namespace captions
}  // namespace alexaClientSDK