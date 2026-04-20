// MIT License
//
// Copyright (c) 2025 gdashboard contributors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <gtest/gtest.h>
#include "graph/CapabilityBus.hpp"
#include <memory>

// ============================================================================
// Layer 4, Task 4.6: CapabilityBus Unit Tests
// Reference: doc/UNIT_TEST_STRATEGY.md - CapabilityBus Tests
// ============================================================================

namespace {

// Mock capability types for testing
struct MockCapabilityA {
    int value = 42;
};

struct MockCapabilityB {
    std::string name = "test_b";
};

struct MockCapabilityC {
    double data = 3.14;
};

} // namespace

// ============================================================================
// CapabilityBus Tests
// ============================================================================

class CapabilityBusTest : public ::testing::Test {
protected:
    graph::CapabilityBus bus_;
};

TEST_F(CapabilityBusTest, RegisterAndRetrieveCapability) {
    auto cap_a = std::make_shared<MockCapabilityA>();
    cap_a->value = 100;

    bus_.Register<MockCapabilityA>(cap_a);

    auto retrieved = bus_.Get<MockCapabilityA>();
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->value, 100);
}

TEST_F(CapabilityBusTest, RetrieveNonRegisteredCapability_ReturnsNullptr) {
    auto retrieved = bus_.Get<MockCapabilityA>();
    EXPECT_EQ(retrieved, nullptr);
}

TEST_F(CapabilityBusTest, MultipleCapabilitiesSameBus) {
    auto cap_a = std::make_shared<MockCapabilityA>();
    cap_a->value = 50;

    auto cap_b = std::make_shared<MockCapabilityB>();
    cap_b->name = "capability_b";

    bus_.Register<MockCapabilityA>(cap_a);
    bus_.Register<MockCapabilityB>(cap_b);

    auto retrieved_a = bus_.Get<MockCapabilityA>();
    auto retrieved_b = bus_.Get<MockCapabilityB>();

    ASSERT_NE(retrieved_a, nullptr);
    ASSERT_NE(retrieved_b, nullptr);
    EXPECT_EQ(retrieved_a->value, 50);
    EXPECT_EQ(retrieved_b->name, "capability_b");
}

TEST_F(CapabilityBusTest, OverwriteCapability) {
    auto cap_a_v1 = std::make_shared<MockCapabilityA>();
    cap_a_v1->value = 10;
    bus_.Register<MockCapabilityA>(cap_a_v1);

    auto cap_a_v2 = std::make_shared<MockCapabilityA>();
    cap_a_v2->value = 20;
    bus_.Register<MockCapabilityA>(cap_a_v2);

    auto retrieved = bus_.Get<MockCapabilityA>();
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->value, 20);
}

TEST_F(CapabilityBusTest, ReferenceSemantics_SameInstanceRetrieved) {
    auto cap_a = std::make_shared<MockCapabilityA>();
    cap_a->value = 42;
    bus_.Register<MockCapabilityA>(cap_a);

    auto retrieved1 = bus_.Get<MockCapabilityA>();
    auto retrieved2 = bus_.Get<MockCapabilityA>();

    ASSERT_NE(retrieved1, nullptr);
    ASSERT_NE(retrieved2, nullptr);
    EXPECT_EQ(retrieved1.get(), retrieved2.get());
    EXPECT_EQ(retrieved1.get(), cap_a.get());
}

TEST_F(CapabilityBusTest, Has_RegisteredCapability_ReturnsTrue) {
    auto cap_a = std::make_shared<MockCapabilityA>();
    bus_.Register<MockCapabilityA>(cap_a);

    EXPECT_TRUE(bus_.Has<MockCapabilityA>());
}

TEST_F(CapabilityBusTest, Has_UnregisteredCapability_ReturnsFalse) {
    EXPECT_FALSE(bus_.Has<MockCapabilityA>());

    auto cap_b = std::make_shared<MockCapabilityB>();
    bus_.Register<MockCapabilityB>(cap_b);

    EXPECT_FALSE(bus_.Has<MockCapabilityA>());
    EXPECT_TRUE(bus_.Has<MockCapabilityB>());
}

TEST_F(CapabilityBusTest, Clear_RemovesAllCapabilities) {
    auto cap_a = std::make_shared<MockCapabilityA>();
    auto cap_b = std::make_shared<MockCapabilityB>();
    auto cap_c = std::make_shared<MockCapabilityC>();

    bus_.Register<MockCapabilityA>(cap_a);
    bus_.Register<MockCapabilityB>(cap_b);
    bus_.Register<MockCapabilityC>(cap_c);

    EXPECT_TRUE(bus_.Has<MockCapabilityA>());
    EXPECT_TRUE(bus_.Has<MockCapabilityB>());
    EXPECT_TRUE(bus_.Has<MockCapabilityC>());

    bus_.Clear();

    EXPECT_FALSE(bus_.Has<MockCapabilityA>());
    EXPECT_FALSE(bus_.Has<MockCapabilityB>());
    EXPECT_FALSE(bus_.Has<MockCapabilityC>());
}

TEST_F(CapabilityBusTest, MutateRetrievedCapability_AffectsRegisteredCapability) {
    auto cap_a = std::make_shared<MockCapabilityA>();
    cap_a->value = 100;
    bus_.Register<MockCapabilityA>(cap_a);

    auto retrieved = bus_.Get<MockCapabilityA>();
    ASSERT_NE(retrieved, nullptr);

    retrieved->value = 200;

    auto retrieved_again = bus_.Get<MockCapabilityA>();
    EXPECT_EQ(retrieved_again->value, 200);
}
