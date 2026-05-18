#include <gtest/gtest.h>
#include "AdaptiveRadixTree.h"
#include "art/Encoding.h"
#include <vector>

// NOTE: AI generated tests

// Type aliases for convenience
using TreeType = ART::AdaptiveRadixTree<Encoding<uint64_t>, int>;
using KeyType = Encoding<uint64_t>;

class AtTest : public ::testing::Test {
protected:
    TreeType tree;

    KeyType make_key(uint64_t value) {
        return KeyType(value);
    }
};

// ============================================================================
// Basic Lookup Tests - Node4
// ============================================================================

TEST_F(AtTest, LookupSingleKeyInNode4) {
    KeyType key = make_key(42);
    int value = 100;
    
    tree.insert(key, value);
    
    int* result = tree.at(key);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, value);
}

TEST_F(AtTest, LookupMultipleKeysInNode4) {
    std::vector<uint64_t> keys = {1, 2, 3, 4, 5};
    
    for (size_t i = 0; i < keys.size(); ++i) {
        KeyType key = make_key(keys[i]);
        int value = static_cast<int>(i * 10);
        tree.insert(key, value);
    }
    
    for (size_t i = 0; i < keys.size(); ++i) {
        KeyType key = make_key(keys[i]);
        int* result = tree.at(key);
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(*result, static_cast<int>(i * 10));
    }
}

TEST_F(AtTest, LookupNonexistentKeyReturnNullptrNode4) {
    // Tests: Search for missing key returns nullptr without error
    // Node4 search termination without false positives
    KeyType key1 = make_key(1);
    int value = 10;
    tree.insert(key1, value);
    
    KeyType key2 = make_key(999);
    int* result = tree.at(key2);
    EXPECT_EQ(result, nullptr);
}

// ============================================================================
// Lookup with Prefix Collisions - Node16, Node48 (shared prefixes)
// ============================================================================

TEST_F(AtTest, LookupWithPrefixCollisionSimpleNode4) {
    // Tests: Keys with minimal differences in low bytes
    // Node4 children differentiate similar keys
    // Keys with similar byte patterns
    KeyType key1 = make_key(0x0000000000000001ULL);
    KeyType key2 = make_key(0x0000000000000002ULL);
    KeyType key3 = make_key(0x0000000000000003ULL);
    
    tree.insert(key1, 1);
    tree.insert(key2, 2);
    tree.insert(key3, 3);
    // NOTE: AI generated tests

    // Verify lookup works correctly for all keys
    EXPECT_EQ(*tree.at(key1), 1);
    EXPECT_EQ(*tree.at(key2), 2);
    EXPECT_EQ(*tree.at(key3), 3);
}

TEST_F(AtTest, LookupWithPrefixCollisionLargeNode4To16To48) {
    // Tests: 8 keys with common prefixes: 0x1000... (6 keys), 0x1100..., 0x2000...
    // Root Node4  ->  Node16/Node48 for prefix subtrees
    // Many keys with common prefix byte sequences
    std::vector<uint64_t> keys = {
        0x1000000000000000ULL,
        0x1000000000000001ULL,
        0x1000000000000002ULL,
        0x1000000000000100ULL,
        0x1000000000000101ULL,
        0x1100000000000000ULL,
        0x2000000000000000ULL,
        0xFFFFFFFFFFFFFFFFULL,
    };
    
    for (size_t i = 0; i < keys.size(); ++i) {
        KeyType key = make_key(keys[i]);
        tree.insert(key, static_cast<int>(i));
    }
    
    // Verify all lookups return correct values
    for (size_t i = 0; i < keys.size(); ++i) {
        KeyType key = make_key(keys[i]);
        int* result = tree.at(key);
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(*result, static_cast<int>(i));
    }
}

// ============================================================================
// Lookup in Large Trees - Node16, Node48, Node256
// ============================================================================

TEST_F(AtTest, LookupRandomKeysIn300ItemLargeTreeNode16To48To256) {
    // Tests: 300 sequential keys cause Node4  ->  Node16  ->  Node48  ->  Node256 growth
    // Random access patterns verify correct traversal
    // Insert many keys to test lookup performance and correctness
    const int num_keys = 300;
    
    for (int i = 0; i < num_keys; ++i) {
        KeyType key = make_key(i);
        tree.insert(key, i * 10);
    }
    
    // Verify random lookups work correctly
    std::vector<int> test_indices = {0, 1, 10, 50, 100, 149, 150, 200, 250, 299};
    for (int idx : test_indices) {
        KeyType key = make_key(idx);
        int* result = tree.at(key);
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(*result, idx * 10);
    }
}

TEST_F(AtTest, LookupAllSequentialKeysIn50ItemTreeNode4To16) {
    // Tests: Sequential keys create deep tree with transitions
    // All keys accessible despite tree structure changes
    // Insert sequential keys
    const int num_keys = 50;
    for (int i = 0; i < num_keys; ++i) {
        KeyType key = make_key(i);
        tree.insert(key, i * 100);
    }
    
    // Lookup all keys
    for (int i = 0; i < num_keys; ++i) {
        KeyType key = make_key(i);
        int* result = tree.at(key);
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(*result, i * 100);
    }
}

// ============================================================================
// Lookup After Updates - Update propagation
// ============================================================================

TEST_F(AtTest, LookupReturnsUpdatedValueInNode4) {
    KeyType key = make_key(100);
    int value1 = 50;
    int value2 = 150;
    
    tree.insert(key, value1);
    EXPECT_EQ(*tree.at(key), value1);
    
    tree.update(key, value2);
    int* result = tree.at(key);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, value2);
}

TEST_F(AtTest, LookupReturnsConsistentlyUpdatedValueNode4) {
    // Tests: Same key updated 3 times, each lookup sees latest value
    // In-place value updates in leaf node
    KeyType key = make_key(42);
    
    tree.insert(key, 1);
    EXPECT_EQ(*tree.at(key), 1);
    
    tree.update(key, 2);
    EXPECT_EQ(*tree.at(key), 2);
    
    tree.update(key, 3);
    EXPECT_EQ(*tree.at(key), 3);
    
    tree.update(key, 4);
    EXPECT_EQ(*tree.at(key), 4);
}

// ============================================================================
// Lookup Edge Cases
// ============================================================================

TEST_F(AtTest, LookupZeroKeyNode4) {
    KeyType key = make_key(0ULL);
    int value = 123;
    
    tree.insert(key, value);
    
    int* result = tree.at(key);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, value);
}

TEST_F(AtTest, LookupMaxUint64KeyNode4) {
    KeyType key = make_key(UINT64_MAX);
    int value = 999;
    
    tree.insert(key, value);
    
    int* result = tree.at(key);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, value);
}

TEST_F(AtTest, LookupMidpointUint64KeyNode4) {
    KeyType key = make_key(UINT64_MAX / 2);
    int value = 555;
    
    tree.insert(key, value);
    
    int* result = tree.at(key);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, value);
}

TEST_F(AtTest, LookupZeroValueNode4) {
    KeyType key = make_key(999);
    int value = 0;
    
    tree.insert(key, value);
    
    int* result = tree.at(key);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, 0);
}

TEST_F(AtTest, LookupNegativeValueNode4) {
    KeyType key1 = make_key(1);
    KeyType key2 = make_key(2);
    
    tree.insert(key1, -100);
    tree.insert(key2, -1);
    
    EXPECT_EQ(*tree.at(key1), -100);
    EXPECT_EQ(*tree.at(key2), -1);
}

// ============================================================================
// Lookup Correctness After Mixed Operations: Insert -> Update -> Erase
// ============================================================================

TEST_F(AtTest, LookupAfterInsertUpdateEraseSequenceNode4) {
    // Tests: 5 keys, update some, erase some, verify correctness
    // Verifies lookup works correctly after each operation
    std::vector<uint64_t> keys = {10, 20, 30, 40, 50};
    
    // Insert all
    for (auto k : keys) {
        KeyType key = make_key(k);
        tree.insert(key, static_cast<int>(k * 2));
    }
    
    // Update some
    tree.update(make_key(10), 200);
    tree.update(make_key(30), 300);
    
    // Erase some
    tree.erase(make_key(20));
    tree.erase(make_key(40));
    
    // Verify lookups
    EXPECT_EQ(*tree.at(make_key(10)), 200);
    EXPECT_EQ(tree.at(make_key(20)), nullptr);
    EXPECT_EQ(*tree.at(make_key(30)), 300);
    EXPECT_EQ(tree.at(make_key(40)), nullptr);
    EXPECT_EQ(*tree.at(make_key(50)), 100);
}

TEST_F(AtTest, LookupConsistentAfterComplexInsertUpdateEraseSequence) {
    // Tests: Complex 5-key sequence with selective update and persistence
    // Verifies lookup correctness through update operations and non-updated keys
    // Perform a complex sequence and verify lookup correctness
    std::vector<uint64_t> base_keys = {1, 10, 100, 1000, 10000};
    
    // Phase 1: Insert all keys
    for (auto k : base_keys) {
        KeyType key = make_key(k);
        tree.insert(key, static_cast<int>(k));
    }
    
    // Phase 2: Update some keys
    for (size_t i = 0; i < base_keys.size(); i += 2) {
        KeyType key = make_key(base_keys[i]);
        tree.update(key, static_cast<int>(base_keys[i] * 2));
    }
    
    // Verify updated values
    for (size_t i = 0; i < base_keys.size(); i += 2) {
        KeyType key = make_key(base_keys[i]);
        int* result = tree.at(key);
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(*result, static_cast<int>(base_keys[i] * 2));
    }
    
    // Verify non-updated values
    for (size_t i = 1; i < base_keys.size(); i += 2) {
        KeyType key = make_key(base_keys[i]);
        int* result = tree.at(key);
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(*result, static_cast<int>(base_keys[i]));
    }
}
