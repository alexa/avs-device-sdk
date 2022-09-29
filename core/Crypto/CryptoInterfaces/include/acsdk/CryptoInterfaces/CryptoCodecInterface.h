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

#ifndef ACSDK_CRYPTOINTERFACES_CRYPTOCODECINTERFACE_H_
#define ACSDK_CRYPTOINTERFACES_CRYPTOCODECINTERFACE_H_

#include <vector>

namespace alexaClientSDK {
namespace cryptoInterfaces {

/**
 * @brief Crypto codec (cipher) interface.
 *
 * This interface provides functions to encrypt and decrypt the data, and behaviour depends on the way the interface is
 * created. See @ref CryptoFactoryInterface for details on this interface creation.
 *
 * * * *
 * ### Using Encryption and Decryption without Authentication
 *
 * For encryption without authentication, the algorithm takes key, initialization vector, and plaintext (unencrypted
 * data) as inputs, and produces ciphertext (encrypted data) as output. Application must keep the key secret, while
 * initialization vector can be stored or transferred along ciphertext.
 *
 * For decryption without authentication, the algorithm takes key, initialization vector, and ciphertext (encrypted
 * data) as inputs, and produces plaintext (unencrypted data) as output. When decrypting, key and initialization vector
 * must match the ones, provided during decryption.
 *
 * Codec must be initialized before use with a call to CryptoCodecInterface::init method. The data is encrypted or
 * decrypted with subsequent calls to CryptoCodecInterface::process. Because the codec may cache some of the output
 * inside internal buffers, the user must call CryptoCodecInterface::finalize method to get the output remainder.
 *
 * @code{.cpp}
 * CryptoCodecInterface::DataBlock plaintext = ...;
 * CryptoCodecInterface::Key key = ...;
 * CryptoCodecInterface::IV iv = ...;
 *
 * auto codec = cryptoFactory->createEncoder(AlgorithmType::AES_256_CBC_PAD);
 * cipher->init(key, iv);
 * CryptoCodecInterface::DataBlock ciphertext;
 * codec->encode(plaintext, &ciphertext);
 * codec->finalize(&ciphertext);
 * @endcode
 *
 * The interface allows processing of a continuous stream of data, broken into parts, as long as each part is mappable
 * into one of supported "process" inputs.
 *
 * @code{.cpp}
 * cipher->init(key, iv);
 * CryptoCodecInterface::DataBlock ciphertext;
 * // Encode data blocks
 * codec->process(plaintext1, ciphertext);
 * codec->process(plaintext2, ciphertext);
 * ...
 * codec->process(plaintextN, ciphertext);
 * codec->finalize(&ciphertext);
 * @endcode
 *
 * The instance of this class can be re-initialized for reuse by calling #init() method and supplying new key and IV.
 * This method resets codec state and prepares it for encoding/decoding new data. Key and IV can have the same value
 * or be different:
 * @code{.cpp}
 * // Encode the data with a key and first IV.
 * cipher->init(key, iv1);
 * codec->process(plaintext1, ciphertext1);
 * ...
 * codec->finalize(&ciphertext1);
 * // Reset the codec to re-encode data with the same key but different IV
 * codec->init(key, iv2)
 * codec->process(plaintext1, ciphertext2);
 * ...
 * codec->finalize(&ciphertext2);
 * @endcode
 *
 * * * *
 * ### Using Authenticated Encryption and Authenticated Decryption (AEAD) Algorithms
 *
 * #### Authenticated encryption
 *
 * For authenticated encryption, the algorithm takes key, initialization vector, additional authenticated data (AAD) and
 * plaintext (unencrypted data) as inputs, and produces ciphertext (encrypted data) and tag (also known as Message
 * Authentication Code/MAC) as outputs.
 *
 * Application must keep the key data secret, while initialization vector and tag can be stored or transferred along
 * ciphertext. The best practice is not to store AAD but to compute it depending on domain scenario. For example, when
 * encrypting property values, property key can be used as AAD, which would protect encrypted data from being reused for
 * a property with a different name.
 *
 * Overall, encryption process is similar to un-authenticated encryption, with the following differences:
 * * Caller must provide AAD right after initialization and before processing is done. AAD is optional, but recommended.
 * * Caller must get a tag (MAC) after encryption is finalized.
 *
 * After codec is initialized with #init() method, additional data is provided with #processAAD() calls before starting
 * encryption with #process() method calls. The tag is retrieved with #getTag() call after #finalize().
 *
 * @code{.cpp}
 * // Encrypt the data with a key and first IV.
 * cipher->init(key, iv1);
 * codec->processAAD(additionalAuthenticatedData);
 * ...
 * // Start encrypting data
 * codec->process(plaintext1, ciphertext1);
 * ...
 * codec->finalize(&ciphertext1);
 *
 * // Retrieve tag
 * codec->getTag(tag);
 * @endcode
 *
 * #### Authenticated Decryption
 *
 * For authenticated decryption, the algorithm takes key, initialization vector, additional authenticated data (AAD),
 * tag (also known as Message Authentication Code/MAC), and ciphertext (encrypted data) as inputs, and produces
 * plaintext (unencrypted data) as output.
 *
 * Overall, decryption process is similar to un-authenticated decryption, with the following differences:
 * * Caller must provide AAD right after initialization and before processing is done. AAD must be the same as one,
 *   used during authenticated encryption.
 * * Caller must set a tag (MAC) for verification before decryption is finalized.
 *
 * After codec is initialized with #init() method, additional data is provided with #processAAD() calls before starting
 * decryption with #process() method calls. The tag is set with #setTag() call before #finalize().
 *
 * @code{.cpp}
 * // Decrypt the data with a key and first IV.
 * cipher->init(key, iv1);
 * codec->processAAD(additionalAuthenticatedData);
 * ...
 * // Start decrypting data
 * codec->process(plaintext1, ciphertext1);
 * ...
 * // Set tag for verification
 * code->setTag(tag);
 * // Finalize the operation
 * codec->finalize(&ciphertext1);
 * @endcode
 *
 * ### Thread Safety
 *
 * This interface is not thread safe and caller must ensure only one thread can make calls at any time.
 *
 * @ingroup Lib_acsdkCryptoInterfaces
 */
class CryptoCodecInterface {
public:
    /// @brief Data block type.
    /// This type represents a byte array.
    typedef std::vector<unsigned char> DataBlock;

    /// @brief Key type.
    /// This type contains key bytes.
    typedef std::vector<unsigned char> Key;

    /// @brief Initialization vector type.
    /// Initialization vector contains data to initialize codec state before encrypting or
    /// decrypting data.
    typedef std::vector<unsigned char> IV;

    /// @brief Tag vector type.
    /// Tag is used with AEAD mode of operation like with Galois/Counter mode.
    typedef std::vector<unsigned char> Tag;

    /// @brief Default destructor.
    virtual ~CryptoCodecInterface() noexcept = default;

    /**
     * @brief Initialize the codec.
     *
     * Initializes (or re-initializes) codec with a given key and initialization vector. This method must be called
     * before any processing can be done.
     *
     * This method can be called to reset and re-initialize codec instance for reuse.
     *
     * @param[in] key   Key to use. The method will fail with an error if the size of the key doesn't correspond to
     *                  cipher type.
     * @param[in] iv    Initialization vector. The method will fail with an error if the size of IV doesn't correspond
     *                  to cipher type.
     *
     * @return True if operation has succeeded. If operation fails, false is returned, and the state
     *         of codec is undefined, and the object must be discarded.
     */
    virtual bool init(const Key& key, const IV& iv) noexcept = 0;

    /**
     * @brief Process AAD data block.
     *
     * Processes Additional Authenticated Data block. AAD is used for Authenticated Encryption Authenticated Decryption
     * algorithms like AES-GCM, and cannot be used with non-AEAD algorithms.
     *
     * AEAD algorithms allow submission of arbitrary amount of AAD (including none), and this data affects algorithm
     * output and tag value computation. When data is encrypted with AAD, the same AAD must be used for decryption.
     *
     * AAD doesn't impact the output size of ciphertext when encrypting, nor the size of plaintext when decrypting. For
     * data decryption the total submitted AAD input must match the one used for encryption. There is no difference if
     * AAD is submitted all at once, or split into smaller chunks and submitted through a series of calls.
     *
     * This method can be called any number of times after #init() has been performed and before calling #process(). If
     * there is no more data to process, the user must call #finalize() to get the final data block. The method will
     * fail, if this method is called before #init() or after #process() or #finalize() calls. The method will fail if
     * the codec algorithm is not from AEAD family.
     *
     * @param[in] dataIn    Additional authenticated data. If the data container is empty, the method will do nothing
     *                      and return true.
     *
     * @return True if operation has succeeded. If operation fails, false is returned, and the state
     *         of codec is undefined, and the codec object must be re-initialized or discarded.
     */
    virtual bool processAAD(const DataBlock& dataIn) noexcept = 0;

    /**
     * @brief Process AAD data block range.
     *
     * Processes Additional Authenticated Data block range. AAD is used for Authenticated Encryption Authenticated
     * Decryption algorithms like AES-GCM, and cannot be used with non-AEAD algorithms.
     *
     * AEAD algorithms allow submission of arbitrary amount of AAD (including none), and this data affects algorithm
     * output and tag value computation. When data is encrypted with AAD, the same AAD must be used for decryption.
     *
     * AAD doesn't impact the output size of ciphertext when encrypting, nor the size of plaintext when decrypting. For
     * data decryption the total submitted AAD input must match the one used for encryption. There is no difference if
     * AAD is submitted all at once, or split into smaller chunks and submitted through a series of calls.
     *
     * This method can be called any number of times after #init() has been performed and before calling #process(). If
     * there is no more data to process, the user must call #finalize() to get the final data block. The method will
     * fail, if this method is called before #init() or after #process() or #finalize() calls. The method will fail if
     * the codec algorithm is not from AEAD family.
     *
     * @param[in] dataInBegin   Range start. This parameter must be equal or less than @a dataInEnd. If the parameter is
     *                          greater than @a dataInEnd the implementation does nothing and returns false.
     * @param[in] dataInEnd     Range end. This parameter must be equal or greater than @a dataInBegin. If the parameter
     *                          is smaller than @a dataInBegin the implementation does nothing and returns false.
     *
     * @return True if operation has succeeded. If operation fails, false is returned, and the state
     *         of codec is undefined, and the codec object must be re-initialized or discarded.
     */
    virtual bool processAAD(DataBlock::const_iterator dataInBegin, DataBlock::const_iterator dataInEnd) noexcept = 0;

    /**
     * @brief Encrypt or decrypt a data block.
     *
     * Processes (encrypts or decrypts) a data block. This method consumes a block of input data and optionally produces
     * output data. Because cipher algorithms can cache some data internally, the size of output may not match size of
     * input.
     *
     * This method can be called any number of times after #init has been performed and before calling #finalize. If
     * there is no more data to process, the user must call #finalize() to get the final data block. The method will
     * fail, if this method is called before #init() or after #finalize() calls.
     *
     * When cipher is processing data, the output is appended to @a dataOut container. The caller should not make
     * assumptions how many bytes will be appended, as the implementation may cache data internally.
     *
     * @param[in]   dataIn  Data to encrypt or decrypt. If the data container is empty, the method will do nothing and
     *                      return true.
     * @param[out]  dataOut Processed data. Method appends data to @a dataOut container. The size of output may differ
     *                      from the size of input.
     *
     * @return True if operation has succeeded. If operation fails, false is returned, and the state
     *         of codec and @a dataOut are undefined, and the codec object must be re-initialized or discarded.
     */
    virtual bool process(const DataBlock& dataIn, DataBlock& dataOut) noexcept = 0;

    /**
     * @brief Encrypt or decrypt a data block range.
     *
     * Processes (encrypts or decrypts) a data block range. This method consumes a block of input data and optionally
     * produces output data. Because cipher algorithms can cache some data internally, the size of output may not match
     * size of input.
     *
     * This method can be called any number of times after #init has been performed and before calling #finalize. If
     * there is no more data to process, the user must call #finalize() to get the final data block. The method will
     * fail, if this method is called before #init() or after #finalize() calls.
     *
     * When cipher is processing data, the output is appended to @a dataOut container. The caller should not make
     * assumptions how many bytes will be appended, as the implementation may cache data internally.
     *
     * @param[in]   dataInBegin Range start. This parameter must be equal or less than @a dataInEnd. If the parameter is
     *                          greater than @a dataInEnd the implementation does nothing and returns false.
     * @param[in]   dataInEnd   Range end. This parameter must be equal or greater than @a dataInBegin. If the parameter
     *                          is smaller than @a dataInBegin the implementation does nothing and returns false.
     * @param[out]  dataOut     Processed data. Method appends (not replaces) data to @a dataOut container. The size of
     *                          output may differ from the size of input.
     *
     * @return True if operation has succeeded. If operation fails, false is returned, and the state
     *         of codec and @a dataOut are undefined, and the codec object must be re-initialized or discarded.
     */
    virtual bool process(
        DataBlock::const_iterator dataInBegin,
        DataBlock::const_iterator dataInEnd,
        DataBlock& dataOut) noexcept = 0;

    /**
     * @brief Complete data processing.
     *
     * Completes processing (encryption or decryption) of data. This method writes a final data block to @a dataOut if
     * necessary. Finalize may or may not produce a final data block depending on codec state and encryption mode. For
     * example, when block cipher is used without padding, this method never produces contents (it may still fail if
     * previous input didn't match block boundary), but when PKCS#7 padding is used, this method may produce up to block
     * size bytes of data.
     *
     * When performing Authenticated Encryption, this method completes tag (MAC) computation and #getTag() method shall
     * be called after this method.
     *
     * When performing Authenticated Decryption, #setTag() method shall be called with a tag (MAC) and this method
     * performs tag validation.
     *
     * @param[out] dataOut Processed data. Method appends data to @a dataOut container.
     *
     * @return True if operation has succeeded. If operation fails, false is returned, and the state
     *         of codec and @a dataOut are undefined, and the codec object must be re-initialized or discarded.
     *
     * @see #getTag()
     * @see #setTag()
     */
    virtual bool finalize(DataBlock& dataOut) noexcept = 0;

    /**
     * @brief Provides tag from authenticated encryption.
     *
     * This method returns a tag (known as Message Authentication Code/MAC) after authenticated encryption is completed
     * with #finalize() call. This method must be used with Authenticated Encryption Authenticated Decryption ciphers
     * like AES-GCM, and cannot be used with non-AEAD algorithms. TThe method will fail if the codec algorithm is not
     * from AEAD family.
     *
     * @param[out] tag Tag value. Method appends a value to @a tag container.
     *
     * @return True if operation has succeeded. If operation fails, false is returned, and the state of codec and @a tag
     *         are undefined, and the codec object must be re-initialized or discarded.
     *
     * @see #setTag()
     * @see https://en.wikipedia.org/wiki/Message_authentication_code
     */
    virtual bool getTag(Tag& tag) noexcept = 0;

    /**
     * @brief Sets tag for authenticated decryption.
     *
     * This method provide a tag (known as Message Authentication Code/MAC) to authenticated decryption algorithm after
     * all ciphertext is submitted with #process() calls and before completing it with #finalize() call. This method
     * must be used with Authenticated Encryption Authenticated Decryption ciphers like AES-GCM, and cannot be used with
     * non-AEAD algorithms. The method will fail if the codec algorithm is not from AEAD family.
     *
     * @param[in] tag Tag value.
     *
     * @return True if operation has succeeded. If operation fails, false is returned, the state of codec is undefined,
     *         and the codec object must be re-initialized or discarded.
     *
     * @see #getTag()
     * @see https://en.wikipedia.org/wiki/Message_authentication_code
     */
    virtual bool setTag(const Tag& tag) noexcept = 0;
};

}  // namespace cryptoInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_CRYPTOINTERFACES_CRYPTOCODECINTERFACE_H_
