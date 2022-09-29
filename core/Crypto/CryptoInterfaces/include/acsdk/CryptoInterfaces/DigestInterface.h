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

#ifndef ACSDK_CRYPTOINTERFACES_DIGESTINTERFACE_H_
#define ACSDK_CRYPTOINTERFACES_DIGESTINTERFACE_H_

#include <string>
#include <vector>

namespace alexaClientSDK {
namespace cryptoInterfaces {

/**
 * @brief Digest computation interface.
 *
 * This interface wraps up logic for computing various digest types (SHA-2, RC5, etc.).
 *
 * To compute the digest, the user shall call any of @a process methods to consume all input data, and when all data
 * is consume, call #finalize to get the result.
 *
 * @code{.cpp}
 * DigestInterface::DataBlock input = ...;
 *
 * auto digest = cryptoFactory->createDigest(DigestType::SHA256);
 * codec->process(input);
 * codec->process(input);
 * DigestInterface::DataBlock digestData;
 * codec->finalize(&digestData);
 * @endcode
 *
 * The instance of the class is reusable, and it also can be used if any of the methods returned
 * an error code after call to #reset().
 *
 * ### Thread Safety
 *
 * This interface is not thread safe and caller must ensure only one thread can make calls at any time.
 *
 * @ingroup Lib_acsdkCryptoInterfaces
 */
class DigestInterface {
public:
    /// @brief Data block type.
    /// This type represents a byte array.
    typedef std::vector<unsigned char> DataBlock;

    /// Default destructor.
    virtual ~DigestInterface() noexcept = default;

    /**
     * @brief Updates digest with a data block.
     *
     * Updates digest value with a data from a data block.
     *
     * This call is logical equivalent to a following code:
     * @code{.cpp}
     * for (b: dataIn) processUInt8(b);
     * @endcode
     *
     * @param[in] dataIn Data for digest.
     *
     * @return True if operation has succeeded. If operation fails, the state of object is undefined, and user must
     * call #reset() to reuse the object.
     */
    virtual bool process(const DataBlock& dataIn) noexcept = 0;

    /**
     * @brief Updates digest with a data block range.
     *
     * Updates digest value with a data from a data block range.
     *
     * This call is logical equivalent to a following code:
     * @code{.cpp}
     * for (auto it = begin; it != end; ++it) processUInt8(*it);
     * @endcode
     *
     * @param[in] begin Begin of data block. This parameter must be equal or less than @a end. If the parameter is
     *                  greater than @a dataInEnd the implementation does nothing and returns false.
     * @param[in] end   Range end. This parameter must be equal or greater than @a begin. If the parameter is smaller
     *                  than @a begin the implementation does nothing and returns false.
     *
     * @return True if operation has succeeded. If operation fails, the state of object is undefined, and user must
     * call #reset() to reuse the object.
     */
    virtual bool process(DataBlock::const_iterator begin, DataBlock::const_iterator end) noexcept = 0;

    /**
     * @brief Updates digest with a byte value.
     *
     * Updates digest value with a single byte value.
     *
     * @param[in] value byte value.
     *
     * @return True if operation has succeeded. If operation fails, the state of object is undefined, and user must
     * call #reset() to reuse the object.
     */
    virtual bool processByte(unsigned char value) noexcept = 0;

    /**
     * @brief Updates digest with uint8_t value.
     *
     * Updates digest value with a 8-bit value.
     *
     * @param[in] value Integer value.
     *
     * @return True if operation has succeeded. If operation fails, the state of object is undefined, and user must
     * call #reset() to reuse the object.
     */
    virtual bool processUInt8(uint8_t value) noexcept = 0;

    /**
     * @brief Updates digest with uint16_t integer value.
     *
     * Updates digest value with uint16_t data. This method uses big endian (network byte order) encoding for
     * presenting input value as a byte array.
     *
     * This method is equivalent to the following:
     * @code
     * processUInt8(value >> 8) && processUInt8(value);
     * @endcode
     *
     * @param[in] value Integer value.
     *
     * @return True if operation has succeeded. If operation fails, the state of object is undefined, and user must
     * call #reset() to reuse the object.
     */
    virtual bool processUInt16(uint16_t value) noexcept = 0;

    /**
     * @brief Updates digest with uint32_t integer value.
     *
     * Updates digest value with uint32_t data. This method uses big endian (network byte order) encoding for
     * presenting input value as a byte array.
     *
     * This method is equivalent to the following:
     * @code
     * processUInt16(value >> 16) && processUInt16(value);
     * @endcode
     *
     * @param[in] value Integer value.
     *
     * @return True if operation has succeeded. If operation fails, the state of object is undefined, and user must
     * call #reset() to reuse the object.
     */
    virtual bool processUInt32(uint32_t value) noexcept = 0;

    /**
     * @brief Updates digest with uint64_t integer value.
     *
     * Updates digest value with uint64_t data. This method uses big endian (network byte order) encoding for
     * presenting input value as a byte array.
     *
     * This method is equivalent to the following:
     * @code
     * processUInt32((uint32_t)(value >> 32)) && processUInt32((uint32_t)value);
     * @endcode
     *
     * @param[in] value Integer value.
     *
     * @return True if operation has succeeded. If operation fails, the state of object is undefined, and user must
     * call #reset() to reuse the object.
     */
    virtual bool processUInt64(uint64_t value) noexcept = 0;

    /**
     * @brief Updates digest with string value.
     *
     * Updates digest with bytes from a string object. The input is treated as a byte array without terminating null
     * character.
     *
     * This method is equivalent to the following:
     * @code
     * for (auto ch: value) processByte(ch);
     * @endcode
     *
     * @param[in] value String value.
     *
     * @return True if operation has succeeded. If operation fails, the state of object is undefined, and user must
     * call #reset() to reuse the object.
     */
    virtual bool processString(const std::string& value) noexcept = 0;

    /**
     * @brief Finishes digest computation and produces the result.
     *
     * This method finishes digest computation and produces the result. The object is reset if this call succeeds and
     * can be reused for computing new digest.
     *
     * @param[out] dataOut  Computed digest. The size of output depends on the selected digest algorithm. The method
     *                      appends data to @a dataOut container.
     *
     * @return True if operation has succeeded. This object state is reset and ready to start computing a new digest.
     * If operation fails, the state of the object and contents of @a dataOut container are undefined. The user can
     * call #reset() to reuse the object in this case.
     */
    virtual bool finalize(DataBlock& dataOut) noexcept = 0;

    /**
     * @brief Resets the digest.
     *
     * This method resets object state and prepares it for reuse.
     *
     * @return True if operation has succeeded. If operation fails, the state of the object is undefined and the object
     * must not used.
     */
    virtual bool reset() noexcept = 0;
};

}  // namespace cryptoInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_CRYPTOINTERFACES_DIGESTINTERFACE_H_
