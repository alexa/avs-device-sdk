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

#include "SQLiteStorage/SQLiteStatement.h"

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace storage {
namespace sqliteStorage {

using namespace avsCommon::utils::logger;

/// String to identify log entries originating from this file.
static const std::string TAG("SQLiteStatement");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

static const int SQLITE_BIND_PARAMETER_LEFT_MOST_INDEX = 1;
static const int SQLITE_RESULT_FIELD_LEFT_MOST_INDEX = 0;
static const int SQLITE_PARSE_STRING_UNTIL_NUL_CHARACTER = -1;

SQLiteStatement::SQLiteStatement(sqlite3* dbHandle, const std::string& sqlString) : m_stepResult{SQLITE_OK} {
    int rcode = sqlite3_prepare_v2(
        dbHandle,                                 // the db handle
        sqlString.c_str(),                        // the sql string
        SQLITE_PARSE_STRING_UNTIL_NUL_CHARACTER,  // SQLite string parsing instruction
        &m_handle,                                // statement handle
        nullptr);                                 // pointer to unused portion of sql string

    if (rcode != SQLITE_OK) {
        ACSDK_ERROR(LX("SQLiteStatement::constructorFailed")
                        .m("Could not create prepared statement.")
                        .d("rcode", rcode)
                        .d("error message", sqlite3_errmsg(dbHandle)));
        m_handle = nullptr;
    }
}

SQLiteStatement::~SQLiteStatement() {
    finalize();
}

bool SQLiteStatement::isValid() {
    return nullptr != m_handle;
}

bool SQLiteStatement::step() {
    m_stepResult = sqlite3_step(m_handle);

    if (m_stepResult != SQLITE_ROW && m_stepResult != SQLITE_DONE) {
        ACSDK_ERROR(LX("SQLiteStatement::stepFailed")
                        .m("Executing the select did not generate a row or complete.")
                        .d("result", m_stepResult));
        return false;
    }

    return true;
}

bool SQLiteStatement::reset() {
    int rcode = sqlite3_reset(m_handle);
    if (rcode != SQLITE_OK) {
        ACSDK_ERROR(LX("SQLiteStatement::stepFailed").m("Could not reset the prepared statement.").d("rcode", rcode));
        return false;
    }
    return true;
}

bool SQLiteStatement::bindIntParameter(int index, int value) {
    if (index < SQLITE_BIND_PARAMETER_LEFT_MOST_INDEX) {
        ACSDK_ERROR(LX("SQLiteStatement::bindIntParameterFailed").d("invalid position", index));
        return false;
    }

    int rcode = sqlite3_bind_int(
        m_handle,  // the statement handle
        index,     // the position to bind to
        value);    // the value to bind

    if (rcode != SQLITE_OK) {
        ACSDK_ERROR(LX("SQLiteStatement::bindIntParameterFailed")
                        .m("Could not bind int parameter.")
                        .d("bound position", index)
                        .d("value", value)
                        .d("rcode", rcode));
        return false;
    }

    return true;
}

bool SQLiteStatement::bindInt64Parameter(int index, int64_t value) {
    if (index < SQLITE_BIND_PARAMETER_LEFT_MOST_INDEX) {
        ACSDK_ERROR(LX("SQLiteStatement::bindInt64ParameterFailed").d("invalid position", index));
        return false;
    }

    int rcode = sqlite3_bind_int64(
        m_handle,  // the statement handle
        index,     // the position to bind to
        value);    // the value to bind

    if (rcode != SQLITE_OK) {
        ACSDK_ERROR(LX("SQLiteStatement::bindInt64ParameterFailed")
                        .m("Could not bind int64_t parameter.")
                        .d("bound position", index)
                        .d("value", value)
                        .d("rcode", rcode));
        return false;
    }

    return true;
}

bool SQLiteStatement::bindStringParameter(int index, const std::string& value) {
    if (index < SQLITE_BIND_PARAMETER_LEFT_MOST_INDEX) {
        ACSDK_ERROR(LX("SQLiteStatement::bindStringParameterFailed").d("invalid position", index));
        return false;
    }

    int rcode = sqlite3_bind_text(
        m_handle,                                 // the statement handle
        index,                                    // the position to bind to
        value.c_str(),                            // the value to bind
        SQLITE_PARSE_STRING_UNTIL_NUL_CHARACTER,  // SQLite string parsing instruction
        nullptr);                                 // optional destructor for SQLite to call once done

    if (rcode != SQLITE_OK) {
        ACSDK_ERROR(LX("SQLiteStatement::bindStringParameterFailed")
                        .m("Could not bind string parameter.")
                        .d("bound position", index)
                        .d("value", value)
                        .d("rcode", rcode));
        return false;
    }

    return true;
}

int SQLiteStatement::getStepResult() const {
    return m_stepResult;
}

int SQLiteStatement::getColumnCount() const {
    return sqlite3_column_count(m_handle);
}

std::string SQLiteStatement::getColumnName(int index) const {
    if (index < SQLITE_RESULT_FIELD_LEFT_MOST_INDEX) {
        ACSDK_ERROR(LX("SQLiteStatement::getColumnNameFailed").d("invalid position", index));
        return "";
    }

    return sqlite3_column_name(m_handle, index);
}

std::string SQLiteStatement::getColumnText(int index) const {
    if (index < SQLITE_RESULT_FIELD_LEFT_MOST_INDEX) {
        ACSDK_ERROR(LX("SQLiteStatement::getColumnTextFailed").d("invalid position", index));
        return "";
    }

    // The reinterpret_cast is needed due to SQLite storing text data as utf8 (ie. const unsigned char*).
    // We are ok in our implementation to do the cast, since we know we're storing as regular ascii.
    const char* rowValue = reinterpret_cast<const char*>(sqlite3_column_text(m_handle, index));
    return std::string(rowValue);
}

int SQLiteStatement::getColumnInt(int index) const {
    if (index < SQLITE_RESULT_FIELD_LEFT_MOST_INDEX) {
        ACSDK_ERROR(LX("SQLiteStatement::getColumnIntFailed").d("invalid position", index));
        return 0;
    }

    return sqlite3_column_int(m_handle, index);
}

int64_t SQLiteStatement::getColumnInt64(int index) const {
    if (index < SQLITE_RESULT_FIELD_LEFT_MOST_INDEX) {
        ACSDK_ERROR(LX("SQLiteStatement::getColumnInt64Failed").d("invalid position", index));
        return 0;
    }

    return static_cast<int64_t>(sqlite3_column_int64(m_handle, index));
}

void SQLiteStatement::finalize() {
    if (m_handle) {
        int rcode = sqlite3_finalize(m_handle);
        m_handle = nullptr;

        if (rcode != SQLITE_OK) {
            ACSDK_ERROR(
                LX("SQLiteStatement::finalizeFailed").m("Could release the prepared statement.").d("rcode", rcode));
        }
    }
}

}  // namespace sqliteStorage
}  // namespace storage
}  // namespace alexaClientSDK
