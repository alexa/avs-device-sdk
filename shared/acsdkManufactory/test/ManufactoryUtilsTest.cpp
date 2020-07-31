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

/// @file ManufactoryUtilsTest.cpp

/**
 * Test of acsdkManufactory's compile time utilities.
 */

#include <iostream>
#include <memory>

#include "acsdkManufactory/Import.h"
#include "acsdkManufactory/internal/Utils.h"

namespace alexaClientSDK {
namespace acsdkManufactory {
namespace internal {

/**
 * Template used to create distinct classes with which to test compile time utilities.
 *
 * @tparam x An integer used to distinguish between instantiations of this template.
 */
template <int x>
class Foo {
public:
};

/**
 * Function used to invoke compile time utilities.
 */
void manufactoryUtilsTest() {
    // Test ContainsType

    static_assert(!ContainsType<std::tuple<>, Foo<1>>::value, "test");
    static_assert(ContainsType<std::tuple<Foo<1>>, Foo<1>>::value, "test");
    static_assert(!ContainsType<std::tuple<Foo<2>>, Foo<1>>::value, "test");
    static_assert(ContainsType<std::tuple<Foo<1>, Foo<2>>, Foo<1>>::value, "test");
    static_assert(!ContainsType<std::tuple<Foo<1>, Foo<2>>, Foo<3>>::value, "test");
    static_assert(ContainsType<std::tuple<Foo<1>, Foo<1>>, Foo<1>>::value, "test");
    static_assert(!ContainsType<std::tuple<Foo<2>, Foo<2>>, Foo<1>>::value, "test");
    static_assert(ContainsType<std::tuple<Foo<1>, Foo<2>, Foo<3>>, Foo<3>>::value, "test");
    static_assert(!ContainsType<std::tuple<Foo<1>, Foo<2>, Foo<3>>, Foo<4>>::value, "test");

    // Test ContainsTypes

    static_assert(!ContainsTypes<std::tuple<>, Foo<1>>::value, "test");
    static_assert(ContainsTypes<std::tuple<Foo<1>>, Foo<1>>::value, "test");
    static_assert(!ContainsTypes<std::tuple<Foo<1>>, Foo<2>>::value, "test");
    static_assert(!ContainsTypes<std::tuple<Foo<1>>, Foo<1>, Foo<2>>::value, "test");
    static_assert(ContainsTypes<std::tuple<Foo<1>, Foo<2>>, Foo<1>>::value, "test");
    static_assert(ContainsTypes<std::tuple<Foo<1>, Foo<2>>, Foo<1>, Foo<1>>::value, "test");
    static_assert(ContainsTypes<std::tuple<Foo<1>, Foo<2>>, Foo<2>, Foo<2>>::value, "test");
    static_assert(ContainsTypes<std::tuple<Foo<1>, Foo<2>>, Foo<1>, Foo<2>>::value, "test");
    static_assert(ContainsTypes<std::tuple<Foo<1>, Foo<1>>, Foo<1>>::value, "test");
    static_assert(!ContainsTypes<std::tuple<Foo<1>, Foo<1>>, Foo<2>>::value, "test");
    static_assert(ContainsTypes<std::tuple<Foo<1>, Foo<2>, Foo<3>>, Foo<1>>::value, "test");
    static_assert(!ContainsTypes<std::tuple<Foo<1>, Foo<2>, Foo<3>>, Foo<4>>::value, "test");

    // Test ContainsTupleTypes

    static_assert(ContainsTupleTypes<std::tuple<>, std::tuple<>>::value, "test");
    static_assert(!ContainsTupleTypes<std::tuple<>, std::tuple<Foo<1>>>::value, "test");
    static_assert(ContainsTupleTypes<std::tuple<Foo<1>, Foo<1>>, std::tuple<>>::value, "test");
    static_assert(ContainsTupleTypes<std::tuple<Foo<1>, Foo<1>>, std::tuple<Foo<1>>>::value, "test");
    static_assert(!ContainsTupleTypes<std::tuple<Foo<1>, Foo<1>>, std::tuple<Foo<2>>>::value, "test");
    static_assert(ContainsTupleTypes<std::tuple<Foo<1>, Foo<2>>, std::tuple<>>::value, "test");
    static_assert(ContainsTupleTypes<std::tuple<Foo<1>, Foo<2>>, std::tuple<Foo<1>>>::value, "test");
    static_assert(ContainsTupleTypes<std::tuple<Foo<1>, Foo<2>>, std::tuple<Foo<2>>>::value, "test");
    static_assert(ContainsTupleTypes<std::tuple<Foo<1>, Foo<2>>, std::tuple<Foo<1>, Foo<2>>>::value, "test");
    static_assert(!ContainsTupleTypes<std::tuple<Foo<1>, Foo<2>>, std::tuple<Foo<2>, Foo<3>>>::value, "test");
    static_assert(!ContainsTupleTypes<std::tuple<Foo<1>, Foo<2>>, std::tuple<Foo<3>, Foo<2>>>::value, "test");

    // Test HasImport

    static_assert(!HasImport<>::value, "test");
    static_assert(!HasImport<Foo<1>>::value, "test");
    static_assert(HasImport<Import<Foo<1>>>::value, "test");
    static_assert(!HasImport<Foo<1>, Foo<2>>::value, "test");
    static_assert(HasImport<Import<Foo<1>>, Foo<2>>::value, "test");
    static_assert(HasImport<Import<Foo<1>>, Import<Foo<2>>>::value, "test");
    static_assert(HasImport<Foo<1>, Import<Foo<2>>>::value, "test");
    static_assert(!HasImport<Foo<1>, Foo<2>, Foo<3>>::value, "test");
    static_assert(HasImport<Import<Foo<1>>, Foo<2>, Foo<3>>::value, "test");
    static_assert(HasImport<Import<Foo<1>>, Import<Foo<2>, Foo<3>>>::value, "test");
    static_assert(HasImport<Foo<1>, Import<Foo<2>>, Foo<3>>::value, "test");
    static_assert(!HasImport<Foo<1>, Foo<1>>::value, "test");
    static_assert(HasImport<Import<Foo<1>>, Import<Foo<1>>>::value, "test");

    // Test DedupTypes

    DedupTypes<>::type x1 = {};
    DedupTypes<Foo<1>>::type x2((Foo<1>()));
    DedupTypes<Foo<1>, Foo<1>>::type x3((Foo<1>()));
    DedupTypes<Foo<1>, Foo<2>>::type x4((Foo<1>()), (Foo<2>()));
    DedupTypes<Foo<1>, Foo<2>, Foo<2>>::type x5((Foo<1>()), (Foo<2>()));
    DedupTypes<Foo<1>, Foo<2>, Foo<1>, Foo<2>>::type x6((Foo<1>()), (Foo<2>()));
    DedupTypes<Foo<1>, Foo<2>, Foo<2>, Foo<1>>::type x7((Foo<1>()), (Foo<2>()));
    DedupTypes<Foo<1>, Foo<2>, Foo<3>, Foo<2>, Foo<1>>::type x8((Foo<1>()), (Foo<2>()), (Foo<3>()));
    DedupTypes<Foo<1>, Foo<2>, Foo<3>, Foo<2>, Foo<3>, Foo<1>>::type x9((Foo<1>()), (Foo<2>()), (Foo<3>()));
    DedupTypes<Foo<1>, Foo<2>, Foo<3>, Foo<2>, Foo<2>, Foo<2>>::type x10((Foo<1>()), (Foo<2>()), (Foo<3>()));
    // Avoid annoying "unused variable" warnings.
    std::cout << &x1 << &x2 << &x3 << &x4 << &x5 << &x6 << &x7 << &x8 << &x9 << &x10;

    // Test RemoveTypes

    RemoveTypes<std::tuple<>, std::tuple<>>::type x11 = {};
    RemoveTypes<std::tuple<Foo<1>>, std::tuple<>>::type x12((Foo<1>()));
    RemoveTypes<std::tuple<Foo<1>>, std::tuple<Foo<1>>>::type x13 = {};
    RemoveTypes<std::tuple<Foo<1>, Foo<2>>, std::tuple<>>::type x14((Foo<1>()), (Foo<2>()));
    RemoveTypes<std::tuple<Foo<1>, Foo<2>>, std::tuple<Foo<1>>>::type x15((Foo<2>()));
    RemoveTypes<std::tuple<Foo<1>, Foo<2>>, std::tuple<Foo<2>>>::type x16((Foo<1>()));
    RemoveTypes<std::tuple<Foo<1>, Foo<1>>, std::tuple<Foo<1>>>::type x17 = {};
    RemoveTypes<std::tuple<Foo<1>, Foo<1>>, std::tuple<Foo<1>, Foo<1>>>::type x18 = {};
    RemoveTypes<std::tuple<Foo<1>, Foo<1>>, std::tuple<Foo<2>>>::type x19((Foo<1>()), (Foo<1>()));
    RemoveTypes<std::tuple<Foo<1>, Foo<2>, Foo<3>>, std::tuple<Foo<4>, Foo<3>, Foo<2>>>::type x20((Foo<1>()));
    // Avoid annoying "unused variable" warnings.
    std::cout << &x11 << &x12 << &x13 << &x14 << &x15 << &x16 << &x17 << &x18 << &x19 << &x20;

    // Test GetImportsAndExports

    GetImportsAndExports<>::type::exports x21 = {};
    GetImportsAndExports<>::type::imports x22 = {};
    GetImportsAndExports<Foo<1>>::type::exports x23((Foo<1>()));
    GetImportsAndExports<Foo<1>>::type::imports x24 = {};
    GetImportsAndExports<Foo<1>, Foo<1>>::type::exports x25((Foo<1>()));
    GetImportsAndExports<Foo<1>, Foo<1>>::type::imports x26 = {};
    GetImportsAndExports<Foo<1>, Foo<2>>::type::exports x27((Foo<1>()), (Foo<2>()));
    GetImportsAndExports<Foo<1>, Foo<2>>::type::imports x28 = {};
    GetImportsAndExports<Foo<1>, Foo<2>, Foo<3>>::type::exports x29((Foo<1>()), (Foo<2>()), (Foo<3>()));
    GetImportsAndExports<Foo<1>, Foo<2>, Foo<3>>::type::imports x30 = {};
    GetImportsAndExports<Import<Foo<1>>, Foo<2>, Foo<3>>::type::exports x31((Foo<2>()), (Foo<3>()));
    GetImportsAndExports<Import<Foo<1>>, Foo<2>, Foo<3>>::type::imports x32((Foo<1>()));
    GetImportsAndExports<Import<Foo<1>>, Import<Foo<2>>, Foo<3>>::type::exports x33((Foo<3>()));
    GetImportsAndExports<Import<Foo<1>>, Import<Foo<2>>, Foo<3>>::type::imports x34((Foo<1>()), (Foo<2>()));
    GetImportsAndExports<Import<Foo<1>>, Import<Foo<2>>, Import<Foo<3>>>::type::exports x35 = {};
    GetImportsAndExports<Import<Foo<1>>, Import<Foo<2>>, Import<Foo<3>>>::type::imports x36(
        (Foo<1>()), (Foo<2>()), (Foo<3>()));
    GetImportsAndExports<Foo<1>, Import<Foo<2>>, Import<Foo<3>>>::type::exports x37((Foo<1>()));
    GetImportsAndExports<Foo<1>, Import<Foo<2>>, Import<Foo<3>>>::type::imports x38((Foo<2>()), (Foo<3>()));
    // Avoid annoying "unused variable" warnings.
    std::cout << &x21 << &x22 << &x23 << &x24 << &x25 << &x26 << &x27 << &x28 << &x29 << &x30;
    std::cout << &x31 << &x32 << &x33 << &x34 << &x35 << &x36 << &x37 << &x38;

    // Test that large numbers of template parameters don't trigger degenerate compiler behavior.
    GetImportsAndExports<
        Import<Foo<1>>,
        Foo<2>,
        std::shared_ptr<Foo<3>>,
        std::shared_ptr<Foo<4>>,
        Import<Foo<5>>,
        Import<Foo<6>>,
        Import<Foo<7>>,
        Foo<8>,
        Import<std::shared_ptr<Foo<9>>>,
        Foo<10>,
        Import<Foo<11>>,
        Foo<12>,
        Import<Foo<13>>,
        Import<Foo<14>>,
        Foo<15>,
        Foo<16>,
        Foo<17>,
        Import<Foo<18>>,
        Import<Foo<19>>,
        Import<Foo<20>>,
        Foo<21>,
        Import<Foo<22>>,
        Foo<23>,
        Import<Foo<24>>,
        Import<Foo<25>>,
        Import<std::shared_ptr<Foo<26>>>,
        Foo<27>,
        Foo<28>,
        Import<std::shared_ptr<Foo<29>>>,
        Import<std::shared_ptr<Foo<30>>>,
        Import<Foo<31>>,
        Import<Foo<32>>,
        Import<Foo<33>>,
        Foo<34>,
        std::shared_ptr<Foo<35>>,
        Import<std::shared_ptr<Foo<36>>>,
        Foo<37>,
        std::shared_ptr<Foo<38>>,
        std::shared_ptr<Foo<39>>,
        std::shared_ptr<Foo<40>>,
        Foo<41>,
        Foo<42>,
        Foo<43>,
        Foo<44>,
        Foo<45>,
        Foo<46>,
        Foo<47>,
        Foo<48>,
        Foo<49>,
        Foo<50>,
        Foo<51>,
        Foo<52>,
        Foo<53>,
        Foo<54>,
        Foo<55>,
        Foo<56>,
        Foo<57>,
        Foo<58>,
        Foo<59>,
        Foo<60>,
        Foo<61>,
        Foo<62>,
        Foo<63>,
        Foo<64>,
        Foo<65>,
        Foo<66>,
        Foo<67>,
        Foo<68>,
        Foo<69>,
        Foo<70>,
        Foo<71>,
        Foo<72>,
        Foo<73>,
        Foo<74>,
        Foo<75>,
        Foo<76>,
        Foo<77>,
        Foo<78>,
        Foo<79>,
        Foo<80>,
        Foo<81>,
        Foo<82>,
        Foo<83>,
        Foo<84>,
        Foo<85>,
        Foo<86>,
        Foo<87>,
        Foo<88>,
        Foo<89>,
        Foo<90>,
        Foo<91>,
        Foo<92>,
        Foo<93>,
        Foo<94>,
        Foo<85>,
        Foo<96>,
        Foo<97>,
        Foo<98>,
        Foo<99>,
        Foo<100>>::type::exports x39;
    // Avoid annoying "unused variable" warnings.
    std::cerr << &x39;
};

}  // namespace internal
}  // namespace acsdkManufactory
}  // namespace alexaClientSDK
