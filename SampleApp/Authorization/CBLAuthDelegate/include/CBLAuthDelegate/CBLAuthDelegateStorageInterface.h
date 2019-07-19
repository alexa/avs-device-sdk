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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_AUTHORIZATION_CBLAUTHDELEGATE_INCLUDE_CBLAUTHDELEGATE_CBLAUTHDELEGATESTORAGEINTERFACE_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_AUTHORIZATION_CBLAUTHDELEGATE_INCLUDE_CBLAUTHDELEGATE_CBLAUTHDELEGATESTORAGEINTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace authorization {
namespace cblAuthDelegate {

/**
 * The interface to persistent storage needed by a CBLAuthDelegate (Code Based Linking AuthDelegate).
 */
class CBLAuthDelegateStorageInterface {
public:
    /**
     * Destructor.
     */
    virtual ~CBLAuthDelegateStorageInterface() = default;

    /**
     * Creates a new database with the given filepath.
     * If the file specified already exists, or if a database is already being handled by this object, then
     * this function returns false.
     *
     * @return @c true If the database is created ok, or @c false if either the file exists or a database is already
     * being handled by this object.
     */
    virtual bool createDatabase() = 0;

    /**
     * Open a database with the given filepath.  If this object is already managing an open database, or the file
     * does not exist, or there is a problem opening the database, this function returns false.
     *
     * @return @c true If the database is opened ok, @c false if either the file does not exist, if this object is
     * already managing an open database, or if there is another internal reason the database could not be opened.
     */
    virtual bool open() = 0;

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
     * A utility function to clear all records from storage.  Note that the storage will still exist, it
     * will just have not content.
     *
     * @return Whether the database was successfully cleared.
     */
    virtual bool clear() = 0;
};

}  // namespace cblAuthDelegate
}  // namespace authorization
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_AUTHORIZATION_CBLAUTHDELEGATE_INCLUDE_CBLAUTHDELEGATE_CBLAUTHDELEGATESTORAGEINTERFACE_H_
