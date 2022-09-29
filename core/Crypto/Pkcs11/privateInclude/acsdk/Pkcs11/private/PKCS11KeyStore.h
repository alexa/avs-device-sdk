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

#ifndef ACSDK_PKCS11_PRIVATE_PKCS11KEYSTORE_H_
#define ACSDK_PKCS11_PRIVATE_PKCS11KEYSTORE_H_

#include <memory>
#include <unordered_map>

#include <acsdk/CryptoInterfaces/KeyStoreInterface.h>
#include <acsdk/Pkcs11/private/PKCS11Functions.h>
#include <acsdk/Pkcs11/private/PKCS11Key.h>
#include <acsdk/Pkcs11/private/PKCS11KeyDescriptor.h>
#include <acsdk/Pkcs11/private/PKCS11Session.h>
#include <acsdk/Pkcs11/private/PKCS11Slot.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>

namespace alexaClientSDK {
namespace pkcs11 {

using alexaClientSDK::avsCommon::utils::metrics::MetricRecorderInterface;
using alexaClientSDK::cryptoInterfaces::AlgorithmType;
using alexaClientSDK::cryptoInterfaces::KeyStoreInterface;

/**
 * @brief Key store implementation for PKCS11.
 *
 * This class implements features of @ref alexaClientSDK::cryptoInterfaces::KeyStoreInterface using PKCS#11
 * interface functions.
 *
 * @ingroup Lib_acsdkPkcs11
 */
class PKCS11KeyStore : public KeyStoreInterface {
public:
    /**
     * @brief Creates key store.
     *
     * @param[in] metricRecorder Optional reference for metrics reporting.
     *
     * @return Object reference or nullptr on error.
     */
    static std::shared_ptr<KeyStoreInterface> create(
        const std::shared_ptr<MetricRecorderInterface>& metricRecorder = nullptr) noexcept;

    /// @name KeyStoreInterface methods.
    ///@{
    ~PKCS11KeyStore() noexcept override;
    bool encrypt(
        const std::string& keyAlias,
        AlgorithmType type,
        const IV& iv,
        const DataBlock& plaintext,
        KeyChecksum& checksum,
        DataBlock& ciphertext) noexcept override;
    bool encryptAE(
        const std::string& keyAlias,
        AlgorithmType type,
        const IV& iv,
        const DataBlock& aad,
        const DataBlock& plaintext,
        KeyChecksum& checksum,
        DataBlock& ciphertext,
        Tag& tag) noexcept override;
    bool decrypt(
        const std::string& keyAlias,
        AlgorithmType type,
        const KeyChecksum& checksum,
        const IV& iv,
        const DataBlock& ciphertext,
        DataBlock& plaintext) noexcept override;
    bool decryptAD(
        const std::string& keyAlias,
        AlgorithmType type,
        const KeyChecksum& checksum,
        const IV& iv,
        const DataBlock& aad,
        const DataBlock& ciphertext,
        const Tag& tag,
        DataBlock& plaintext) noexcept override;
    bool getDefaultKeyAlias(std::string& keyAlias) noexcept override;
    ///@}

private:
    /// @brief Private constructor.
    PKCS11KeyStore(const std::shared_ptr<MetricRecorderInterface>& metricRecorder) noexcept;

    /// @brief Helper to initialize key store.
    bool init() noexcept;

    /**
     * @brief Locates key by given alias and type.
     *
     * @param[in] objectLabel   Key alias.
     * @param[in] type          Key type.
     * @return Key wrapper reference or nullptr on error.
     */
    std::shared_ptr<PKCS11Key> loadKey(const std::string& objectLabel, AlgorithmType type) noexcept;

    /**
     * @brief Locates key by given criteria.
     *
     * HSM may contain several objects of the same type and label. Search criteria in @c PKCS11KeyDescriptor narrows
     * down search results, so there are better chances to find what we actually look for.
     *
     * @param[in] descriptor    Key descriptor with search criteria.
     *
     * @return Key wrapper reference or nullptr on error.
     *
     * @see PKCS11KeyDescriptor
     */
    std::shared_ptr<PKCS11Key> loadKeyLocked(PKCS11KeyDescriptor&& descriptor) noexcept;

    /**
     * @brief Submit metrics.
     *
     * This method reports a counter increment for a given event name. Failure metrics increments two counters: one for
     * given event, and one with a predefined name ("FAILURE").
     *
     * @param[in] activity  The activity name.
     * @param[in] eventName The event name string.
     * @param[in] count     The metric value.
     * @param[in] failure   Flag, if FAILURE metric shall also be incremented.
     */
    void submitMetric(const std::string& activity, const std::string& eventName, uint64_t count, bool failure) noexcept;

    /// Optional reference to metric recorder.
    std::shared_ptr<MetricRecorderInterface> m_metricRecorder;

    /// PKCS11 functions reference
    std::shared_ptr<PKCS11Functions> m_functions;

    /// PKCS11 session reference
    std::shared_ptr<PKCS11Session> m_session;

    /// Mutex to serialize key collection access.
    std::mutex m_keysMutex;

    /// Map of key alias to PKCS11 key handles.
    std::unordered_map<PKCS11KeyDescriptor, std::shared_ptr<PKCS11Key>> m_keys;

    /// Default key alias.
    std::string m_defaultKeyAlias;
};

}  // namespace pkcs11
}  // namespace alexaClientSDK

#endif  // ACSDK_PKCS11_PRIVATE_PKCS11KEYSTORE_H_
