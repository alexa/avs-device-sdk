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

#include <memory>
#include <string>
#include <ostream>
#include <istream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <Settings/SettingStringConversion.h>

namespace alexaClientSDK {
namespace settings {
namespace test {

using namespace avsCommon::utils::json;

/// The key used for converting the hello object.
static const std::string KEY = "key";

/// The value used to initialize the hello object.
static const std::string INIT_VALUE = "value";

/**
 * Define an enumeration for testing string conversion.
 */
enum class HelloEnum { HI, THERE };

/**
 * Define an operator<< for the test enum @c HelloEnum.
 * @param os The output stream.
 * @param hello The enumeration to be converted.
 * @return The output stream. The failbit is set if the conversion fails.
 */
std::ostream& operator<<(std::ostream& os, const HelloEnum& hello) {
    if (HelloEnum::HI == hello) {
        os << "HI";
    } else if (HelloEnum::THERE == hello) {
        os << "THERE";
    } else {
        os.setstate(std::ios_base::failbit);
    }
    return os;
}

/**
 * Define an operator>> for the test enum @c HelloEnum.
 *
 * @param is The input stream.
 * @param[out] hello The enum that will receive the new value.
 * @return The input stream. The failbit is set if the conversion fails.
 */
std::istream& operator>>(std::istream& is, HelloEnum& hello) {
    std::string str;
    is >> str;
    if ("HI" == str) {
        hello = HelloEnum::HI;
    } else if ("THERE" == str) {
        hello = HelloEnum::THERE;
    } else {
        is.setstate(std::ios_base::failbit);
    }
    return is;
}

/**
 * Dummy class that only has a name key.
 */
class HelloClass {
public:
    /**
     * The constructor.
     */
    HelloClass();

    /**
     * Compares two instances of HelloClass
     *
     * @param rhs The right hand size.
     * @return @c true if name is the same and @c false otherwise.
     */
    bool operator==(const HelloClass& rhs) const;

    /// Just a string to check the conversion succeeds.
    std::string m_name = INIT_VALUE;
};

HelloClass::HelloClass() : m_name{INIT_VALUE} {
}

bool HelloClass::operator==(const HelloClass& rhs) const {
    return m_name == rhs.m_name;
}

/**
 * Define an operator<< for the test class @c HelloClass.
 *
 * @param os The output stream.
 * @param hello The object to be converted.
 * @return The output stream. The failbit is set if the conversion fails.
 */
std::ostream& operator<<(std::ostream& os, const HelloClass& hello) {
    JsonGenerator jsonGenerator;
    EXPECT_TRUE(jsonGenerator.addMember(KEY, hello.m_name));
    os << jsonGenerator.toString();
    return os;
}

/**
 * Define an operator>> for the test class @c HelloClass.
 *
 * @param is The input stream.
 * @param[out] hello The object that will receive the new value.
 * @return The input stream. The failbit is set if the conversion fails.
 */
std::istream& operator>>(std::istream& is, HelloClass& hello) {
    std::string json;
    is >> json;
    if (!jsonUtils::retrieveValue(json, KEY, &hello.m_name)) {
        is.setstate(std::ios_base::failbit);
    }
    return is;
}

std::pair<bool, std::string> expected(bool result, std::string value) {
    return {result, value};
};

/// Test boolean conversions.
TEST(SettingStringConversionTest, testBoolConversion) {
    // Valid conversions
    EXPECT_EQ(toSettingString<bool>(false), expected(true, "false"));
    EXPECT_EQ(toSettingString<bool>(true), expected(true, "true"));

    EXPECT_EQ(fromSettingString<bool>("false", true), std::make_pair(true, false));
    EXPECT_EQ(fromSettingString<bool>("true", false), std::make_pair(true, true));

    // Invalid conversion
    EXPECT_EQ(fromSettingString<bool>("not bool", false), std::make_pair(false, false));
};

TEST(SettingStringConversionTest, testIntegralByteSize) {
    // Valid conversions
    EXPECT_EQ(fromSettingString<int8_t>("10", 100).first, true);
    EXPECT_EQ(fromSettingString<int8_t>("10", 100).second, 10);
    EXPECT_EQ(fromSettingString<uint8_t>("10", 100).second, 10u);

    EXPECT_EQ(toSettingString<int8_t>(10), expected(true, "10"));
    EXPECT_EQ(toSettingString<uint8_t>(10), expected(true, "10"));

    // Invalid conversion
    EXPECT_EQ(fromSettingString<int8_t>("not int", 10).first, false);
    EXPECT_EQ(fromSettingString<int8_t>("not int", 10).second, 10);
};

TEST(SettingStringConversionTest, testArithmeticTypes) {
    // Valid conversions
    EXPECT_EQ(toSettingString<char>('a'), expected(true, "a"));
    EXPECT_EQ(toSettingString<int>(10), expected(true, "10"));
    EXPECT_EQ(toSettingString<int>(-10), expected(true, "-10"));
    EXPECT_EQ(toSettingString<double>(10.2), expected(true, "10.2"));
    EXPECT_EQ(toSettingString<double>(1.2e10), expected(true, "1.2e+10"));

    EXPECT_EQ(fromSettingString<char>("a", 'b'), std::make_pair(true, 'a'));
    EXPECT_EQ(fromSettingString<int>("10", 100), std::make_pair(true, 10));
    EXPECT_EQ(fromSettingString<int>("-10", 100), std::make_pair(true, -10));
    EXPECT_EQ(fromSettingString<double>("10.2", 2.2), std::make_pair(true, 10.2));
    EXPECT_EQ(fromSettingString<double>("1.2e10", 2.2), std::make_pair(true, 1.2e+10));

    // Invalid conversion
    EXPECT_EQ(fromSettingString<int>("not int", 100), std::make_pair(false, 100));
    EXPECT_EQ(fromSettingString<double>("not double", 2.2), std::make_pair(false, 2.2));
}

TEST(SettingStringConversionTest, testFromEnum) {
    // Valid conversions
    EXPECT_EQ(toSettingString<HelloEnum>(HelloEnum::HI), expected(true, R"("HI")"));
    EXPECT_EQ(fromSettingString<HelloEnum>(R"("THERE")", HelloEnum::HI), std::make_pair(true, HelloEnum::THERE));

    // Invalid conversion
    EXPECT_EQ(fromSettingString<HelloEnum>(R"("BLAH")", HelloEnum::HI), std::make_pair(false, HelloEnum::HI));
    EXPECT_EQ(fromSettingString<HelloEnum>("", HelloEnum::HI), std::make_pair(false, HelloEnum::HI));
    EXPECT_EQ(fromSettingString<HelloEnum>("-THERE-", HelloEnum::HI), std::make_pair(false, HelloEnum::HI));
}

TEST(SettingStringConversionTest, testFromClass) {
    // Valid conversions
    HelloClass newValue;
    newValue.m_name = "newValue";

    EXPECT_EQ(toSettingString<HelloClass>(HelloClass()), expected(true, R"({"key":"value"})"));
    EXPECT_EQ(fromSettingString<HelloClass>(R"({"key":"newValue"})", HelloClass()), std::make_pair(true, newValue));

    // Invalid conversion
    EXPECT_EQ(fromSettingString<HelloClass>("invalid json", HelloClass()), std::make_pair(false, HelloClass()));
}

}  // namespace test
}  // namespace settings
}  // namespace alexaClientSDK
