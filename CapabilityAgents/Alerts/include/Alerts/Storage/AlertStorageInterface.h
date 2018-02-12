/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_STORAGE_ALERTSTORAGEINTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_STORAGE_ALERTSTORAGEINTERFACE_H_

#include "Alerts/Alert.h"

#include <memory>
#include <string>
#include <vector>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {
namespace storage {

/**
 * An Interface class which defines APIs for interacting with a general database, as well as for storing, loading,
 * and modifying Alerts.
 */
class AlertStorageInterface {
public:
    /**
     * Destructor.
     */
    virtual ~AlertStorageInterface() = default;

    /**
     * Creates a new database with the given filepath.
     * If the file specified already exists, or if a database is already being handled by this object, then
     * this function returns false.
     *
     * @param filePath The path to the file which will be used to contain the database.
     * @return @c true If the database is created ok, or @c false if either the file exists or a database is already
     * being handled by this object.
     */
    virtual bool createDatabase(const std::string& filePath) = 0;

    /**
     * Open a database with the given filepath.  If this object is already managing an open database, or the file
     * does not exist, or there is a problem opening the database, this function returns false.
     *
     * @param filePath The path to the file which will be used to contain the database.
     * @return @c true If the database is opened ok, @c false if either the file does not exist, if this object is
     * already managing an open database, or if there is another internal reason the database could not be opened.
     */
    virtual bool open(const std::string& filePath) = 0;

    /**
     * Query if this object is currently managing an open database.
     *
     * @return @c true If a database is being currently managed by this object, @c false otherwise.
     */
    virtual bool isOpen() = 0;

    /**
     * Close the currently open database, if one is open.
     */
    virtual void close() = 0;

    /**
     * Query whether an alert is currently stored with the given token.
     *
     * @param token The AVS token which uniquely identifies an alert.
     * @return @c true If the alert is stored in the database, @c false otherwise.
     */
    virtual bool alertExists(const std::string& token) = 0;

    /**
     * Stores a single @c Alert in the database.
     *
     * @param The @c Alert to store.
     * @return Whether the @c Alert was successfully stored.
     */
    virtual bool store(std::shared_ptr<Alert> alert) = 0;

    /**
     * Loads all alerts in the database.
     *
     * @param[out] alertContainer The container where alerts should be stored.
     * @return Whether the @c Alerts were successfully loaded.
     */
    virtual bool load(std::vector<std::shared_ptr<Alert>>* alertContainer) = 0;

    /**
     * Updates a database record of the @c Alert parameter.
     * The fields which are updated by this operation are the state and scheduled times of the alert.
     * All other fields of an alert do not change over time, and so will not be captured in the database
     * when calling this function.
     *
     * @param alert The @c Alert to be modified.
     * @return Whether the @c Alert was successfully modified.
     */
    virtual bool modify(std::shared_ptr<Alert> alert) = 0;

    /**
     * Erases a single alert from the database.
     *
     * @param alert The @c Alert to be erased.
     * @return Whether the @c Alert was successfully erased.
     */
    virtual bool erase(std::shared_ptr<Alert> alert) = 0;

    /**
     * Erases a collection of alerts from the database.
     * NOTE: The input to this function is expected to be a collection of database-ids for each row in the
     * alerts table.  This is different from the 'token' which AVS uses to identify an alert.  The reason for
     * this decision is that database operations should be more efficient overall when keying off a small integer,
     * rather than a long string.
     *
     * @param alertDbIds A container of alert ids.
     * @return Whether the alerts were successfully erased.
     */
    virtual bool erase(const std::vector<int>& alertDbIds) = 0;

    /**
     * A utility function to clear the database of all records.  Note that the database will still exist, as will
     * the tables.  Only the rows will be erased.
     *
     * @return Whether the database was successfully cleared.
     */
    virtual bool clearDatabase() = 0;

    /**
     * An enum class to help debug database contents.  This type is used in the printStats function below.
     */
    enum class StatLevel {
        /// Print only a single line, providing a count of rows from each table.
        ONE_LINE,
        /// Print all details of the Alerts table, summarizing the other tables.
        ALERTS_SUMMARY,
        /// Print all details of all records.
        EVERYTHING
    };

    /**
     * A utility function to print the contents of the database to the SDK logger output.
     * This function is provided for debug use only.
     */
    virtual void printStats(StatLevel level = StatLevel::ONE_LINE) = 0;
};

}  // namespace storage
}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_STORAGE_ALERTSTORAGEINTERFACE_H_
