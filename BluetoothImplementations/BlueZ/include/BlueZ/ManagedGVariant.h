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

#ifndef ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_MANAGEDGVARIANT_H_
#define ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_MANAGEDGVARIANT_H_

#include <memory>
#include <string>

#include <gio/gio.h>

namespace alexaClientSDK {
namespace bluetoothImplementations {
namespace blueZ {

/**
 * A wrapper std::shared class for GLib's GVariant objects. This class is not thread safe. This class does not increase
 * the reference count of the variant, but will descrease it in destructor. Floating references are converted to
 * normal ones effectively extending their lifetime to the lifetime of the @c ManagedGVariant instance.
 */
class ManagedGVariant {
public:
    /**
     * Destructor
     */
    ~ManagedGVariant();

    /**
     * Default constructor
     */
    ManagedGVariant();

    /**
     * A constructor attaching to an existing @c GVariant*. If @c variant is a floating reference, it will be converted
     * to normal one, extending its lifetime to the lifetime of the @c ManagedGVariant instance.
     *
     * @param variant Variant to be attached.
     */
    explicit ManagedGVariant(GVariant* variant);

    /**
     * Get a pointer to the internal @c GVariant* variable
     *
     * @return A pointer to the internal variable holding the GVariant*. The pointer is valid while the
     * @c ManagedGVariant instance is valid.
     */
    GVariant** asOutputParameter();

    /**
     * Returns a raw @c GVariant* attached to this object
     *
     * @return A raw @c GVariant* attached to this object. This pointer is valid as long as this @c ManagedGVariant
     * instance if valid.
     */
    GVariant* get();

    /**
     * Method that dumps the contents of the wrapped variant to a string. Returns "<NULL>" if no variant attached.
     *
     * @param withAnnotations true if type information should be added to the output
     * @return A string with the textual representation of the variant
     */
    std::string dumpToString(bool withAnnotations);

    /**
     * A helper method to extract variant contained in the variant attached to the object. This method is useful when
     * Parsing variant containers where child elements could be boxed into the wrapper variant.
     *
     * @return A variant contained in the variant attached.
     */
    ManagedGVariant unbox();

    /**
     * A helper conversion operator to allow checking if object has any variant attached
     *
     * @return true if variant is not null
     */
    bool hasValue();

    /**
     * Swap the @c GVariant* values with another instance of @c ManagedGVariant.
     * @param other
     */
    void swap(ManagedGVariant& other);

private:
    /**
     * The variant attached
     */
    GVariant* m_variant;
};

inline ManagedGVariant::~ManagedGVariant() {
    if (m_variant) {
        g_variant_unref(m_variant);
    }
}

inline ManagedGVariant::ManagedGVariant() : m_variant{nullptr} {
}

inline ManagedGVariant::ManagedGVariant(GVariant* variant) : m_variant{variant} {
    if (variant && g_variant_is_floating(variant)) {
        g_variant_ref_sink(variant);
    }
}

inline GVariant** ManagedGVariant::asOutputParameter() {
    return &m_variant;
}

inline GVariant* ManagedGVariant::get() {
    return m_variant;
}

inline std::string ManagedGVariant::dumpToString(bool withAnnotations) {
    if (!m_variant) {
        return "<NULL>";
    }
    auto cstring = g_variant_print(m_variant, withAnnotations);
    std::string result = cstring;
    g_free(cstring);
    return result;
}

inline ManagedGVariant ManagedGVariant::unbox() {
    if (!m_variant) {
        return ManagedGVariant();
    }
    return ManagedGVariant(g_variant_get_variant(m_variant));
}

inline bool ManagedGVariant::hasValue() {
    return m_variant != nullptr;
}

inline void ManagedGVariant::swap(ManagedGVariant& other) {
    GVariant* temp = other.m_variant;
    other.m_variant = m_variant;
    m_variant = temp;
}

}  // namespace blueZ
}  // namespace bluetoothImplementations
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_BLUETOOTHIMPLEMENTATIONS_BLUEZ_INCLUDE_BLUEZ_MANAGEDGVARIANT_H_
