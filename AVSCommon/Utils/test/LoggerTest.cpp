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

#include <thread>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "AVSCommon/Utils/Logger/Logger.h"
#include "AVSCommon/Utils/Logger/LoggerSinkManager.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {
namespace test {

using namespace ::testing;

/// Macro used to initial creation of log entries for this source.
#define LX(event) LogEntry(TEST_SOURCE_STRING, event)

/// String used to verify passing metadata keys.
#define METADATA_KEY "metadata_key"

/// Key for boolean @c true metadata value.
#define METADATA_KEY_TRUE "key_true"

/// Key for boolean @c false metadata value.
#define METADATA_KEY_FALSE "key_false"

/// Constant string used to separate (and connect) keys and values.
#define KEY_VALUE_SEPARATOR "="

/// Constant string used to separate key,value pairs from each other.
#define KEY_VALUE_PAIR_SEPARATOR ","

/// Escaped version of UNESCAPED_METADATA_VALUE, used to verify proper escaping of metadata values.
#define ESCAPED_METADATA_VALUE R"(reserved_chars['\\' '\,' '\:' '\='])"

/// Expected output string for test of boolean metadata.
// clang-format off
#define METADATA_EXPECTED_BOOLEANS                      \
        METADATA_KEY_TRUE KEY_VALUE_SEPARATOR "true"    \
        KEY_VALUE_PAIR_SEPARATOR                        \
        METADATA_KEY_FALSE KEY_VALUE_SEPARATOR "false"
// clang-format on

/// String used to test that the source component is logged
static const std::string TEST_SOURCE_STRING = "<The_Source_Of_Log_Entries>";

/// String used to test that the event component is logged
static const std::string TEST_EVENT_STRING = "[Some_Event_Worth_Logging]";

/// Metadata value with characters that must be escaped, to exercise the code that escapes metadata values.
static const std::string UNESCAPED_METADATA_VALUE = R"(reserved_chars['\' ',' ':' '='])";

/// String used to test that the message component is logged
static const std::string TEST_MESSAGE_STRING = "Hello World!";
/// Another String used to test that the message component is logged
static const std::string TEST_MESSAGE_STRING_1 = "World Hello!";
/// A const char* nullptr to verify the logger won't crash in degenerate cases
static const char* TEST_MESSAGE_CONST_NULL_CHAR_PTR = nullptr;
/// A char* nullptr to verify the logger won't crash in degenerate cases
static char* TEST_MESSAGE_NULL_CHAR_PTR = nullptr;

/**
 * Mock derivation of Logger for verifying calls and parameters to those calls.
 */
class MockLogger : public Logger {
public:
    MOCK_METHOD1(shouldLog, bool(Level level));
    MOCK_METHOD4(
        emit,
        void(Level level, std::chrono::system_clock::time_point time, const char* threadMoniker, const char* text));

    /**
     * Create a new MockLogger instance.
     * @return A new MockLogger instance.
     */
    static std::shared_ptr<NiceMock<MockLogger>> create();

    /**
     * MockLogger constructor.
     */
    MockLogger();

    /**
     * Helper method or capturing parameters so that they can be compared to expected values.
     *
     * @param level The severity Level of this log line.
     * @param time The time that the event to log occurred.
     * @param threadMoniker Moniker of the thread that generated the event.
     * @param text The text of the log entry.
     */
    void mockEmit(Level level, std::chrono::system_clock::time_point time, const char* threadId, const char* text);

    /// The last time value passed in a log(...) call.
    std::chrono::system_clock::time_point m_lastTime;

    /// The last thread moniker passed in a log(...) call.
    std::string m_lastThreadMoniker;

    /// The last text value passed in a log(...) call.
    std::string m_lastText;
};

/**
 * Mock derivation of ModuleLogger for verifying calls and parameters to those calls.
 */
class MockModuleLogger : public ModuleLogger {
public:
    /**
     * MockModuleLogger constructor.
     */
    MockModuleLogger();

    /**
     * MockModuleLogger destructor.
     */
    ~MockModuleLogger();

    Level getLogLevel() {
        return Logger::m_level;
    }
};

/// Global to hold on to the current @c MockLogger to use.
static std::shared_ptr<MockLogger> g_log;

/**
 * The log sink that will receive logs from the @c ModuleLogger.  Just forwards logs to the current MockLogger
 * (allowing he MockLogger to be destroyed ay the end of tests, and re-created by the next test).
 */
class TestLogger : public Logger {
public:
    /**
     * Constructor
     */
    TestLogger();

    void emit(Level level, std::chrono::system_clock::time_point time, const char* threadMoniker, const char* text)
        override;
};

TestLogger::TestLogger() : Logger(Level::DEBUG9) {
}

void TestLogger::emit(
    Level level,
    std::chrono::system_clock::time_point time,
    const char* threadMoniker,
    const char* text) {
    g_log->emit(level, time, threadMoniker, text);
}

// Temporarily back out of namespace "test" to put getLoggerTestLogger() in the required namespace.
}  // namespace test

/**
 * Function for ACSDK_* macros to use to get the @c Logger to use.
 * @return The @c Logger to use.
 */
std::shared_ptr<Logger> getLoggerTestLogger() {
    static std::shared_ptr<Logger> testLogger = std::make_shared<test::TestLogger>();
    return testLogger;
}

// Return to namespace "test"
namespace test {

std::shared_ptr<NiceMock<MockLogger>> MockLogger::create() {
    auto result = std::make_shared<NiceMock<MockLogger>>();
    ON_CALL(*result.get(), shouldLog(_)).WillByDefault(Return(true));
    ON_CALL(*result.get(), emit(_, _, _, _)).WillByDefault(Invoke(result.get(), &MockLogger::mockEmit));
    return result;
}

MockLogger::MockLogger() : Logger(Level::DEBUG9) {
}

void MockLogger::mockEmit(
    Level level,
    std::chrono::system_clock::time_point time,
    const char* threadMoniker,
    const char* text) {
    m_lastTime = time;
    m_lastThreadMoniker = threadMoniker;
    m_lastText = text;
}

MockModuleLogger::MockModuleLogger() : ModuleLogger(ACSDK_STRINGIFY(ACSDK_LOG_SINK)) {
}

MockModuleLogger::~MockModuleLogger() {
    LoggerSinkManager::instance().removeSinkObserver(this);
    m_sink->removeLogLevelObserver(this);
}

/**
 * Class for testing the Logger class
 */
class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;

    /**
     * Make EXPECT_CALL invocations for the given log level. The expectations are based upon the ACSDK_*
     * calls that will be made by exerciseLogLevels().
     * @param level The log level that is expected to be set for this test.
     */
    void setLevelExpectations(Level level);

    /**
     * Exercise each of the the ACSDK_* macros for the various log levels.
     */
    void exerciseLevels();
};

void LoggerTest::SetUp() {
    // make sure getLoggerTestLogger() is used as sink
    g_log = MockLogger::create();
    LoggerSinkManager::instance().initialize(getLoggerTestLogger());
}

void LoggerTest::TearDown() {
    g_log.reset();
}

void LoggerTest::setLevelExpectations(Level level) {
    ACSDK_GET_LOGGER_FUNCTION().setLevel(level);

    switch (level) {
        case Level::UNKNOWN:
            break;
        case Level::NONE:
        case Level::CRITICAL:
            EXPECT_CALL(*(g_log.get()), emit(Level::ERROR, _, _, _)).Times(0);
        case Level::ERROR:
            EXPECT_CALL(*(g_log.get()), emit(Level::WARN, _, _, _)).Times(0);
        case Level::WARN:
            EXPECT_CALL(*(g_log.get()), emit(Level::INFO, _, _, _)).Times(0);
        case Level::INFO:
            EXPECT_CALL(*(g_log.get()), emit(Level::DEBUG0, _, _, _)).Times(0);
        case Level::DEBUG0:
            EXPECT_CALL(*(g_log.get()), emit(Level::DEBUG1, _, _, _)).Times(0);
        case Level::DEBUG1:
            EXPECT_CALL(*(g_log.get()), emit(Level::DEBUG2, _, _, _)).Times(0);
        case Level::DEBUG2:
            EXPECT_CALL(*(g_log.get()), emit(Level::DEBUG3, _, _, _)).Times(0);
        case Level::DEBUG3:
            EXPECT_CALL(*(g_log.get()), emit(Level::DEBUG4, _, _, _)).Times(0);
        case Level::DEBUG4:
            EXPECT_CALL(*(g_log.get()), emit(Level::DEBUG5, _, _, _)).Times(0);
        case Level::DEBUG5:
            EXPECT_CALL(*(g_log.get()), emit(Level::DEBUG6, _, _, _)).Times(0);
        case Level::DEBUG6:
            EXPECT_CALL(*(g_log.get()), emit(Level::DEBUG7, _, _, _)).Times(0);
        case Level::DEBUG7:
            EXPECT_CALL(*(g_log.get()), emit(Level::DEBUG8, _, _, _)).Times(0);
        case Level::DEBUG8:
            EXPECT_CALL(*(g_log.get()), emit(Level::DEBUG9, _, _, _)).Times(0);
        case Level::DEBUG9:
            break;
    }

#ifdef ACSDK_DEBUG_LOG_ENABLED
#define DEBUG_COUNT 1
#else
#define DEBUG_COUNT 0
#endif

    switch (level) {
        case Level::DEBUG9:
            EXPECT_CALL(*(g_log.get()), emit(Level::DEBUG9, _, _, _)).Times(DEBUG_COUNT);
        case Level::DEBUG8:
            EXPECT_CALL(*(g_log.get()), emit(Level::DEBUG8, _, _, _)).Times(DEBUG_COUNT);
        case Level::DEBUG7:
            EXPECT_CALL(*(g_log.get()), emit(Level::DEBUG7, _, _, _)).Times(DEBUG_COUNT);
        case Level::DEBUG6:
            EXPECT_CALL(*(g_log.get()), emit(Level::DEBUG6, _, _, _)).Times(DEBUG_COUNT);
        case Level::DEBUG5:
            EXPECT_CALL(*(g_log.get()), emit(Level::DEBUG5, _, _, _)).Times(DEBUG_COUNT);
        case Level::DEBUG4:
            EXPECT_CALL(*(g_log.get()), emit(Level::DEBUG4, _, _, _)).Times(DEBUG_COUNT);
        case Level::DEBUG3:
            EXPECT_CALL(*(g_log.get()), emit(Level::DEBUG3, _, _, _)).Times(DEBUG_COUNT);
        case Level::DEBUG2:
            EXPECT_CALL(*(g_log.get()), emit(Level::DEBUG2, _, _, _)).Times(DEBUG_COUNT);
        case Level::DEBUG1:
            EXPECT_CALL(*(g_log.get()), emit(Level::DEBUG1, _, _, _)).Times(DEBUG_COUNT);
        case Level::DEBUG0:
            EXPECT_CALL(*(g_log.get()), emit(Level::DEBUG0, _, _, _)).Times(DEBUG_COUNT);
        case Level::INFO:
            EXPECT_CALL(*(g_log.get()), emit(Level::INFO, _, _, _)).Times(1);
        case Level::WARN:
            EXPECT_CALL(*(g_log.get()), emit(Level::WARN, _, _, _)).Times(1);
        case Level::ERROR:
            EXPECT_CALL(*(g_log.get()), emit(Level::ERROR, _, _, _)).Times(1);
        case Level::CRITICAL:
            EXPECT_CALL(*(g_log.get()), emit(Level::CRITICAL, _, _, _)).Times(1);
        case Level::NONE:
        case Level::UNKNOWN:
            break;
    }
}

void LoggerTest::exerciseLevels() {
    ACSDK_DEBUG9(LX("DEBUG9"));
    ACSDK_DEBUG8(LX("DEBUG8"));
    ACSDK_DEBUG7(LX("DEBUG7"));
    ACSDK_DEBUG6(LX("DEBUG6"));
    ACSDK_DEBUG5(LX("DEBUG5"));
    ACSDK_DEBUG4(LX("DEBUG4"));
    ACSDK_DEBUG3(LX("DEBUG3"));
    ACSDK_DEBUG2(LX("DEBUG2"));
    ACSDK_DEBUG1(LX("DEBUG1"));
    ACSDK_DEBUG0(LX("DEBUG0"));
    ACSDK_INFO(LX("INFO"));
    ACSDK_WARN(LX("WARN"));
    ACSDK_ERROR(LX("ERROR"));
    ACSDK_CRITICAL(LX("CRITICAL"));
}

/**
 * Test delivery of log messages when the log level is set to DEBUG9.  This test sets the log level to DEBUG9
 * and then verifies that all logs (except those compiled off) are passed through to the emit() method.
 */
TEST_F(LoggerTest, logDebug9Level) {
    setLevelExpectations(Level::DEBUG9);
    exerciseLevels();
}

/**
 * Test delivery of log messages when the log level is set to DEBUG8.  This test sets the log level to DEBUG8
 * and then verifies that all logs (except those compiled off) are passed through to the emit() method.
 */
TEST_F(LoggerTest, logDebug8Level) {
    setLevelExpectations(Level::DEBUG8);
    exerciseLevels();
}

/**
 * Test delivery of log messages when the log level is set to DEBUG7.  This test sets the log level to DEBUG7
 * and then verifies that all logs (except those compiled off) are passed through to the emit() method.
 */
TEST_F(LoggerTest, logDebug7Level) {
    setLevelExpectations(Level::DEBUG7);
    exerciseLevels();
}

/**
 * Test delivery of log messages when the log level is set to DEBUG6.  This test sets the log level to DEBUG6
 * and then verifies that all logs (except those compiled off) are passed through to the emit() method.
 */
TEST_F(LoggerTest, logDebug6Level) {
    setLevelExpectations(Level::DEBUG6);
    exerciseLevels();
}

/**
 * Test delivery of log messages when the log level is set to DEBUG5.  This test sets the log level to DEBUG5
 * and then verifies that all logs (except those compiled off) are passed through to the emit() method.
 */
TEST_F(LoggerTest, logDebug5Level) {
    setLevelExpectations(Level::DEBUG5);
    exerciseLevels();
}

/**
 * Test delivery of log messages when the log level is set to DEBUG4.  This test sets the log level to DEBUG4
 * and then verifies that all logs (except those compiled off) are passed through to the emit() method.
 */
TEST_F(LoggerTest, logDebug4Level) {
    setLevelExpectations(Level::DEBUG4);
    exerciseLevels();
}

/**
 * Test delivery of log messages when the log level is set to DEBUG3.  This test sets the log level to DEBUG3
 * and then verifies that all logs (except those compiled off) are passed through to the emit() method.
 */
TEST_F(LoggerTest, logDebug3Level) {
    setLevelExpectations(Level::DEBUG3);
    exerciseLevels();
}

/**
 * Test delivery of log messages when the log level is set to DEBUG2.  This test sets the log level to DEBUG2
 * and then verifies that all logs (except those compiled off) are passed through to the emit() method.
 */
TEST_F(LoggerTest, logDebug2Level) {
    setLevelExpectations(Level::DEBUG2);
    exerciseLevels();
}

/**
 * Test delivery of log messages when the log level is set to DEBUG1.  This test sets the log level to DEBUG1
 * and then verifies that only logs of DEBUG or above are passed through to the emit() method.
 */
TEST_F(LoggerTest, logDebug1Level) {
    setLevelExpectations(Level::DEBUG1);
    exerciseLevels();
}

/**
 * Test delivery of log messages when the log level is set to DEBUG1.  This test sets the log level to DEBUG0
 * and then verifies that only logs of DEBUG or above are passed through to the emit() method.
 */
TEST_F(LoggerTest, logDebug0Level) {
    setLevelExpectations(Level::DEBUG0);
    exerciseLevels();
}

/**
 * Test delivery of log messages when the log level is set to INFO.  This test sets the log level to INFO
 * and then verifies that only logs of INFO or above are passed through to the emit() method.
 */
TEST_F(LoggerTest, logInfoLevel) {
    setLevelExpectations(Level::INFO);
    exerciseLevels();
}

/**
 * Test delivery of log messages when the log level is set to WARN.  This test sets the log level to WARN
 * and then verifies that only logs of WARN or above are passed through to the emit() method.
 */
TEST_F(LoggerTest, logWarnLevel) {
    setLevelExpectations(Level::WARN);
    exerciseLevels();
}

/**
 * Test delivery of log messages when the log level is set to ERROR.  This test sets the log level to ERROR
 * and then verifies that only logs of ERROR or above are passed through to the emit() method.
 */
TEST_F(LoggerTest, logErrorLevel) {
    setLevelExpectations(Level::ERROR);
    exerciseLevels();
}

/**
 * Test delivery of log messages when the log level is set to CRITICAL.  This test sets the log level to CRITICAL
 * and then verifies that only CRITICAL logs are passed through to the emit() method.
 */
TEST_F(LoggerTest, logCriticalLevel) {
    setLevelExpectations(Level::CRITICAL);
    exerciseLevels();
}

/**
 * Test delivery of log messages when the log level is set to NONE.  This test sets the log level to NONE
 * and then verifies that no logs are passed through to the emit() method.
 */
TEST_F(LoggerTest, logNoneLevel) {
    setLevelExpectations(Level::NONE);
    exerciseLevels();
}

/**
 * Test to ensure that logger usage with possible nullptr inputs is robust.  As some functionality is templated,
 * we must test both char* and const char* variants, for LogEntry construction, and the .d() and .m() functionality.
 */
TEST_F(LoggerTest, testNullInputs) {
    ACSDK_GET_LOGGER_FUNCTION().setLevel(Level::INFO);

    EXPECT_CALL(*(g_log.get()), emit(Level::INFO, _, _, _)).Times(13);

    // The good case.
    ACSDK_INFO(LX("testEntryName").d("key", "value"));

    // Test the constructors.
    ACSDK_INFO(LX(TEST_MESSAGE_CONST_NULL_CHAR_PTR).m("testEventNameConstNullPtr"));
    ACSDK_INFO(LX(TEST_MESSAGE_NULL_CHAR_PTR).m("testEventNameNullPtr"));

    // Test the .d() bad variants, both params.
    ACSDK_INFO(LX("testEntryName").d("key", TEST_MESSAGE_CONST_NULL_CHAR_PTR));
    ACSDK_INFO(LX("testEntryName").d("key", TEST_MESSAGE_NULL_CHAR_PTR));
    ACSDK_INFO(LX("testEntryName").d(TEST_MESSAGE_CONST_NULL_CHAR_PTR, "value"));
    ACSDK_INFO(LX("testEntryName").d(TEST_MESSAGE_NULL_CHAR_PTR, "value"));
    ACSDK_INFO(LX("testEntryName").d(TEST_MESSAGE_CONST_NULL_CHAR_PTR, TEST_MESSAGE_CONST_NULL_CHAR_PTR));
    ACSDK_INFO(LX("testEntryName").d(TEST_MESSAGE_CONST_NULL_CHAR_PTR, TEST_MESSAGE_NULL_CHAR_PTR));
    ACSDK_INFO(LX("testEntryName").d(TEST_MESSAGE_NULL_CHAR_PTR, TEST_MESSAGE_CONST_NULL_CHAR_PTR));
    ACSDK_INFO(LX("testEntryName").d(TEST_MESSAGE_NULL_CHAR_PTR, TEST_MESSAGE_NULL_CHAR_PTR));

    // Test the .m() variants.
    ACSDK_INFO(LX("testEntryName").m(TEST_MESSAGE_CONST_NULL_CHAR_PTR));
    ACSDK_INFO(LX("testEntryName").m(TEST_MESSAGE_NULL_CHAR_PTR));
}

/**
 * Test delivery of appropriate time values from the logging system.  This test captures the current time
 * both before and after an invocation of ACDK_LOG_INFO and verifies that the time value passed to the
 * emit() method is between (inclusive) the before and after times.
 */
TEST_F(LoggerTest, verifyTime) {
    ACSDK_GET_LOGGER_FUNCTION().setLevel(Level::INFO);

    EXPECT_CALL(*(g_log.get()), emit(Level::INFO, _, _, _)).Times(1);
    auto beforeTime = std::chrono::system_clock::now();
    ACSDK_INFO(LX("testing time"));
    auto afterTime = std::chrono::system_clock::now();
    ASSERT_LE(beforeTime, g_log->m_lastTime);
    ASSERT_LE(g_log->m_lastTime, afterTime);
}

/**
 * Test delivery of appropriate thread moniker values from the logging system.  This test invokes ACSDK_INFO from
 * two threads and verifies that the thread moniker values passed to the emit() method are in fact different.
 */
TEST_F(LoggerTest, verifyThreadMoniker) {
    ACSDK_GET_LOGGER_FUNCTION().setLevel(Level::INFO);
    EXPECT_CALL(*(g_log.get()), emit(Level::INFO, _, _, _)).Times(2);
    ACSDK_INFO(LX("testing threadMoniker (1 of 2)"));
    auto firstThreadMoniker = g_log->m_lastThreadMoniker;
    std::thread secondThread([firstThreadMoniker]() {
        ACSDK_INFO(LX("testing threadMoniker (2 of 2)"));
        ASSERT_NE(firstThreadMoniker, g_log->m_lastThreadMoniker);
    });
    secondThread.join();
}

/**
 * Test passing the source name through the logging system.  The source name is a parameter of the LogEntry
 * constructor.  Expect that the source parameter passed to the LogEntry constructor is included in the text
 * passed to the emit() method.
 */
TEST_F(LoggerTest, verifySource) {
    ACSDK_GET_LOGGER_FUNCTION().setLevel(Level::INFO);
    EXPECT_CALL(*(g_log.get()), emit(Level::INFO, _, _, _)).Times(1);
    ACSDK_INFO(LX("random_event"));
    ASSERT_NE(g_log->m_lastText.find(TEST_SOURCE_STRING), std::string::npos);
}

/**
 * Test passing the event name through the logging system.  The event name is a parameter of the LogEntry
 * constructor.  Expect that the event parameter passed to the LogEntry constructor is included in the text
 * passed to the emit() method.
 */
TEST_F(LoggerTest, verifyEvent) {
    ACSDK_GET_LOGGER_FUNCTION().setLevel(Level::INFO);
    EXPECT_CALL(*(g_log.get()), emit(Level::INFO, _, _, _)).Times(1);
    std::string event(TEST_EVENT_STRING);
    ACSDK_INFO(LX(event));
    ASSERT_NE(g_log->m_lastText.find(TEST_EVENT_STRING), std::string::npos);
}

/**
 * Test passing a metadata parameter to the logging system.  Invokes ACSDK_INFO with a metadata parameter.
 * Expects that that metadata is constructed properly (including the escaping of reserved characters) and that
 * both the key and escaped value are included in the text passed to the emit() method.
 */
TEST_F(LoggerTest, verifyMetadata) {
    ACSDK_GET_LOGGER_FUNCTION().setLevel(Level::INFO);
    EXPECT_CALL(*(g_log.get()), emit(Level::INFO, _, _, _)).Times(1);
    ACSDK_INFO(LX("testing metadata")
                   .d(METADATA_KEY, UNESCAPED_METADATA_VALUE)
                   .d(METADATA_KEY_TRUE, true)
                   .d(METADATA_KEY_FALSE, false));
    ASSERT_NE(g_log->m_lastText.find(METADATA_KEY KEY_VALUE_SEPARATOR ESCAPED_METADATA_VALUE), std::string::npos);
    ASSERT_NE(g_log->m_lastText.find(METADATA_EXPECTED_BOOLEANS), std::string::npos);
}

/**
 * Test passing a message parameter to the logging system.  Invokes ACSDK_INFO with a message parameter.
 * Expects that the message is included in text passed to the emit() method.
 */
TEST_F(LoggerTest, verifyMessage) {
    ACSDK_GET_LOGGER_FUNCTION().setLevel(Level::INFO);
    EXPECT_CALL(*(g_log.get()), emit(Level::INFO, _, _, _)).Times(1);
    std::string message(TEST_MESSAGE_STRING);
    ACSDK_INFO(LX("testing message").m(message));
    ASSERT_NE(g_log->m_lastText.find(TEST_MESSAGE_STRING), std::string::npos);
}

/**
 * Test passing sensitive data to the logging system.  It should only be emitted in DEBUG builds.
 */
TEST_F(LoggerTest, testSensitiveDataSuppressed) {
    ACSDK_GET_LOGGER_FUNCTION().setLevel(Level::INFO);
    EXPECT_CALL(*(g_log.get()), emit(Level::INFO, _, _, _)).Times(1);
    ACSDK_INFO(LX("testing metadata").sensitive(METADATA_KEY, UNESCAPED_METADATA_VALUE));
    auto result = g_log->m_lastText.find(METADATA_KEY KEY_VALUE_SEPARATOR ESCAPED_METADATA_VALUE) != std::string::npos;
#ifdef ACSDK_EMIT_SENSITIVE_LOGS
    ASSERT_TRUE(result);
#else
    ASSERT_FALSE(result);
#endif
}

/**
 * Test observer mechanism in the MockModuleLogger.  Expects that when the logLevel changes for the sink, the
 * callback of the MockModuleLogger is triggered.  Also make sure any changes to sink's logLevel is ignored
 * after the MockModuleLogger's logLevel has been set.
 */
TEST_F(LoggerTest, testModuleLoggerObserver) {
    MockModuleLogger mockModuleLogger;
    getLoggerTestLogger()->setLevel(Level::WARN);
    ASSERT_EQ(mockModuleLogger.getLogLevel(), Level::WARN);
    mockModuleLogger.setLevel(Level::CRITICAL);
    ASSERT_EQ(mockModuleLogger.getLogLevel(), Level::CRITICAL);
    getLoggerTestLogger()->setLevel(Level::NONE);
    ASSERT_EQ(mockModuleLogger.getLogLevel(), Level::NONE);
}

/**
 * Test observer mechanism with multiple observers.  Expects all observers to be notified of the logLevel change.
 */
TEST_F(LoggerTest, testMultipleModuleLoggerObservers) {
    MockModuleLogger mockModuleLogger1;
    MockModuleLogger mockModuleLogger2;
    MockModuleLogger mockModuleLogger3;

    getLoggerTestLogger()->setLevel(Level::WARN);
    ASSERT_EQ(mockModuleLogger1.getLogLevel(), Level::WARN);
    ASSERT_EQ(mockModuleLogger2.getLogLevel(), Level::WARN);
    ASSERT_EQ(mockModuleLogger3.getLogLevel(), Level::WARN);

    mockModuleLogger1.setLevel(Level::CRITICAL);
    ASSERT_EQ(mockModuleLogger1.getLogLevel(), Level::CRITICAL);
    ASSERT_EQ(mockModuleLogger2.getLogLevel(), Level::WARN);
    ASSERT_EQ(mockModuleLogger3.getLogLevel(), Level::WARN);

    getLoggerTestLogger()->setLevel(Level::NONE);
    ASSERT_EQ(mockModuleLogger1.getLogLevel(), Level::NONE);
    ASSERT_EQ(mockModuleLogger2.getLogLevel(), Level::NONE);
    ASSERT_EQ(mockModuleLogger3.getLogLevel(), Level::NONE);
}

/**
 * Test changing of sink logger using the LoggerSinkManager.  Expect the sink in
 * ModuleLoggers will be changed.
 */
TEST_F(LoggerTest, testChangeSinkLogger) {
    std::shared_ptr<MockLogger> sink1 = MockLogger::create();
    std::shared_ptr<Logger> sink1Logger = sink1;

    // reset loglevel to INFO
    getLoggerTestLogger()->setLevel(Level::INFO);

    // ModuleLoggers uses TestLogger as sink, so there shouldn't be any message
    // sent to sink1
    ACSDK_INFO(LX(TEST_MESSAGE_STRING));
    ASSERT_NE(g_log->m_lastText.find(TEST_MESSAGE_STRING), std::string::npos);
    ASSERT_EQ(sink1->m_lastText.find(TEST_MESSAGE_STRING), std::string::npos);

    // change to use sink1, now log message should be sent to sink1
    LoggerSinkManager::instance().initialize(sink1Logger);
    ACSDK_INFO(LX(TEST_MESSAGE_STRING_1));
    ASSERT_NE(g_log->m_lastText.find(TEST_MESSAGE_STRING), std::string::npos);
    ASSERT_NE(sink1->m_lastText.find(TEST_MESSAGE_STRING_1), std::string::npos);

    // reset to the default sink to avoid messing up with subsequent tests
    LoggerSinkManager::instance().initialize(getLoggerTestLogger());
}

}  // namespace test
}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
