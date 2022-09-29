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

#include <algorithm>

#include <acsdk/Notifier/private/ObserverWrapper.h>
#include <AVSCommon/Utils/Logger/Logger.h>

/// String to identify log entries originating from this file.
/// @ingroup Lib_acsdkNotifier
/// @private
#define TAG "ObserverWrapper"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 * @ingroup Lib_acsdkNotifier
 * @private
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace notifier {

ObserverWrapper::ObserverWrapper(ReferenceType type, const std::shared_ptr<void>& observer) noexcept : m_type{type} {
    switch (m_type) {
        case ReferenceType::None:
            break;
        case ReferenceType::StrongRef:
            new (&m_sharedPtrObserver) std::shared_ptr<void>{observer};
            break;
        case ReferenceType::WeakRef:
            new (&m_weakPtrObserver) std::weak_ptr<void>{observer};
            break;
        default:
            ACSDK_ERROR(LX(__func__).d("reason", "unexpectedState"));
            break;
    }
}

ObserverWrapper::ObserverWrapper(ObserverWrapper&& src) noexcept : m_type{src.m_type} {
    switch (m_type) {
        case ReferenceType::None:
            break;
        case ReferenceType::StrongRef:
            new (&m_sharedPtrObserver) std::shared_ptr<void>{std::move(src.m_sharedPtrObserver)};
            break;
        case ReferenceType::WeakRef:
#if !defined(__clang__) && defined(__GNUC__) && (__GNUC__ < 6)
            // Move constructor doesn't work for std::weak_ptr in GCC 4.8
            new (&m_weakPtrObserver) std::weak_ptr<void>{src.m_weakPtrObserver};
            src.m_weakPtrObserver.reset();
#else
            new (&m_weakPtrObserver) std::weak_ptr<void>{std::move(src.m_weakPtrObserver)};
#endif
            break;
        default:
            ACSDK_ERROR(LX(__func__).d("reason", "unexpectedState"));
            break;
    }
}

ObserverWrapper::ObserverWrapper(const ObserverWrapper& src) noexcept : m_type{src.m_type} {
    switch (m_type) {
        case ReferenceType::None:
            break;
        case ReferenceType::StrongRef:
            new (&m_sharedPtrObserver) std::shared_ptr<void>{src.m_sharedPtrObserver};
            break;
        case ReferenceType::WeakRef:
            new (&m_weakPtrObserver) std::weak_ptr<void>{src.m_weakPtrObserver};
            break;
        default:
            ACSDK_ERROR(LX(__func__).d("reason", "unexpectedState"));
            break;
    }
}

ObserverWrapper& ObserverWrapper::operator=(ObserverWrapper&& src) noexcept {
    if (m_type != src.m_type) {
        reset();
        m_type = src.m_type;
        switch (m_type) {
            case ReferenceType::StrongRef:
                new (&m_sharedPtrObserver) std::shared_ptr<void>{std::move(src.m_sharedPtrObserver)};
                break;
            case ReferenceType::WeakRef:
#if !defined(__clang__) && defined(__GNUC__) && (__GNUC__ < 6)
                // Move constructor doesn't work for std::weak_ptr in GCC 4.8
                new (&m_weakPtrObserver) std::weak_ptr<void>{src.m_weakPtrObserver};
                src.m_weakPtrObserver.reset();
#else
                new (&m_weakPtrObserver) std::weak_ptr<void>{std::move(src.m_weakPtrObserver)};
#endif
                break;
            default:
                ACSDK_ERROR(LX(__func__).d("reason", "unexpectedState"));
                break;
        }
    } else {
        switch (m_type) {
            case ReferenceType::None:
                break;
            case ReferenceType::StrongRef:
                m_sharedPtrObserver = std::move(src.m_sharedPtrObserver);
                break;
            case ReferenceType::WeakRef:
                m_weakPtrObserver = std::move(src.m_weakPtrObserver);
                break;
            default:
                ACSDK_ERROR(LX(__func__).d("reason", "unexpectedState"));
                break;
        }
    }

    return *this;
}

ObserverWrapper::~ObserverWrapper() noexcept {
    reset();
}

std::shared_ptr<void> ObserverWrapper::get() const {
    switch (m_type) {
        case ReferenceType::None:
            return nullptr;
        case ReferenceType::StrongRef:
            return m_sharedPtrObserver;
        case ReferenceType::WeakRef:
            return m_weakPtrObserver.lock();
        default:
            ACSDK_ERROR(LX(__func__).d("reason", "unexpectedState"));
            return nullptr;
    }
}

void ObserverWrapper::reset() noexcept {
    switch (m_type) {
        case ReferenceType::None:
            break;
        case ReferenceType::StrongRef:
            m_sharedPtrObserver.~shared_ptr<void>();
            m_type = ReferenceType::None;
            break;
        case ReferenceType::WeakRef:
            m_weakPtrObserver.~weak_ptr<void>();
            m_type = ReferenceType::None;
            break;
        default:
            ACSDK_ERROR(LX(__func__).d("reason", "unexpectedState"));
            break;
    }
}

bool ObserverWrapper::isEqualOrExpired(void* observer) const noexcept {
    switch (m_type) {
        case ReferenceType::None:
            return true;
        case ReferenceType::StrongRef:
            return m_sharedPtrObserver.get() == observer;
        case ReferenceType::WeakRef: {
            auto shared = m_weakPtrObserver.lock();
            return !shared || shared.get() == observer;
        }
        default:
            ACSDK_ERROR(LX(__func__).d("reason", "unexpectedState"));
            return true;
    }
}

ReferenceType ObserverWrapper::getReferenceType() const noexcept {
    return m_type;
}

}  // namespace notifier
}  // namespace alexaClientSDK
