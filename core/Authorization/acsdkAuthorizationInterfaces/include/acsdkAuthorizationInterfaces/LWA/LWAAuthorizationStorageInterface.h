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

#ifndef ACSDKAUTHORIZATIONINTERFACES_LWA_LWAAUTHORIZATIONSTORAGEINTERFACE_H_
#define ACSDKAUTHORIZATIONINTERFACES_LWA_LWAAUTHORIZATIONSTORAGEINTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace acsdkAuthorizationInterfaces {
namespace lwa {

/**
 * The interface to persistent storage needed by a LWAAuthorizationInterface.
 *
 * IMPORTANT: Your token storage MUST be encrypted.
 * Note that in the default SDK implementation, we do not provide encryption.
 */
class LWAAuthorizationStorageInterface {
public:
    /**
     * Destructor.
     */
    virtual ~LWAAuthorizationStorageInterface() = default;

    /**
     * @deprecated The recommendation is to use use openOrCreate().
     *
     * Creates a new database with the given filepath.
     * If the file specified already exists, or if a database is already being handled by this object, then
     * this function returns false.
     *
     * @return @c true If the database is created ok, or @c false if either the file exists or a database is already
     * being handled by this object.
     */
    virtual bool createDatabase() = 0;

    /**
     * @deprecated The recommendation is to use use openOrCreate().
     *
     * Open a database with the given filepath.  If this object is already managing an open database, or the file
     * does not exist, or there is a problem opening the database, this function returns false.
     *
     * @return @c true If the database is opened ok, @c false if either the file does not exist, if this object is
     * already managing an open database, or if there is another internal reason the database could not be opened.
     */
    virtual bool open() = 0;

    /**
     * Open the database with the given filepath or creates it if it does not exist.
     *
     * @return @c true If the database is opened and usable. @c false if this object is
     * already managing an open database, or if there is another internal reason the database is unusable.
     */
    virtual bool openOrCreate() = 0;

    /**
     * Set the stored refresh token value.
     *
     * @param refreshToken The refresh token to insert into the database.
     * @return Whether the refresh token was successfully stored.
     */
    virtual bool setRefreshToken(const std::string& refreshToken) = 0;

    /**
     * Clear the stored refresh token value.
     *
     * @return Whether the refresh token was successfully cleared.
     */
    virtual bool clearRefreshToken() = 0;

    /**
     * Get the stored refresh token value.
     *
     * @param[out] refreshToken
     * @return Whether or not the operation was successful.
     */
    virtual bool getRefreshToken(std::string* refreshToken) = 0;

    /**
     * Stores the UserId associated with the account.
     *
     * @param userId The userId to store.
     */
    virtual bool setUserId(const std::string& userId) = 0;

    /**
     * Retrieves the UserId associated with the account.
     *
     * @param[out] userId
     * @return Whether or not the operation was successful.
     */
    virtual bool getUserId(std::string* userId) = 0;

    /**
     * A utility function to clear all records from storage.  Note that the storage will still exist, it
     * will just have not content.
     *
     * @return Whether the database was successfully cleared.
     */
    virtual bool clear() = 0;
};

}  // namespace lwa
}  // namespace acsdkAuthorizationInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKAUTHORIZATIONINTERFACES_LWA_LWAAUTHORIZATIONSTORAGEINTERFACE_H_
