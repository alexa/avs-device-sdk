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

#include <AVSCommon/Utils/Logger/Logger.h>

#include "BlueZ/GVariantMapReader.h"

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

/// String to identify log entries originating from this file.
static const std::string TAG{"GVariantMapReader"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

bool GVariantMapReader::forEach(std::function<bool(char* key, GVariant* value)> iteratorFunction) const {
    if (nullptr == m_map) {
        ACSDK_ERROR(LX("forEachFailed").d("reason", "m_map is null"));
        return false;
    }

    GVariantIter iter;
    g_variant_iter_init(&iter, m_map);
    GVariant* value = nullptr;
    char* key = nullptr;
    bool stopped = false;

    while (g_variant_iter_next(&iter, m_useObjectPathKeys ? "{&o*}" : "{&s*}", &key, &value)) {
        bool shouldContinue = iteratorFunction(key, value);
        g_variant_unref(value);
        if (!shouldContinue) {
            stopped = true;
            break;
        }
    }

    return stopped;
}

GVariantMapReader::GVariantMapReader(GVariant* originalVariant, bool useObjectPathAsKey) :
        m_map{originalVariant},
        m_useObjectPathKeys{useObjectPathAsKey} {
    if (originalVariant) {
        g_variant_ref(originalVariant);
    }
}

GVariantMapReader::GVariantMapReader(ManagedGVariant& originalVariant, bool useObjectPathAsKey) :
        m_map{originalVariant.get()},
        m_useObjectPathKeys{useObjectPathAsKey} {
    if (originalVariant.hasValue()) {
        g_variant_ref(originalVariant.get());
    }
}

GVariantMapReader::~GVariantMapReader() {
    if (m_map != nullptr) {
        g_variant_unref(m_map);
    }
}

bool GVariantMapReader::getCString(const char* name, char** value) const {
    if (nullptr == m_map) {
        ACSDK_ERROR(LX("getCStringFailed").d("reason", "m_map is null"));
        return false;
    }
    if (nullptr == name) {
        ACSDK_ERROR(LX("getCStringFailed").d("reason", "name is null"));
        return false;
    }
    if (nullptr == value) {
        ACSDK_ERROR(LX("getCStringFailed").d("reason", "value is null"));
        return false;
    }
    return g_variant_lookup(m_map, name, "&s", value) != 0;
}

bool GVariantMapReader::getInt32(const char* name, gint32* value) const {
    if (nullptr == m_map) {
        ACSDK_ERROR(LX("getInt32Failed").d("reason", "m_map is null"));
        return false;
    }
    if (nullptr == name) {
        ACSDK_ERROR(LX("getInt32Failed").d("reason", "name is null"));
        return false;
    }
    if (nullptr == value) {
        ACSDK_ERROR(LX("getInt32Failed").d("reason", "value is null"));
        return false;
    }
    return g_variant_lookup(m_map, name, "i", value) != 0;
}

bool GVariantMapReader::getBoolean(const char* name, gboolean* value) const {
    if (nullptr == m_map) {
        ACSDK_ERROR(LX("getBooleanFailed").d("reason", "m_map is null"));
        return false;
    }
    if (nullptr == name) {
        ACSDK_ERROR(LX("getBooleanFailed").d("reason", "name is null"));
        return false;
    }
    if (nullptr == value) {
        ACSDK_ERROR(LX("getBooleanFailed").d("reason", "value is null"));
        return false;
    }
    return g_variant_lookup(m_map, name, "b", value) != 0;
}

ManagedGVariant GVariantMapReader::getVariant(const char* name) const {
    if (nullptr == m_map) {
        ACSDK_ERROR(LX("getVariantFailed").d("reason", "m_map is null"));
        return ManagedGVariant();
    }
    if (nullptr == name) {
        ACSDK_ERROR(LX("getVariantFailed").d("reason", "name is null"));
        return ManagedGVariant();
    }
    GVariant* value = nullptr;
    g_variant_lookup(m_map, name, "*", &value);
    return ManagedGVariant(value);
}

GVariant* GVariantMapReader::get() const {
    return m_map;
}

GVariantMapReader::GVariantMapReader(const GVariantMapReader& other) {
    m_map = other.m_map;
    if (nullptr != m_map) {
        g_variant_ref(m_map);
    }
    m_useObjectPathKeys = other.m_useObjectPathKeys;
}

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK
