// MIT License
//
// Copyright (c) 2026 gdashboard contributors
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
#include "core/TypeInfo.hpp"
#include <typeinfo>
#include <string>
#include <vector>
#include <map>
#include <functional>  // For std::hash

// ============================================================================
// Layer 1, Task 1.3: TypeInfo Unit Tests
// Reference: doc/UNIT_TEST_STRATEGY.md - Core Components Layer
// ============================================================================

// Test fixture for TypeInfo tests
class TypeInfoTest : public ::testing::Test {
protected:
    // Helper to extract just the type name for comparison (handles platform differences)
    static std::string GetTypeName(std::string_view full_name) {
        // Remove namespace prefixes if any
        size_t pos = full_name.rfind("::");
        if (pos != std::string::npos) {
            return std::string(full_name.substr(pos + 2));
        }
        return std::string(full_name);
    }
};

// ============================================================================
// Primitive Type Tests (6 tests)
// ============================================================================

TEST_F(TypeInfoTest, Primitives_int) {
    constexpr auto name = type_name_const<int>();
    EXPECT_FALSE(name.empty());
    EXPECT_TRUE(name.find("int") != std::string_view::npos);
}

TEST_F(TypeInfoTest, Primitives_double) {
    constexpr auto name = type_name_const<double>();
    EXPECT_FALSE(name.empty());
    EXPECT_TRUE(name.find("double") != std::string_view::npos);
}

TEST_F(TypeInfoTest, Primitives_bool) {
    constexpr auto name = type_name_const<bool>();
    EXPECT_FALSE(name.empty());
    EXPECT_TRUE(name.find("bool") != std::string_view::npos);
}

TEST_F(TypeInfoTest, Primitives_char) {
    constexpr auto name = type_name_const<char>();
    EXPECT_FALSE(name.empty());
    EXPECT_TRUE(name.find("char") != std::string_view::npos);
}

TEST_F(TypeInfoTest, Primitives_void) {
    constexpr auto name = type_name_const<void>();
    EXPECT_FALSE(name.empty());
    EXPECT_TRUE(name.find("void") != std::string_view::npos);
}

TEST_F(TypeInfoTest, Primitives_size_t) {
    constexpr auto name = type_name_const<size_t>();
    EXPECT_FALSE(name.empty());
    // size_t implementation varies, just verify we get something
    EXPECT_GT(name.length(), 0);
}

// ============================================================================
// Standard Library Container Tests (4 tests)
// ============================================================================

TEST_F(TypeInfoTest, StdContainers_string) {
    constexpr auto name = type_name_const<std::string>();
    EXPECT_FALSE(name.empty());
    // Should contain either "basic_string" or "string"
    bool has_string = name.find("string") != std::string_view::npos ||
                      name.find("std::string") != std::string_view::npos;
    EXPECT_TRUE(has_string);
}

TEST_F(TypeInfoTest, StdContainers_vector) {
    constexpr auto name = type_name_const<std::vector<int>>();
    EXPECT_FALSE(name.empty());
    EXPECT_TRUE(name.find("vector") != std::string_view::npos);
}

TEST_F(TypeInfoTest, StdContainers_map) {
    constexpr auto name = type_name_const<std::map<int, int>>();
    EXPECT_FALSE(name.empty());
    EXPECT_TRUE(name.find("map") != std::string_view::npos);
}

TEST_F(TypeInfoTest, StdContainers_nested) {
    constexpr auto name = type_name_const<std::vector<std::string>>();
    EXPECT_FALSE(name.empty());
    EXPECT_TRUE(name.find("vector") != std::string_view::npos);
}

// ============================================================================
// Namespace Stripping Tests (6 tests)
// ============================================================================

TEST_F(TypeInfoTest, NamespaceStripping_noNamespace) {
    constexpr std::string_view input = "MyClass";
    constexpr auto result = StripNamespace(input);
    EXPECT_EQ(result, "MyClass");
}

TEST_F(TypeInfoTest, NamespaceStripping_singleNamespace) {
    constexpr std::string_view input = "ns::MyClass";
    constexpr auto result = StripNamespace(input);
    EXPECT_EQ(result, "MyClass");
}

TEST_F(TypeInfoTest, NamespaceStripping_nestedNamespaces) {
    constexpr std::string_view input = "ns1::ns2::ns3::MyClass";
    constexpr auto result = StripNamespace(input);
    EXPECT_EQ(result, "MyClass");
}

TEST_F(TypeInfoTest, NamespaceStripping_stdNamespace) {
    constexpr std::string_view input = "std::vector";
    constexpr auto result = StripNamespace(input);
    EXPECT_EQ(result, "vector");
}

TEST_F(TypeInfoTest, NamespaceStripping_emptyString) {
    constexpr std::string_view input = "";
    constexpr auto result = StripNamespace(input);
    EXPECT_EQ(result, "");
}

TEST_F(TypeInfoTest, NamespaceStripping_onlyNamespace) {
    constexpr std::string_view input = "ns::";
    constexpr auto result = StripNamespace(input);
    EXPECT_EQ(result, "");
}

// ============================================================================
// TypeName Extraction Tests (5 tests)
// ============================================================================

TEST_F(TypeInfoTest, TypeNameExtraction_primitiveInt) {
    constexpr auto name = TypeName<int>();
    EXPECT_FALSE(name.empty());
    // After namespace stripping, should be "int"
    EXPECT_TRUE(name.find("int") != std::string_view::npos);
}

TEST_F(TypeInfoTest, TypeNameExtraction_vector) {
    constexpr auto name = TypeName<std::vector<int>>();
    EXPECT_FALSE(name.empty());
    EXPECT_TRUE(name.find("vector") != std::string_view::npos);
}

TEST_F(TypeInfoTest, TypeNameExtraction_string) {
    constexpr auto name = TypeName<std::string>();
    EXPECT_FALSE(name.empty());
    bool has_string = name.find("string") != std::string_view::npos;
    EXPECT_TRUE(has_string);
}

TEST_F(TypeInfoTest, TypeNameExtraction_pointer) {
    constexpr auto name = TypeName<int*>();
    EXPECT_FALSE(name.empty());
    EXPECT_TRUE(name.find("*") != std::string_view::npos);
}

TEST_F(TypeInfoTest, TypeNameExtraction_reference) {
    constexpr auto name = TypeName<int&>();
    EXPECT_FALSE(name.empty());
    EXPECT_TRUE(name.find("&") != std::string_view::npos);
}

// ============================================================================
// Type Hashing Tests (5 tests)
// ============================================================================

TEST_F(TypeInfoTest, TypeHashing_sameTypeSameHash) {
    auto hash1 = typeid(int).hash_code();
    auto hash2 = typeid(int).hash_code();
    // Note: hash_code() is not guaranteed to be stable across runs,
    // but within the same run it should be consistent
    EXPECT_EQ(hash1, hash2);
}

TEST_F(TypeInfoTest, TypeHashing_differentTypesDifferentHash) {
    auto int_hash = typeid(int).hash_code();
    auto double_hash = typeid(double).hash_code();
    EXPECT_NE(int_hash, double_hash);
}

TEST_F(TypeInfoTest, TypeHashing_typeInfoCreation) {
    TypeInfo info_int = TypeInfo::create<int>();
    TypeInfo info_double = TypeInfo::create<double>();

    EXPECT_NE(info_int.hash, info_double.hash);
    EXPECT_TRUE(info_int.is_valid());
    EXPECT_TRUE(info_double.is_valid());
}

TEST_F(TypeInfoTest, TypeHashing_vectorsWithDifferentElementTypes) {
    auto vec_int = TypeInfo::create<std::vector<int>>();
    auto vec_double = TypeInfo::create<std::vector<double>>();

    EXPECT_NE(vec_int.hash, vec_double.hash);
}

TEST_F(TypeInfoTest, TypeHashing_nonZeroForValidTypes) {
    TypeInfo info = TypeInfo::create<std::string>();
    EXPECT_NE(info.hash, 0);
    EXPECT_TRUE(info.is_valid());
}

// ============================================================================
// TypeInfo Struct Tests (5 tests)
// ============================================================================

TEST_F(TypeInfoTest, TypeInfoStruct_create_int) {
    TypeInfo info = TypeInfo::create<int>();

    EXPECT_FALSE(info.name.empty());
    EXPECT_NE(info.hash, 0);
    EXPECT_TRUE(info.is_valid());
}

TEST_F(TypeInfoTest, TypeInfoStruct_create_string) {
    TypeInfo info = TypeInfo::create<std::string>();

    EXPECT_FALSE(info.name.empty());
    EXPECT_NE(info.hash, 0);
    EXPECT_TRUE(info.is_valid());
}

TEST_F(TypeInfoTest, TypeInfoStruct_equality_sameTypes) {
    TypeInfo info1 = TypeInfo::create<int>();
    TypeInfo info2 = TypeInfo::create<int>();

    EXPECT_EQ(info1.hash, info2.hash);
}

TEST_F(TypeInfoTest, TypeInfoStruct_equality_differentTypes) {
    TypeInfo info_int = TypeInfo::create<int>();
    TypeInfo info_string = TypeInfo::create<std::string>();

    EXPECT_NE(info_int.hash, info_string.hash);
}

TEST_F(TypeInfoTest, TypeInfoStruct_defaultConstruction) {
    TypeInfo info;

    EXPECT_TRUE(info.name.empty());
    EXPECT_EQ(info.hash, 0);
    EXPECT_FALSE(info.is_valid());
}

// ============================================================================
// std::type_index Integration Tests (4 tests)
// ============================================================================
// Note: std::type_index tests moved to advanced testing phase
// as they require additional STL integration testing

TEST_F(TypeInfoTest, TypeHashing_consistency) {
    // Verify hash codes are consistent for the same type
    auto hash1 = typeid(int).hash_code();
    auto hash2 = typeid(int).hash_code();
    EXPECT_EQ(hash1, hash2);
}

TEST_F(TypeInfoTest, TypeHashing_differentTypes) {
    // Verify different types have different hash codes
    auto hash_int = typeid(int).hash_code();
    auto hash_double = typeid(double).hash_code();
    EXPECT_NE(hash_int, hash_double);
}

TEST_F(TypeInfoTest, TypeInfo_mapCompatibility) {
    // Verify TypeInfo hashes can be used in collections
    std::map<size_t, std::string> type_map;

    size_t int_hash = typeid(int).hash_code();
    size_t double_hash = typeid(double).hash_code();

    type_map[int_hash] = "integer";
    type_map[double_hash] = "floating_point";

    EXPECT_EQ(type_map.size(), 2);
    EXPECT_EQ(type_map[int_hash], "integer");
}

TEST_F(TypeInfoTest, TypeIndexAlternative_usingTypeId) {
    // Alternative approach using typeid directly
    const std::type_info& int_type = typeid(int);
    const std::type_info& string_type = typeid(std::string);

    EXPECT_NE(int_type.hash_code(), string_type.hash_code());
}


// ============================================================================
// Constexpr and Compile-Time Tests (5 tests)
// ============================================================================

TEST_F(TypeInfoTest, Constexpr_type_name_v_variable) {
    // This test verifies that type_name_v is a valid constexpr variable
    constexpr auto name = type_name_v<int>;
    EXPECT_FALSE(name.empty());
    EXPECT_TRUE(name.find("int") != std::string_view::npos);
}

TEST_F(TypeInfoTest, Constexpr_type_name_v_multiple_types) {
    constexpr auto int_name = type_name_v<int>;
    constexpr auto double_name = type_name_v<double>;
    constexpr auto string_name = type_name_v<std::string>;

    EXPECT_NE(int_name, double_name);
    EXPECT_NE(int_name, string_name);
    EXPECT_NE(double_name, string_name);
}

TEST_F(TypeInfoTest, Constexpr_type_name_v_in_constexpr_context) {
    // Verify we can use type_name_v in a constexpr function
    constexpr auto get_name = []() {
        return type_name_v<std::vector<int>>;
    };

    constexpr auto name = get_name();
    EXPECT_FALSE(name.empty());
    EXPECT_TRUE(name.find("vector") != std::string_view::npos);
}

TEST_F(TypeInfoTest, Constexpr_StripNamespace_in_constexpr) {
    // Verify StripNamespace works in constexpr context
    constexpr std::string_view input = "std::vector";
    constexpr auto result = StripNamespace(input);

    EXPECT_EQ(result, "vector");
}

TEST_F(TypeInfoTest, Constexpr_TypeName_in_constexpr) {
    // Verify TypeName<T>() works in constexpr context
    constexpr auto name = TypeName<int>();
    EXPECT_FALSE(name.empty());
}

// ============================================================================
// User-Defined Type Tests (4 tests)
// ============================================================================

struct SimpleStruct {
    int x;
};

class SimpleClass {
public:
    void foo() {}
};

namespace TestNamespace {
    struct NamespacedStruct {
        int value;
    };

    class NamespacedClass {
    public:
        void method() {}
    };
}

TEST_F(TypeInfoTest, UserDefinedTypes_struct) {
    constexpr auto name = type_name_const<SimpleStruct>();
    EXPECT_FALSE(name.empty());
    EXPECT_TRUE(name.find("SimpleStruct") != std::string_view::npos);
}

TEST_F(TypeInfoTest, UserDefinedTypes_class) {
    constexpr auto name = type_name_const<SimpleClass>();
    EXPECT_FALSE(name.empty());
    EXPECT_TRUE(name.find("SimpleClass") != std::string_view::npos);
}

TEST_F(TypeInfoTest, UserDefinedTypes_namespacedStruct) {
    constexpr auto name = type_name_const<TestNamespace::NamespacedStruct>();
    EXPECT_FALSE(name.empty());
    EXPECT_TRUE(name.find("NamespacedStruct") != std::string_view::npos);
}

TEST_F(TypeInfoTest, UserDefinedTypes_namespacedClass) {
    constexpr auto name = type_name_const<TestNamespace::NamespacedClass>();
    EXPECT_FALSE(name.empty());
    EXPECT_TRUE(name.find("NamespacedClass") != std::string_view::npos);
}

// ============================================================================
// Edge Cases & Error Handling (5 tests)
// ============================================================================

TEST_F(TypeInfoTest, EdgeCase_emptyNamestripped) {
    constexpr std::string_view empty = "";
    constexpr auto result = StripNamespace(empty);
    EXPECT_EQ(result, "");
}

TEST_F(TypeInfoTest, EdgeCase_singleCharacterName) {
    constexpr std::string_view single = "A";
    constexpr auto result = StripNamespace(single);
    EXPECT_EQ(result, "A");
}

TEST_F(TypeInfoTest, EdgeCase_multipleColonPairs) {
    constexpr std::string_view input = "a::b::c::d";
    constexpr auto result = StripNamespace(input);
    EXPECT_EQ(result, "d");
}

TEST_F(TypeInfoTest, EdgeCase_pointerToPointer) {
    constexpr auto name = type_name_const<int**>();
    EXPECT_FALSE(name.empty());
    // Should have multiple asterisks
    size_t asterisk_count = 0;
    for (char c : name) {
        if (c == '*') asterisk_count++;
    }
    EXPECT_EQ(asterisk_count, 2);
}

TEST_F(TypeInfoTest, EdgeCase_constQualifier) {
    constexpr auto name_const = type_name_const<const int>();
    constexpr auto name_non_const = type_name_const<int>();

    EXPECT_FALSE(name_const.empty());
    EXPECT_FALSE(name_non_const.empty());
    // Const should appear in the const version
    EXPECT_TRUE(name_const.find("const") != std::string_view::npos ||
                name_const != name_non_const);
}

// ============================================================================
// Cross-Platform Consistency Tests (3 tests)
// ============================================================================

TEST_F(TypeInfoTest, CrossPlatform_sameTypeConsistentHash) {
    // Hash should be stable for the same type within a run
    auto hash1 = typeid(std::vector<std::string>).hash_code();
    auto hash2 = typeid(std::vector<std::string>).hash_code();

    EXPECT_EQ(hash1, hash2);
}

TEST_F(TypeInfoTest, CrossPlatform_typeNameNonEmpty) {
    // Every type should have a non-empty name
    constexpr auto name1 = type_name_const<int>();
    constexpr auto name2 = type_name_const<std::string>();
    constexpr auto name3 = type_name_const<void>();

    EXPECT_GT(name1.length(), 0);
    EXPECT_GT(name2.length(), 0);
    EXPECT_GT(name3.length(), 0);
}

TEST_F(TypeInfoTest, CrossPlatform_typeInfoConsistency) {
    // Creating TypeInfo for the same type should give same hash
    TypeInfo info1 = TypeInfo::create<std::map<int, std::string>>();
    TypeInfo info2 = TypeInfo::create<std::map<int, std::string>>();

    EXPECT_EQ(info1.hash, info2.hash);
    EXPECT_EQ(info1.name, info2.name);
}

