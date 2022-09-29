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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_ACSDK_TEST_GMOCKEXTENSIONS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_ACSDK_TEST_GMOCKEXTENSIONS_H_

#include <gmock/gmock.h>

/**
 * @brief Internal macro for constructing method name.
 *
 * Macro constructs method name in a manner similar to GMOCK_MOCKER_.
 *
 * @param arity     Parameter count.
 * @param constness Optional @a const specifier.
 * @param Method    Method name.
 *
 * @return Method name.
 */
#define GMOCK_NOEXCEPT_MOCKER_(arity, constness, Method) \
    GTEST_CONCAT_TOKEN_(gmock##constness##noexcept##arity##_##Method##_, __LINE__)

/**
 * @{
 * @brief Internal macro for declaring and defining mock method.
 *
 * Macro declares and defines mock method with @a noexcept specifier. Otherwise it works similarly to macros
 * GMOCK_METHOD0_ ... GMOCK_METHOD10_.
 *
 * @param tn        Number of arguments.
 * @param constness Optional @a const specifier.
 * @param Method    Method name.
 */
#define GMOCK_NOEXCEPT_METHOD0_(tn, constness, ct, Method, ...)                                                \
    GMOCK_RESULT_(tn, __VA_ARGS__) ct Method() constness noexcept {                                            \
        GTEST_COMPILE_ASSERT_(                                                                                 \
            (::testing::tuple_size<tn ::testing::internal::Function<__VA_ARGS__>::ArgumentTuple>::value == 0), \
            this_method_does_not_take_0_arguments);                                                            \
        GMOCK_NOEXCEPT_MOCKER_(0, constness, Method).SetOwnerAndName(this, #Method);                           \
        return GMOCK_NOEXCEPT_MOCKER_(0, constness, Method).Invoke();                                          \
    }                                                                                                          \
    ::testing::MockSpec<__VA_ARGS__>& gmock_##Method() constness noexcept {                                    \
        GMOCK_NOEXCEPT_MOCKER_(0, constness, Method).RegisterOwner(this);                                      \
        return GMOCK_NOEXCEPT_MOCKER_(0, constness, Method).With();                                            \
    }                                                                                                          \
    mutable ::testing::FunctionMocker<__VA_ARGS__> GMOCK_NOEXCEPT_MOCKER_(0, constness, Method)

#define GMOCK_NOEXCEPT_METHOD1_(tn, constness, ct, Method, ...)                                                        \
    GMOCK_RESULT_(tn, __VA_ARGS__) ct Method(GMOCK_ARG_(tn, 1, __VA_ARGS__) gmock_a1) constness noexcept {             \
        GTEST_COMPILE_ASSERT_(                                                                                         \
            (::testing::tuple_size<tn ::testing::internal::Function<__VA_ARGS__>::ArgumentTuple>::value == 1),         \
            this_method_does_not_take_1_argument);                                                                     \
        GMOCK_NOEXCEPT_MOCKER_(1, constness, Method).SetOwnerAndName(this, #Method);                                   \
        return GMOCK_NOEXCEPT_MOCKER_(1, constness, Method).Invoke(gmock_a1);                                          \
    }                                                                                                                  \
    ::testing::MockSpec<__VA_ARGS__>& gmock_##Method(GMOCK_MATCHER_(tn, 1, __VA_ARGS__) gmock_a1) constness noexcept { \
        GMOCK_NOEXCEPT_MOCKER_(1, constness, Method).RegisterOwner(this);                                              \
        return GMOCK_NOEXCEPT_MOCKER_(1, constness, Method).With(gmock_a1);                                            \
    }                                                                                                                  \
    mutable ::testing::FunctionMocker<__VA_ARGS__> GMOCK_NOEXCEPT_MOCKER_(1, constness, Method)

#define GMOCK_NOEXCEPT_METHOD2_(tn, constness, ct, Method, ...)                                                        \
    GMOCK_RESULT_(tn, __VA_ARGS__)                                                                                     \
    ct Method(GMOCK_ARG_(tn, 1, __VA_ARGS__) gmock_a1, GMOCK_ARG_(tn, 2, __VA_ARGS__) gmock_a2) constness noexcept {   \
        GTEST_COMPILE_ASSERT_(                                                                                         \
            (::testing::tuple_size<tn ::testing::internal::Function<__VA_ARGS__>::ArgumentTuple>::value == 2),         \
            this_method_does_not_take_2_arguments);                                                                    \
        GMOCK_NOEXCEPT_MOCKER_(2, constness, Method).SetOwnerAndName(this, #Method);                                   \
        return GMOCK_NOEXCEPT_MOCKER_(2, constness, Method).Invoke(gmock_a1, gmock_a2);                                \
    }                                                                                                                  \
    ::testing::MockSpec<__VA_ARGS__>& gmock_##Method(                                                                  \
        GMOCK_MATCHER_(tn, 1, __VA_ARGS__) gmock_a1, GMOCK_MATCHER_(tn, 2, __VA_ARGS__) gmock_a2) constness noexcept { \
        GMOCK_NOEXCEPT_MOCKER_(2, constness, Method).RegisterOwner(this);                                              \
        return GMOCK_NOEXCEPT_MOCKER_(2, constness, Method).With(gmock_a1, gmock_a2);                                  \
    }                                                                                                                  \
    mutable ::testing::FunctionMocker<__VA_ARGS__> GMOCK_NOEXCEPT_MOCKER_(2, constness, Method)

#define GMOCK_NOEXCEPT_METHOD3_(tn, constness, ct, Method, ...)                                                \
    GMOCK_RESULT_(tn, __VA_ARGS__)                                                                             \
    ct Method(                                                                                                 \
        GMOCK_ARG_(tn, 1, __VA_ARGS__) gmock_a1,                                                               \
        GMOCK_ARG_(tn, 2, __VA_ARGS__) gmock_a2,                                                               \
        GMOCK_ARG_(tn, 3, __VA_ARGS__) gmock_a3) constness noexcept {                                          \
        GTEST_COMPILE_ASSERT_(                                                                                 \
            (::testing::tuple_size<tn ::testing::internal::Function<__VA_ARGS__>::ArgumentTuple>::value == 3), \
            this_method_does_not_take_3_arguments);                                                            \
        GMOCK_NOEXCEPT_MOCKER_(3, constness, Method).SetOwnerAndName(this, #Method);                           \
        return GMOCK_NOEXCEPT_MOCKER_(3, constness, Method).Invoke(gmock_a1, gmock_a2, gmock_a3);              \
    }                                                                                                          \
    ::testing::MockSpec<__VA_ARGS__>& gmock_##Method(                                                          \
        GMOCK_MATCHER_(tn, 1, __VA_ARGS__) gmock_a1,                                                           \
        GMOCK_MATCHER_(tn, 2, __VA_ARGS__) gmock_a2,                                                           \
        GMOCK_MATCHER_(tn, 3, __VA_ARGS__) gmock_a3) constness noexcept {                                      \
        GMOCK_NOEXCEPT_MOCKER_(3, constness, Method).RegisterOwner(this);                                      \
        return GMOCK_NOEXCEPT_MOCKER_(3, constness, Method).With(gmock_a1, gmock_a2, gmock_a3);                \
    }                                                                                                          \
    mutable ::testing::FunctionMocker<__VA_ARGS__> GMOCK_NOEXCEPT_MOCKER_(3, constness, Method)

#define GMOCK_NOEXCEPT_METHOD4_(tn, constness, ct, Method, ...)                                                \
    GMOCK_RESULT_(tn, __VA_ARGS__)                                                                             \
    ct Method(                                                                                                 \
        GMOCK_ARG_(tn, 1, __VA_ARGS__) gmock_a1,                                                               \
        GMOCK_ARG_(tn, 2, __VA_ARGS__) gmock_a2,                                                               \
        GMOCK_ARG_(tn, 3, __VA_ARGS__) gmock_a3,                                                               \
        GMOCK_ARG_(tn, 4, __VA_ARGS__) gmock_a4) constness noexcept {                                          \
        GTEST_COMPILE_ASSERT_(                                                                                 \
            (::testing::tuple_size<tn ::testing::internal::Function<__VA_ARGS__>::ArgumentTuple>::value == 4), \
            this_method_does_not_take_4_arguments);                                                            \
        GMOCK_NOEXCEPT_MOCKER_(4, constness, Method).SetOwnerAndName(this, #Method);                           \
        return GMOCK_NOEXCEPT_MOCKER_(4, constness, Method).Invoke(gmock_a1, gmock_a2, gmock_a3, gmock_a4);    \
    }                                                                                                          \
    ::testing::MockSpec<__VA_ARGS__>& gmock_##Method(                                                          \
        GMOCK_MATCHER_(tn, 1, __VA_ARGS__) gmock_a1,                                                           \
        GMOCK_MATCHER_(tn, 2, __VA_ARGS__) gmock_a2,                                                           \
        GMOCK_MATCHER_(tn, 3, __VA_ARGS__) gmock_a3,                                                           \
        GMOCK_MATCHER_(tn, 4, __VA_ARGS__) gmock_a4) constness noexcept {                                      \
        GMOCK_NOEXCEPT_MOCKER_(4, constness, Method).RegisterOwner(this);                                      \
        return GMOCK_NOEXCEPT_MOCKER_(4, constness, Method).With(gmock_a1, gmock_a2, gmock_a3, gmock_a4);      \
    }                                                                                                          \
    mutable ::testing::FunctionMocker<__VA_ARGS__> GMOCK_NOEXCEPT_MOCKER_(4, constness, Method)

#define GMOCK_NOEXCEPT_METHOD5_(tn, constness, ct, Method, ...)                                                       \
    GMOCK_RESULT_(tn, __VA_ARGS__)                                                                                    \
    ct Method(                                                                                                        \
        GMOCK_ARG_(tn, 1, __VA_ARGS__) gmock_a1,                                                                      \
        GMOCK_ARG_(tn, 2, __VA_ARGS__) gmock_a2,                                                                      \
        GMOCK_ARG_(tn, 3, __VA_ARGS__) gmock_a3,                                                                      \
        GMOCK_ARG_(tn, 4, __VA_ARGS__) gmock_a4,                                                                      \
        GMOCK_ARG_(tn, 5, __VA_ARGS__) gmock_a5) constness noexcept {                                                 \
        GTEST_COMPILE_ASSERT_(                                                                                        \
            (::testing::tuple_size<tn ::testing::internal::Function<__VA_ARGS__>::ArgumentTuple>::value == 5),        \
            this_method_does_not_take_5_arguments);                                                                   \
        GMOCK_NOEXCEPT_MOCKER_(5, constness, Method).SetOwnerAndName(this, #Method);                                  \
        return GMOCK_NOEXCEPT_MOCKER_(5, constness, Method).Invoke(gmock_a1, gmock_a2, gmock_a3, gmock_a4, gmock_a5); \
    }                                                                                                                 \
    ::testing::MockSpec<__VA_ARGS__>& gmock_##Method(                                                                 \
        GMOCK_MATCHER_(tn, 1, __VA_ARGS__) gmock_a1,                                                                  \
        GMOCK_MATCHER_(tn, 2, __VA_ARGS__) gmock_a2,                                                                  \
        GMOCK_MATCHER_(tn, 3, __VA_ARGS__) gmock_a3,                                                                  \
        GMOCK_MATCHER_(tn, 4, __VA_ARGS__) gmock_a4,                                                                  \
        GMOCK_MATCHER_(tn, 5, __VA_ARGS__) gmock_a5) constness noexcept {                                             \
        GMOCK_NOEXCEPT_MOCKER_(5, constness, Method).RegisterOwner(this);                                             \
        return GMOCK_NOEXCEPT_MOCKER_(5, constness, Method).With(gmock_a1, gmock_a2, gmock_a3, gmock_a4, gmock_a5);   \
    }                                                                                                                 \
    mutable ::testing::FunctionMocker<__VA_ARGS__> GMOCK_NOEXCEPT_MOCKER_(5, constness, Method)

#define GMOCK_NOEXCEPT_METHOD6_(tn, constness, ct, Method, ...)                                                \
    GMOCK_RESULT_(tn, __VA_ARGS__)                                                                             \
    ct Method(                                                                                                 \
        GMOCK_ARG_(tn, 1, __VA_ARGS__) gmock_a1,                                                               \
        GMOCK_ARG_(tn, 2, __VA_ARGS__) gmock_a2,                                                               \
        GMOCK_ARG_(tn, 3, __VA_ARGS__) gmock_a3,                                                               \
        GMOCK_ARG_(tn, 4, __VA_ARGS__) gmock_a4,                                                               \
        GMOCK_ARG_(tn, 5, __VA_ARGS__) gmock_a5,                                                               \
        GMOCK_ARG_(tn, 6, __VA_ARGS__) gmock_a6) constness noexcept {                                          \
        GTEST_COMPILE_ASSERT_(                                                                                 \
            (::testing::tuple_size<tn ::testing::internal::Function<__VA_ARGS__>::ArgumentTuple>::value == 6), \
            this_method_does_not_take_6_arguments);                                                            \
        GMOCK_NOEXCEPT_MOCKER_(6, constness, Method).SetOwnerAndName(this, #Method);                           \
        return GMOCK_NOEXCEPT_MOCKER_(6, constness, Method)                                                    \
            .Invoke(gmock_a1, gmock_a2, gmock_a3, gmock_a4, gmock_a5, gmock_a6);                               \
    }                                                                                                          \
    ::testing::MockSpec<__VA_ARGS__>& gmock_##Method(                                                          \
        GMOCK_MATCHER_(tn, 1, __VA_ARGS__) gmock_a1,                                                           \
        GMOCK_MATCHER_(tn, 2, __VA_ARGS__) gmock_a2,                                                           \
        GMOCK_MATCHER_(tn, 3, __VA_ARGS__) gmock_a3,                                                           \
        GMOCK_MATCHER_(tn, 4, __VA_ARGS__) gmock_a4,                                                           \
        GMOCK_MATCHER_(tn, 5, __VA_ARGS__) gmock_a5,                                                           \
        GMOCK_MATCHER_(tn, 6, __VA_ARGS__) gmock_a6) constness noexcept {                                      \
        GMOCK_NOEXCEPT_MOCKER_(6, constness, Method).RegisterOwner(this);                                      \
        return GMOCK_NOEXCEPT_MOCKER_(6, constness, Method)                                                    \
            .With(gmock_a1, gmock_a2, gmock_a3, gmock_a4, gmock_a5, gmock_a6);                                 \
    }                                                                                                          \
    mutable ::testing::FunctionMocker<__VA_ARGS__> GMOCK_NOEXCEPT_MOCKER_(6, constness, Method)

#define GMOCK_NOEXCEPT_METHOD7_(tn, constness, ct, Method, ...)                                                \
    GMOCK_RESULT_(tn, __VA_ARGS__)                                                                             \
    ct Method(                                                                                                 \
        GMOCK_ARG_(tn, 1, __VA_ARGS__) gmock_a1,                                                               \
        GMOCK_ARG_(tn, 2, __VA_ARGS__) gmock_a2,                                                               \
        GMOCK_ARG_(tn, 3, __VA_ARGS__) gmock_a3,                                                               \
        GMOCK_ARG_(tn, 4, __VA_ARGS__) gmock_a4,                                                               \
        GMOCK_ARG_(tn, 5, __VA_ARGS__) gmock_a5,                                                               \
        GMOCK_ARG_(tn, 6, __VA_ARGS__) gmock_a6,                                                               \
        GMOCK_ARG_(tn, 7, __VA_ARGS__) gmock_a7) constness noexcept {                                          \
        GTEST_COMPILE_ASSERT_(                                                                                 \
            (::testing::tuple_size<tn ::testing::internal::Function<__VA_ARGS__>::ArgumentTuple>::value == 7), \
            this_method_does_not_take_7_arguments);                                                            \
        GMOCK_NOEXCEPT_MOCKER_(7, constness, Method).SetOwnerAndName(this, #Method);                           \
        return GMOCK_NOEXCEPT_MOCKER_(7, constness, Method)                                                    \
            .Invoke(gmock_a1, gmock_a2, gmock_a3, gmock_a4, gmock_a5, gmock_a6, gmock_a7);                     \
    }                                                                                                          \
    ::testing::MockSpec<__VA_ARGS__>& gmock_##Method(                                                          \
        GMOCK_MATCHER_(tn, 1, __VA_ARGS__) gmock_a1,                                                           \
        GMOCK_MATCHER_(tn, 2, __VA_ARGS__) gmock_a2,                                                           \
        GMOCK_MATCHER_(tn, 3, __VA_ARGS__) gmock_a3,                                                           \
        GMOCK_MATCHER_(tn, 4, __VA_ARGS__) gmock_a4,                                                           \
        GMOCK_MATCHER_(tn, 5, __VA_ARGS__) gmock_a5,                                                           \
        GMOCK_MATCHER_(tn, 6, __VA_ARGS__) gmock_a6,                                                           \
        GMOCK_MATCHER_(tn, 7, __VA_ARGS__) gmock_a7) constness noexcept {                                      \
        GMOCK_NOEXCEPT_MOCKER_(7, constness, Method).RegisterOwner(this);                                      \
        return GMOCK_NOEXCEPT_MOCKER_(7, constness, Method)                                                    \
            .With(gmock_a1, gmock_a2, gmock_a3, gmock_a4, gmock_a5, gmock_a6, gmock_a7);                       \
    }                                                                                                          \
    mutable ::testing::FunctionMocker<__VA_ARGS__> GMOCK_NOEXCEPT_MOCKER_(7, constness, Method)

#define GMOCK_NOEXCEPT_METHOD8_(tn, constness, ct, Method, ...)                                                \
    GMOCK_RESULT_(tn, __VA_ARGS__)                                                                             \
    ct Method(                                                                                                 \
        GMOCK_ARG_(tn, 1, __VA_ARGS__) gmock_a1,                                                               \
        GMOCK_ARG_(tn, 2, __VA_ARGS__) gmock_a2,                                                               \
        GMOCK_ARG_(tn, 3, __VA_ARGS__) gmock_a3,                                                               \
        GMOCK_ARG_(tn, 4, __VA_ARGS__) gmock_a4,                                                               \
        GMOCK_ARG_(tn, 5, __VA_ARGS__) gmock_a5,                                                               \
        GMOCK_ARG_(tn, 6, __VA_ARGS__) gmock_a6,                                                               \
        GMOCK_ARG_(tn, 7, __VA_ARGS__) gmock_a7,                                                               \
        GMOCK_ARG_(tn, 8, __VA_ARGS__) gmock_a8) constness noexcept {                                          \
        GTEST_COMPILE_ASSERT_(                                                                                 \
            (::testing::tuple_size<tn ::testing::internal::Function<__VA_ARGS__>::ArgumentTuple>::value == 8), \
            this_method_does_not_take_8_arguments);                                                            \
        GMOCK_NOEXCEPT_MOCKER_(8, constness, Method).SetOwnerAndName(this, #Method);                           \
        return GMOCK_NOEXCEPT_MOCKER_(8, constness, Method)                                                    \
            .Invoke(gmock_a1, gmock_a2, gmock_a3, gmock_a4, gmock_a5, gmock_a6, gmock_a7, gmock_a8);           \
    }                                                                                                          \
    ::testing::MockSpec<__VA_ARGS__>& gmock_##Method(                                                          \
        GMOCK_MATCHER_(tn, 1, __VA_ARGS__) gmock_a1,                                                           \
        GMOCK_MATCHER_(tn, 2, __VA_ARGS__) gmock_a2,                                                           \
        GMOCK_MATCHER_(tn, 3, __VA_ARGS__) gmock_a3,                                                           \
        GMOCK_MATCHER_(tn, 4, __VA_ARGS__) gmock_a4,                                                           \
        GMOCK_MATCHER_(tn, 5, __VA_ARGS__) gmock_a5,                                                           \
        GMOCK_MATCHER_(tn, 6, __VA_ARGS__) gmock_a6,                                                           \
        GMOCK_MATCHER_(tn, 7, __VA_ARGS__) gmock_a7,                                                           \
        GMOCK_MATCHER_(tn, 8, __VA_ARGS__) gmock_a8) constness noexcept {                                      \
        GMOCK_NOEXCEPT_MOCKER_(8, constness, Method).RegisterOwner(this);                                      \
        return GMOCK_NOEXCEPT_MOCKER_(8, constness, Method)                                                    \
            .With(gmock_a1, gmock_a2, gmock_a3, gmock_a4, gmock_a5, gmock_a6, gmock_a7, gmock_a8);             \
    }                                                                                                          \
    mutable ::testing::FunctionMocker<__VA_ARGS__> GMOCK_NOEXCEPT_MOCKER_(8, constness, Method)

#define GMOCK_NOEXCEPT_METHOD9_(tn, constness, ct, Method, ...)                                                \
    GMOCK_RESULT_(tn, __VA_ARGS__)                                                                             \
    ct Method(                                                                                                 \
        GMOCK_ARG_(tn, 1, __VA_ARGS__) gmock_a1,                                                               \
        GMOCK_ARG_(tn, 2, __VA_ARGS__) gmock_a2,                                                               \
        GMOCK_ARG_(tn, 3, __VA_ARGS__) gmock_a3,                                                               \
        GMOCK_ARG_(tn, 4, __VA_ARGS__) gmock_a4,                                                               \
        GMOCK_ARG_(tn, 5, __VA_ARGS__) gmock_a5,                                                               \
        GMOCK_ARG_(tn, 6, __VA_ARGS__) gmock_a6,                                                               \
        GMOCK_ARG_(tn, 7, __VA_ARGS__) gmock_a7,                                                               \
        GMOCK_ARG_(tn, 8, __VA_ARGS__) gmock_a8,                                                               \
        GMOCK_ARG_(tn, 9, __VA_ARGS__) gmock_a9) constness noexcept {                                          \
        GTEST_COMPILE_ASSERT_(                                                                                 \
            (::testing::tuple_size<tn ::testing::internal::Function<__VA_ARGS__>::ArgumentTuple>::value == 9), \
            this_method_does_not_take_9_arguments);                                                            \
        GMOCK_NOEXCEPT_MOCKER_(9, constness, Method).SetOwnerAndName(this, #Method);                           \
        return GMOCK_NOEXCEPT_MOCKER_(9, constness, Method)                                                    \
            .Invoke(gmock_a1, gmock_a2, gmock_a3, gmock_a4, gmock_a5, gmock_a6, gmock_a7, gmock_a8, gmock_a9); \
    }                                                                                                          \
    ::testing::MockSpec<__VA_ARGS__>& gmock_##Method(                                                          \
        GMOCK_MATCHER_(tn, 1, __VA_ARGS__) gmock_a1,                                                           \
        GMOCK_MATCHER_(tn, 2, __VA_ARGS__) gmock_a2,                                                           \
        GMOCK_MATCHER_(tn, 3, __VA_ARGS__) gmock_a3,                                                           \
        GMOCK_MATCHER_(tn, 4, __VA_ARGS__) gmock_a4,                                                           \
        GMOCK_MATCHER_(tn, 5, __VA_ARGS__) gmock_a5,                                                           \
        GMOCK_MATCHER_(tn, 6, __VA_ARGS__) gmock_a6,                                                           \
        GMOCK_MATCHER_(tn, 7, __VA_ARGS__) gmock_a7,                                                           \
        GMOCK_MATCHER_(tn, 8, __VA_ARGS__) gmock_a8,                                                           \
        GMOCK_MATCHER_(tn, 9, __VA_ARGS__) gmock_a9) constness noexcept {                                      \
        GMOCK_NOEXCEPT_MOCKER_(9, constness, Method).RegisterOwner(this);                                      \
        return GMOCK_NOEXCEPT_MOCKER_(9, constness, Method)                                                    \
            .With(gmock_a1, gmock_a2, gmock_a3, gmock_a4, gmock_a5, gmock_a6, gmock_a7, gmock_a8, gmock_a9);   \
    }                                                                                                          \
    mutable ::testing::FunctionMocker<__VA_ARGS__> GMOCK_NOEXCEPT_MOCKER_(9, constness, Method)

#define GMOCK_NOEXCEPT_METHOD10_(tn, constness, ct, Method, ...)                                                      \
    GMOCK_RESULT_(tn, __VA_ARGS__)                                                                                    \
    ct Method(                                                                                                        \
        GMOCK_ARG_(tn, 1, __VA_ARGS__) gmock_a1,                                                                      \
        GMOCK_ARG_(tn, 2, __VA_ARGS__) gmock_a2,                                                                      \
        GMOCK_ARG_(tn, 3, __VA_ARGS__) gmock_a3,                                                                      \
        GMOCK_ARG_(tn, 4, __VA_ARGS__) gmock_a4,                                                                      \
        GMOCK_ARG_(tn, 5, __VA_ARGS__) gmock_a5,                                                                      \
        GMOCK_ARG_(tn, 6, __VA_ARGS__) gmock_a6,                                                                      \
        GMOCK_ARG_(tn, 7, __VA_ARGS__) gmock_a7,                                                                      \
        GMOCK_ARG_(tn, 8, __VA_ARGS__) gmock_a8,                                                                      \
        GMOCK_ARG_(tn, 9, __VA_ARGS__) gmock_a9,                                                                      \
        GMOCK_ARG_(tn, 10, __VA_ARGS__) gmock_a10) constness noexcept {                                               \
        GTEST_COMPILE_ASSERT_(                                                                                        \
            (::testing::tuple_size<tn ::testing::internal::Function<__VA_ARGS__>::ArgumentTuple>::value == 10),       \
            this_method_does_not_take_10_arguments);                                                                  \
        GMOCK_NOEXCEPT_MOCKER_(10, constness, Method).SetOwnerAndName(this, #Method);                                 \
        return GMOCK_NOEXCEPT_MOCKER_(10, constness, Method)                                                          \
            .Invoke(                                                                                                  \
                gmock_a1, gmock_a2, gmock_a3, gmock_a4, gmock_a5, gmock_a6, gmock_a7, gmock_a8, gmock_a9, gmock_a10); \
    }                                                                                                                 \
    ::testing::MockSpec<__VA_ARGS__>& gmock_##Method(                                                                 \
        GMOCK_MATCHER_(tn, 1, __VA_ARGS__) gmock_a1,                                                                  \
        GMOCK_MATCHER_(tn, 2, __VA_ARGS__) gmock_a2,                                                                  \
        GMOCK_MATCHER_(tn, 3, __VA_ARGS__) gmock_a3,                                                                  \
        GMOCK_MATCHER_(tn, 4, __VA_ARGS__) gmock_a4,                                                                  \
        GMOCK_MATCHER_(tn, 5, __VA_ARGS__) gmock_a5,                                                                  \
        GMOCK_MATCHER_(tn, 6, __VA_ARGS__) gmock_a6,                                                                  \
        GMOCK_MATCHER_(tn, 7, __VA_ARGS__) gmock_a7,                                                                  \
        GMOCK_MATCHER_(tn, 8, __VA_ARGS__) gmock_a8,                                                                  \
        GMOCK_MATCHER_(tn, 9, __VA_ARGS__) gmock_a9,                                                                  \
        GMOCK_MATCHER_(tn, 10, __VA_ARGS__) gmock_a10) constness noexcept {                                           \
        GMOCK_NOEXCEPT_MOCKER_(10, constness, Method).RegisterOwner(this);                                            \
        return GMOCK_NOEXCEPT_MOCKER_(10, constness, Method)                                                          \
            .With(                                                                                                    \
                gmock_a1, gmock_a2, gmock_a3, gmock_a4, gmock_a5, gmock_a6, gmock_a7, gmock_a8, gmock_a9, gmock_a10); \
    }                                                                                                                 \
    mutable ::testing::FunctionMocker<__VA_ARGS__> GMOCK_NOEXCEPT_MOCKER_(10, constness, Method)
/// @}

/**
 * @{
 * @brief Macro to mock method with @a noexcept specifier.
 */
#define MOCK_NOEXCEPT_METHOD0(m, ...) GMOCK_NOEXCEPT_METHOD0_(, , , m, __VA_ARGS__)
#define MOCK_NOEXCEPT_METHOD1(m, ...) GMOCK_NOEXCEPT_METHOD1_(, , , m, __VA_ARGS__)
#define MOCK_NOEXCEPT_METHOD2(m, ...) GMOCK_NOEXCEPT_METHOD2_(, , , m, __VA_ARGS__)
#define MOCK_NOEXCEPT_METHOD3(m, ...) GMOCK_NOEXCEPT_METHOD3_(, , , m, __VA_ARGS__)
#define MOCK_NOEXCEPT_METHOD4(m, ...) GMOCK_NOEXCEPT_METHOD4_(, , , m, __VA_ARGS__)
#define MOCK_NOEXCEPT_METHOD5(m, ...) GMOCK_NOEXCEPT_METHOD5_(, , , m, __VA_ARGS__)
#define MOCK_NOEXCEPT_METHOD6(m, ...) GMOCK_NOEXCEPT_METHOD6_(, , , m, __VA_ARGS__)
#define MOCK_NOEXCEPT_METHOD7(m, ...) GMOCK_NOEXCEPT_METHOD7_(, , , m, __VA_ARGS__)
#define MOCK_NOEXCEPT_METHOD8(m, ...) GMOCK_NOEXCEPT_METHOD8_(, , , m, __VA_ARGS__)
#define MOCK_NOEXCEPT_METHOD9(m, ...) GMOCK_NOEXCEPT_METHOD9_(, , , m, __VA_ARGS__)
#define MOCK_NOEXCEPT_METHOD10(m, ...) GMOCK_NOEXCEPT_METHOD10_(, , , m, __VA_ARGS__)
/// @}

/**
 * @{
 * @brief Macro to mock method with @a const @a noexcept specifier.
 */
#define MOCK_CONST_NOEXCEPT_METHOD0(m, ...) GMOCK_NOEXCEPT_METHOD0_(, const, , m, __VA_ARGS__)
#define MOCK_CONST_NOEXCEPT_METHOD1(m, ...) GMOCK_NOEXCEPT_METHOD1_(, const, , m, __VA_ARGS__)
#define MOCK_CONST_NOEXCEPT_METHOD2(m, ...) GMOCK_NOEXCEPT_METHOD2_(, const, , m, __VA_ARGS__)
#define MOCK_CONST_NOEXCEPT_METHOD3(m, ...) GMOCK_NOEXCEPT_METHOD3_(, const, , m, __VA_ARGS__)
#define MOCK_CONST_NOEXCEPT_METHOD4(m, ...) GMOCK_NOEXCEPT_METHOD4_(, const, , m, __VA_ARGS__)
#define MOCK_CONST_NOEXCEPT_METHOD5(m, ...) GMOCK_NOEXCEPT_METHOD5_(, const, , m, __VA_ARGS__)
#define MOCK_CONST_NOEXCEPT_METHOD6(m, ...) GMOCK_NOEXCEPT_METHOD6_(, const, , m, __VA_ARGS__)
#define MOCK_CONST_NOEXCEPT_METHOD7(m, ...) GMOCK_NOEXCEPT_METHOD7_(, const, , m, __VA_ARGS__)
#define MOCK_CONST_NOEXCEPT_METHOD8(m, ...) GMOCK_NOEXCEPT_METHOD8_(, const, , m, __VA_ARGS__)
#define MOCK_CONST_NOEXCEPT_METHOD9(m, ...) GMOCK_NOEXCEPT_METHOD9_(, const, , m, __VA_ARGS__)
#define MOCK_CONST_NOEXCEPT_METHOD10(m, ...) GMOCK_NOEXCEPT_METHOD10_(, const, , m, __VA_ARGS__)
/// @}

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_ACSDK_TEST_GMOCKEXTENSIONS_H_
