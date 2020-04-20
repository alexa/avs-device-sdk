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

#ifndef ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_TIMINGADAPTERFACTORY_H_
#define ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_TIMINGADAPTERFACTORY_H_

#include <Captions/CaptionPresenterInterface.h>

#include "CaptionTimingAdapterInterface.h"

namespace alexaClientSDK {
namespace captions {

/**
 * Generator class to produce instances of @c CaptionTimingAdapter on demand.
 */
class TimingAdapterFactory {
public:
    /**
     * Constructor.
     *
     * @param delayInterface The timing interface that is used to delay calls to the presenter. Defaults to a call to
     * the std::this_thread::sleep_for function. Override this value if custom timing is desired.
     */
    TimingAdapterFactory(std::shared_ptr<DelayInterface> delayInterface = nullptr);

    /**
     * Destructor.
     */
    virtual ~TimingAdapterFactory() = default;

    /**
     * Factory function that returns ready-to-use timing adapters.
     *
     * @param presenter The presenter instance to be used when producing timing adapters.
     * @return an instance of @c CaptionTimingAdapter as a @c std::shared_ptr.
     */
    virtual std::shared_ptr<CaptionTimingAdapterInterface> getTimingAdapter(
        std::shared_ptr<CaptionPresenterInterface> presenter) const;

private:
    /// The timing interface that is used to delay calls to the presenter.
    std::shared_ptr<DelayInterface> m_delayInterface;
};

}  // namespace captions
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_CAPTIONS_IMPLEMENTATION_INCLUDE_CAPTIONS_TIMINGADAPTERFACTORY_H_
