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

/// @file ManufactoryTest.cpp

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "acsdkManufactory/Component.h"
#include "acsdkManufactory/ComponentAccumulator.h"
#include "acsdkManufactory/Manufactory.h"

namespace alexaClientSDK {
namespace acsdkManufactory {
namespace test {

using namespace ::testing;
using namespace acsdkManufactory;

class ManufactoryTest : public ::testing::Test {
public:
};

/**
 * Interface for setting a string.
 */
class InterfaceA {
public:
    /**
     * Destructor.
     */
    virtual ~InterfaceA() = default;

    /**
     * Set the string value.
     *
     * @param input The new value for the string.
     */
    virtual void setString(const std::string& input) = 0;
};

/**
 * Interface for accessing an object's ID and string value.
 */
class InterfaceB {
public:
    /**
     * Destructor
     */
    virtual ~InterfaceB() = default;

    /**
     * Get the object's ID.
     *
     * @return The object's ID.
     */
    virtual int getId() const = 0;

    /**
     * Get the string value of the object.
     *
     * @return The string value of the object.
     */
    virtual std::string getString() const = 0;
};

/**
 * Interface combining InterfaceA and InterfaceB.
 */
class InterfaceAB
        : public InterfaceA
        , public InterfaceB {};

/**
 * Class implementing InterfaceAB.
 */
class AB : public InterfaceAB {
public:
    /**
     * Constructor.  Provides each instance of AB with a unique ID.
     */
    AB() : m_id(m_nextId++) {
    }

    int getId() const override {
        return m_id;
    }

    void setString(const std::string& input) override {
        m_state = input;
    };

    std::string getString() const override {
        return m_state;
    };

private:
    /// The ID of the next instance of AB.
    static int m_nextId;

    /// The ID of this instance of AB.
    const int m_id;

    /// The string value of this instance of AB.
    std::string m_state;
};

int AB::m_nextId = 1;

/// Type used to annotate (i.e. distinguish instances of) another type.
struct Annotation1 {};

/// Type used to annotate (i.e. distinguish instances of) another type.
struct Annotation2 {};

/// Type used to annotate (i.e. distinguish instances of) another type.
struct Annotation3 {};

/// Type used to annotate (i.e. distinguish instances of) another type.
struct Annotation4 {};

/**
 * Template class for defining simple derivations of AB.
 * @tparam X Number used to differentiate derived types.
 */
template <int X>
class ABSubclass : public AB {};

// ----- test_manufactureUnique ----- //

/**
 * Factory for creating a new instance of InterfaceA and returning it via std::unique_ptr<InterfaceA>.
 *
 * @return A new instance of InterfaceA.
 */
std::unique_ptr<InterfaceA> createUniqueA() {
    return std::unique_ptr<InterfaceA>(new AB());
}

/**
 * Test manufacture of an instances returned via std::unique_ptr.  Verify that multiple instances of the same
 * interface are indeed distinct instances.
 */
TEST_F(ManufactoryTest, test_manufactureUnique) {
    Component<std::unique_ptr<InterfaceA>> component(ComponentAccumulator<>().addUniqueFactory(createUniqueA));
    auto manufactory = Manufactory<std::unique_ptr<InterfaceA>>::create(component);
    ASSERT_TRUE(manufactory);

    auto a1 = manufactory->get<std::unique_ptr<InterfaceA>>();
    ASSERT_TRUE(a1);
    auto a2 = manufactory->get<std::unique_ptr<InterfaceA>>();
    ASSERT_TRUE(a2);
    ASSERT_NE(a1, a2);
}

// ----- test_manufactureShared ----- //

/**
 * Factory for creating a new instance of InterfaceAB and returning it via std::shared_ptr<InterfaceAB>.
 *
 * @return A new instance of InterfaceAB.
 */
std::shared_ptr<InterfaceAB> createSharedAB() {
    return std::make_shared<AB>();
}

/**
 * Test manufacture of an 'unloadable' instance returned via std::shared_ptr.  Verify that while an instance
 * is still referenced, manufacture of the same interface will provide the same instance.  Also verify that
 * when all references have been deleted, the next manufacture will create a new instance.
 */
TEST_F(ManufactoryTest, test_manufactureShared) {
    Component<std::shared_ptr<InterfaceAB>> component(ComponentAccumulator<>().addUnloadableFactory(createSharedAB));
    auto manufactory = Manufactory<std::shared_ptr<InterfaceAB>>::create(component);
    ASSERT_TRUE(manufactory);

    // Verify created instances are shared...

    auto ab1 = manufactory->get<std::shared_ptr<InterfaceAB>>();
    ASSERT_TRUE(ab1);
    ASSERT_TRUE(ab1->getString().empty());
    auto ab2 = manufactory->get<std::shared_ptr<InterfaceAB>>();
    ASSERT_TRUE(ab2);
    ASSERT_EQ(ab1, ab2);
    ab1->setString("something");

    // ...and that shared instances are released when references outside the manufactory are reset.

    ab1.reset();
    ab2.reset();
    auto ab3 = manufactory->get<std::shared_ptr<InterfaceAB>>();
    ASSERT_TRUE(ab3);
    ASSERT_TRUE(ab3->getString().empty());
}

// ----- test_multipleInhertence ----- //

/**
 * Factory for InterfaceA that depends upon an instance of InterfaceAB, and which returns an InterfaceA
 * pointer the the provided instance of InterfaceAB.
 *
 * @param ab The instance of InterfaceAB to use to create an InterfaceA.
 * @return The InterfaceA portion of the InterfaceAB that was provided.
 */
std::shared_ptr<InterfaceA> createAFromAB(const std::shared_ptr<InterfaceAB>& ab) {
    return ab;
}

/**
 * Factory for InterfaceB that depends upon an instance of InterfaceAB, and which returns an InterfaceB
 * pointer the the provided instance of InterfaceAB.
 *
 * @param ab The instance of InterfaceAB to use to create an InterfaceB.
 * @return The InterfaceB portion of the InterfaceAB that was provided.
 */
std::shared_ptr<InterfaceB> createBFromAB(const std::shared_ptr<InterfaceAB>& ab) {
    return ab;
}

/**
 * Factory used to create an instance of InterfaceAB.
 *
 * @return An instance of InterfaceAB.
 */
std::shared_ptr<InterfaceAB> createAB() {
    return std::make_shared<AB>();
}

/**
 * Definition of a component that exports implementations of InterfaceA and InterfaceB.
 * @return
 */
Component<std::shared_ptr<InterfaceA>, std::shared_ptr<InterfaceB>> getABComponent() {
    ComponentAccumulator<> accumulator;
    return accumulator.addUnloadableFactory(createAFromAB)
        .addUnloadableFactory(createBFromAB)
        .addUnloadableFactory(createAB);
}

/**
 * Verify that a single object can be manufactured as multiple interfaces.
 */
TEST_F(ManufactoryTest, test_multipleInhertance) {
    auto component = getABComponent();
    auto manufactory = Manufactory<std::shared_ptr<InterfaceA>, std::shared_ptr<InterfaceB>>::create(component);
    ASSERT_TRUE(manufactory);

    auto a = manufactory->get<std::shared_ptr<InterfaceA>>();
    ASSERT_TRUE(a);
    auto b = manufactory->get<std::shared_ptr<InterfaceB>>();
    ASSERT_TRUE(b);

    // Make sure b's string is empty.
    ASSERT_TRUE(b->getString().empty());

    // Set a's string value.
    a->setString("something");

    // Verify that setting a's string sets b's string value.
    ASSERT_FALSE(b->getString().empty());
}

// ------ test_annotatedManufacture ----- //

/**
 * Factory used to create an annotated (with Annotation1) instance of InterfaceAB that depends upon
 * the instance of InterfaceAB annotated with Annotation3.
 *
 * @param ab3 A different instance of InterfaceAB (annotated with Annotation3)
 * @return The instance of InterfaceAB annotated with Annotation1.
 */
Annotated<Annotation1, InterfaceAB> createAB1(const Annotated<Annotation3, InterfaceAB>& ab3) {
    if (!ab3) {
        EXPECT_TRUE(ab3);
        return nullptr;
    }
    EXPECT_EQ(ab3->getString(), "3");
    auto result = createAB();
    result->setString("1");
    return result;
}

/**
 * Factory used to create an annotated (with Annotation2) instance of InterfaceAB.
 *
 * @return The instance of InterfaceAB annotated with Annotation2.
 */
Annotated<Annotation2, InterfaceAB> createAB2() {
    auto result = createAB();
    result->setString("2");
    return result;
}

/**
 * Factory used to create an annotated (with Annotation3) instance of InterfaceAB that depends upon
 * the instance of InterfaceAB annotated with Annotation2.
 *
 * @param ab2 A different instance of InterfaceAB (annotated with Annotation2)
 * @return The instance of InterfaceAB annotated with Annotation3.
 */
Annotated<Annotation3, InterfaceAB> createAB3(const Annotated<Annotation2, InterfaceAB>& ab2) {
    if (!ab2) {
        EXPECT_TRUE(ab2);
        return nullptr;
    }
    EXPECT_EQ(ab2->getString(), "2");
    auto result = createAB();
    result->setString("3");
    return result;
}

/**
 * Definition of a component that exports instances of InterfaceAB annotated with Annotation1 and Annotation2,
 * and which imports an instance of InterfaceAB annotated with Annotation3.
 *
 * @return A component that exports instances of InterfaceAB annotated with Annotation1 and Annotation2...
 */
Component<
    Annotated<Annotation1, InterfaceAB>,
    Annotated<Annotation2, InterfaceAB>,
    Import<Annotated<Annotation3, InterfaceAB>>>
getComponent12() {
    ComponentAccumulator<> accumulator;
    return accumulator.addUnloadableFactory(createAB1).addUnloadableFactory(createAB2);
}

/**
 * Definition of a component that exports the instance of InterfaceAB annotated with Annotation3.
 *
 * @return A component that exports the instance of InterfaceAB annotated with Annotation3
 */
Component<Annotated<Annotation3, InterfaceAB>> getComponent3() {
    ComponentAccumulator<> accumulator;
    return accumulator.addComponent(getComponent12()).addUnloadableFactory(createAB3);
}

/**
 * Verify that annotations can be used to distinguish specific instances of objects.
 * Also verify that imports between components are properly resolved.
 */
TEST_F(ManufactoryTest, test_annotatedManufacture) {
    auto component = getComponent3();
    auto manufactory = Manufactory<Annotated<Annotation3, InterfaceAB>>::create(component);
    ASSERT_TRUE(manufactory);

    auto ab3 = manufactory->get<Annotated<Annotation3, InterfaceAB>>();
    ASSERT_TRUE(ab3);
    ASSERT_EQ(ab3->getString(), "3");
    auto anotherAb3 = manufactory->get<Annotated<Annotation3, InterfaceAB>>();
    ASSERT_EQ(ab3, anotherAb3);
}

// ----- test_retainedManufacture ----- //

/**
 * Definition of a Component that exports 'retained' (not unloadable) instances of InterfaceAB.
 *
 * @return A Component that exports 'retained' (not unloadable) instances of InterfaceAB.
 */
Component<std::shared_ptr<InterfaceAB>> getRetainedABComponent() {
    ComponentAccumulator<> accumulator;
    return accumulator.addRetainedFactory(createAB);
}

/**
 * Verify that manufacture of retained interfaces provides a single instance fore the life of the manufactory.
 */
TEST_F(ManufactoryTest, test_retainedManufacture) {
    auto component = getRetainedABComponent();

    // Verify that retained instances are retained across manufactures.

    int id1 = 0;
    {
        auto manufactory = Manufactory<std::shared_ptr<InterfaceAB>>::create(component);
        ASSERT_TRUE(manufactory);

        auto ab1 = manufactory->get<std::shared_ptr<InterfaceAB>>();
        ASSERT_TRUE(ab1);
        id1 = ab1->getId();
        auto ab2 = manufactory->get<std::shared_ptr<InterfaceAB>>();
        ASSERT_TRUE(ab2);
        auto id2 = ab2->getId();
        ASSERT_EQ(ab1, ab2);
        ASSERT_EQ(id1, id2);
    }

    // Verify that even retained instances are retained even when all references (outside of the manufactory)
    // are released.

    {
        auto manufactory = Manufactory<std::shared_ptr<InterfaceAB>>::create(component);
        ASSERT_TRUE(manufactory);

        auto ab3 = manufactory->get<std::shared_ptr<InterfaceAB>>();
        ASSERT_TRUE(ab3);
        auto id3 = ab3->getId();
        ASSERT_NE(id1, id3);
    }
};

// ----- test_requiredManufacture ----- //

/**
 * Factory for an instance of InterfaceAB annotated with Anotation1.
 *
 * @return An instance of InterfaceAB annotated with Anotation1
 */
Annotated<Annotation1, InterfaceAB> createRetainedAB1() {
    return createAB();
}

/**
 * Factory for an instance of InterfaceAB annotated with Anotation2, which is dependent upon an instance of
 * InterfaceAB annotated with Annotation1.
 *
 * @param ab1 An instance of InterfaceAB annotated with Annotation1.
 * @return An instance of InterfaceAB annotated with Annotation2.
 */
Annotated<Annotation2, InterfaceAB> createRequiredAB2(const Annotated<Annotation1, InterfaceAB>& ab1) {
    if (!ab1) {
        EXPECT_TRUE(ab1);
        return nullptr;
    }
    ab1->setString("ab2 was here!");
    return createAB();
}

/**
 * Definition of a Component that exports a 'required' instance of InterfaceAB annotated with Annotation1.
 *
 * @return A Component that exports an instance of InterfaceAB annotated with Annotation1.
 */
Component<Annotated<Annotation1, InterfaceAB>> getRequiredComponent() {
    ComponentAccumulator<> accumulator;
    return accumulator.addRetainedFactory(createRetainedAB1).addRequiredFactory(createRequiredAB2);
};

/**
 * Verify that manufacture of a required interface results in instantiation of the interface at the time that
 * the @c Manufactory is created rather than being driven by other dependencies.
 */
TEST_F(ManufactoryTest, test_requiredManufacture) {
    auto component = getRequiredComponent();
    auto manufactory = Manufactory<Annotated<Annotation1, InterfaceAB>>::create(component);
    ASSERT_TRUE(manufactory);

    auto ab1 = manufactory->get<Annotated<Annotation1, InterfaceAB>>();
    ASSERT_TRUE(ab1);
    ASSERT_EQ(ab1->getString(), "ab2 was here!");
}

// ----- test_primeManufacture -----

/**
 * Template function to create instances of a variety of subclasses of AB.
 *
 * @tparam X A number used to distinguish subclasses.
 * @return A new instance of ABSubclass<X>.
 */
template <int X>
static std::shared_ptr<ABSubclass<X>> createABSubclass() {
    return std::make_shared<ABSubclass<X>>();
}

/**
 * Definition of a component that includes both primary and required types to verify proper ordering of their
 * instantiation.
 *
 * @return A  component that includes both primary and required types.
 */
Component<std::shared_ptr<ABSubclass<1>>, std::shared_ptr<ABSubclass<2>>, std::shared_ptr<ABSubclass<3>>>
getPrimaryTestComponent() {
    return ComponentAccumulator<>()
        .addRequiredFactory(createABSubclass<1>)
        .addPrimaryFactory(createABSubclass<2>)
        .addRequiredFactory(createABSubclass<3>);
};

/**
 * Verify that primary factories are invoked before required factories.
 */
TEST_F(ManufactoryTest, test_primeManufacture) {
    auto component = getPrimaryTestComponent();
    auto manufactory =
        Manufactory<std::shared_ptr<ABSubclass<1>>, std::shared_ptr<ABSubclass<2>>, std::shared_ptr<ABSubclass<3>>>::
            create(component);
    ASSERT_TRUE(manufactory);

    auto v1 = manufactory->get<std::shared_ptr<ABSubclass<1>>>();
    ASSERT_TRUE(v1);
    auto v3 = manufactory->get<std::shared_ptr<ABSubclass<3>>>();
    ASSERT_TRUE(v3);
    auto v2 = manufactory->get<std::shared_ptr<ABSubclass<2>>>();
    ASSERT_TRUE(v2);

    // Because ABSubclass<2> is 'primary', so it should be instantiated first.
    ASSERT_LT(v2->getId(), v1->getId());
    ASSERT_LT(v2->getId(), v3->getId());
}

// ----- test_functionManufacture -----

/**
 * Template function to create a std:functions that creates a distinct subclasses of AB.
 *
 * @tparam X A number used to distinguish subclasses.
 * @tparam Dependencies Types of arguments to the ABSubclass<X> factory
 * @return A function to create a std:functions that creates a distinct subclasses of AB.
 */
template <int X, typename... Dependencies>
std::function<std::shared_ptr<ABSubclass<X>>(Dependencies...)> getFunctionFactory() {
    return [](Dependencies... dependencies) { return std::make_shared<ABSubclass<X>>(); };
}

Component<
    std::shared_ptr<ABSubclass<1>>,
    std::shared_ptr<ABSubclass<2>>,
    std::shared_ptr<ABSubclass<3>>,
    std::shared_ptr<ABSubclass<4>>>
getFunctionTestComponent() {
    return ComponentAccumulator<>()
        .addPrimaryFactory(getFunctionFactory<1>())
        .addRequiredFactory(getFunctionFactory<2, const std::shared_ptr<ABSubclass<3>>&>())
        .addRetainedFactory(getFunctionFactory<3>())
        .addUnloadableFactory(getFunctionFactory<4>());
}

/**
 * Verify that std::function factories work and are invoked in the correct order.
 */
TEST_F(ManufactoryTest, test_functionManufacture) {
    auto component = getFunctionTestComponent();
    auto manufactory = Manufactory<
        std::shared_ptr<ABSubclass<1>>,
        std::shared_ptr<ABSubclass<2>>,
        std::shared_ptr<ABSubclass<3>>,
        std::shared_ptr<ABSubclass<4>>>::create(component);
    ASSERT_TRUE(manufactory);

    auto v2 = manufactory->get<std::shared_ptr<ABSubclass<2>>>();
    ASSERT_TRUE(v2);
    auto v3 = manufactory->get<std::shared_ptr<ABSubclass<3>>>();
    ASSERT_TRUE(v3);
    auto v4 = manufactory->get<std::shared_ptr<ABSubclass<4>>>();
    ASSERT_TRUE(v4);
    auto v1 = manufactory->get<std::shared_ptr<ABSubclass<1>>>();
    ASSERT_TRUE(v1);

    // ABSubclass<1> is primary, so it should be instantiated first.
    ASSERT_LT(v1->getId(), v2->getId());
    // ABSubclass<2> depends upon ABSubclass<3>, so ABSubclass<3> should in instantiated first.
    ASSERT_LT(v3->getId(), v2->getId());
}

// ----- test_annotatedFfunctionManufacture -----

/**
 * Template function to create a std:functions that creates an Annotated subclasses of AB.
 *
 * @tparam X A number used to distinguish subclasses.
 * @tparam Dependencies Types of arguments to the ABSubclass<X> factory
 * @return A function to create a std:functions that creates a distinct subclasses of AB.
 */
template <typename Annotation, typename... Dependencies>
std::function<Annotated<Annotation, AB>(Dependencies...)> getAnnotatedFunctionFactory() {
    return [](Dependencies... dependencies) { return Annotated<Annotation, AB>(new AB); };
}

Component<
    Annotated<Annotation1, AB>,
    Annotated<Annotation2, AB>,
    Annotated<Annotation3, AB>,
    Annotated<Annotation4, AB>,
    Annotated<Annotation2, AB>>
getAnnotatedFunctionTestComponent() {
    return ComponentAccumulator<>()
        .addPrimaryFactory(getAnnotatedFunctionFactory<Annotation1>())
        .addRequiredFactory(getAnnotatedFunctionFactory<Annotation2>())
        .addRetainedFactory(getAnnotatedFunctionFactory<Annotation3, const Annotated<Annotation4, AB>&>())
        .addUnloadableFactory(getAnnotatedFunctionFactory<Annotation4>());
}

/**
 * Verify that std::function factories work and are invoked in the correct order.
 */
TEST_F(ManufactoryTest, test_anotatedFunctionManufacture) {
    auto component = getAnnotatedFunctionTestComponent();
    auto manufactory = Manufactory<
        Annotated<Annotation1, AB>,
        Annotated<Annotation2, AB>,
        Annotated<Annotation3, AB>,
        Annotated<Annotation4, AB>,
        Annotated<Annotation2, AB>>::create(component);
    ASSERT_TRUE(manufactory);

    auto v2 = manufactory->get<Annotated<Annotation2, AB>>();
    ASSERT_TRUE(v2);
    auto v3 = manufactory->get<Annotated<Annotation3, AB>>();
    ASSERT_TRUE(v3);
    auto v4 = manufactory->get<Annotated<Annotation4, AB>>();
    ASSERT_TRUE(v4);
    auto v1 = manufactory->get<Annotated<Annotation1, AB>>();
    ASSERT_TRUE(v1);

    // Annotated<Annotation1, AB> is primary, so it should be instantiated first.
    ASSERT_LT(v1->getId(), v2->getId());
    ASSERT_LT(v1->getId(), v3->getId());
    ASSERT_LT(v1->getId(), v4->getId());
    // Annotated<Annotation3, AB> depends upon Annotated<Annotation4, AB> but doesn't keep the reference and
    // Annotated<Annotation4, AB> is unloadable, so v4 gets a newer instance.
    ASSERT_LT(v3->getId(), v4->getId());
}

// ----- test_checkCyclicDependencies -----

/**
 * Factory for instances of InterfaceA that depends upon InterfaceB.
 *
 * @return An instance of InterfaceA.
 */
std::shared_ptr<InterfaceA> createCyclicA(const std::shared_ptr<InterfaceB>&) {
    return createAB();
}

/**
 * Factory for instances of InterfaceB that depends upon InterfaceA.
 *
 * @return An instance of InterfaceB.
 */
std::shared_ptr<InterfaceB> createCyclicB(const std::shared_ptr<InterfaceA>&) {
    return createAB();
}

/**
 * Definition of a component with a cyclic dependency graph (InterfaceA <-> InterfaceB).
 *
 * @return A component with a cyclic dependency graph.
 */
Component<std::shared_ptr<InterfaceA>, std::shared_ptr<InterfaceB>> getCyclicComponent() {
    ComponentAccumulator<> accumulator;
    return accumulator.addUnloadableFactory(createCyclicA).addUnloadableFactory(createCyclicB);
};

/**
 * Verify that circular dependencies are detected, and the creation of an Manufactory from a component with
 * a circular dependency will fail.
 */
TEST_F(ManufactoryTest, test_checkCyclicDependencies) {
    auto component = getCyclicComponent();
    auto manufactory = Manufactory<std::shared_ptr<InterfaceA>, std::shared_ptr<InterfaceB>>::create(component);
    ASSERT_FALSE(manufactory);
}

// ----- test_subManufactory -----

/**
 * Verify that the creation of a subManufactory works and that cached instances are shared between the
 * primary and subManufactorys.
 */
TEST_F(ManufactoryTest, test_subManufactory) {
    auto component = getABComponent();
    auto manufactory = Manufactory<std::shared_ptr<InterfaceA>, std::shared_ptr<InterfaceB>>::create(component);
    ASSERT_TRUE(manufactory);

    auto a = manufactory->get<std::shared_ptr<InterfaceA>>();
    ASSERT_TRUE(a);
    auto b = manufactory->get<std::shared_ptr<InterfaceB>>();
    ASSERT_TRUE(b);
    ASSERT_TRUE(b->getString().empty());
    a->setString("something");
    ASSERT_FALSE(b->getString().empty());

    auto subsetManufactory = manufactory->createSubsetManufactory<std::shared_ptr<InterfaceB>>();
    auto subB = subsetManufactory->get<std::shared_ptr<InterfaceB>>();
    ASSERT_TRUE(subB);
    ASSERT_EQ(b, subB);
}

}  // namespace test
}  // namespace acsdkManufactory
}  // namespace alexaClientSDK
