#include <gtest/gtest.h>
#include "AdaptiveRadixTree.h"
#include "art/Encoding.h"
#include <vector>
#include <cstring>

// NOTE: AI generated tests

// Type aliases for convenience
using TreeType = ART::AdaptiveRadixTree<Encoding<uint64_t>, int>;
using KeyType = Encoding<uint64_t>;

class InsertUpdateTest : public ::testing::Test {
protected:
    TreeType tree;

    KeyType make_key(uint64_t value) {
        return KeyType(value);
    }
};

// ============================================================================
// Basic Insert Tests - Node4
// ============================================================================

TEST_F(InsertUpdateTest, InsertSingleKeyIntoEmptyTreeNode4) {
    KeyType key = make_key(42);
    int value = 100;
    
    tree.insert(key, value);
    
    int* result = tree.at(key);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, value);
}

TEST_F(InsertUpdateTest, InsertMultipleKeysIntoNode4) {
    // Tests: 5 keys inserted into Node4 at root
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

// ============================================================================
// Update Tests - In-place value updates
// ============================================================================

TEST_F(InsertUpdateTest, UpdateExistingKeyInPlaceNode4) {
    KeyType key = make_key(100);
    int value1 = 50;
    int value2 = 150;
    
    tree.insert(key, value1);
    int* result = tree.at(key);
    EXPECT_EQ(*result, value1);
    
    tree.update(key, value2);
    result = tree.at(key);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, value2);
}

TEST_F(InsertUpdateTest, UpdateMultipleKeysInNode4) {
    std::vector<uint64_t> keys = {10, 20, 30};
    
    for (auto k : keys) {
        KeyType key = make_key(k);
        int value = static_cast<int>(k);
        tree.insert(key, value);
    }
    
    for (auto k : keys) {
        KeyType key = make_key(k);
        int new_value = static_cast<int>(k * 2);
        tree.update(key, new_value);
    }
    
    for (auto k : keys) {
        KeyType key = make_key(k);
        int* result = tree.at(key);
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(*result, static_cast<int>(k * 2));
    }
}

// ============================================================================
// Prefix Collision Tests - Node4 children with shared prefixes
// ============================================================================

TEST_F(InsertUpdateTest, PrefixCollisionSimpleNode4) {
    // Tests: 3 keys with similar low bytes: 0x...001, 0x...002, 0x...003
    // Node4 disambiguates at byte boundary
    // Keys with similar byte patterns
    KeyType key1 = make_key(0x0000000000000001ULL);
    KeyType key2 = make_key(0x0000000000000002ULL);
    KeyType key3 = make_key(0x0000000000000003ULL);
    
    tree.insert(key1, 1);
    tree.insert(key2, 2);
    tree.insert(key3, 3);
    
    EXPECT_EQ(*tree.at(key1), 1);
    EXPECT_EQ(*tree.at(key2), 2);
    EXPECT_EQ(*tree.at(key3), 3);
}

TEST_F(InsertUpdateTest, PrefixCollisionLargeNode4To16To48) {
    // Tests: 8 keys with 3 different prefixes (0x1000..., 0x1100..., 0x2000...)
    // Root Node4 branches into subtrees handling each prefix
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
    
    for (size_t i = 0; i < keys.size(); ++i) {
        KeyType key = make_key(keys[i]);
        int* result = tree.at(key);
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(*result, static_cast<int>(i));
    }
}

// ============================================================================
// Tree Growth Tests - Node4  ->  Node16  ->  Node48  ->  Node256
// ============================================================================

TEST_F(InsertUpdateTest, TreeGrowthWith300KeysNode4To16To48To256) {
    // Tests: 300 sequential keys force node growth/expansion at each level
    // Verifies correctness during all node type transitions
    // Insert many keys to cause node growth (Node4 -> Node16 -> Node48 -> Node256)
    const int num_keys = 300;
    
    for (int i = 0; i < num_keys; ++i) {
        KeyType key = make_key(i);
        tree.insert(key, i * 10);
    }
    
    // Verify all keys are still accessible
    for (int i = 0; i < num_keys; ++i) {
        KeyType key = make_key(i);
        int* result = tree.at(key);
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(*result, i * 10);
    }
}

TEST_F(InsertUpdateTest, InsertSequentialKeysNode4To16) {
    // Tests: 50 sequential keys create deep tree with multiple Node4/Node16 levels
    // Validates correct insertion path even with similar prefixes
    // Insert sequential keys which may have similar prefixes
    const int num_keys = 50;
    for (int i = 0; i < num_keys; ++i) {
        KeyType key = make_key(i);
        tree.insert(key, i * 100);
    }
    
    for (int i = 0; i < num_keys; ++i) {
        KeyType key = make_key(i);
        int* result = tree.at(key);
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(*result, i * 100);
    }
}

// ============================================================================
// Edge Cases - Boundary values
// ============================================================================

TEST_F(InsertUpdateTest, InsertLargeValueRangesNode4) {
    // Tests: Keys {0, UINT64_MAX, UINT64_MAX/2}, Values {INT32_MIN, INT32_MAX, 0}
    // Extreme boundary conditions
    KeyType key1 = make_key(0ULL);
    KeyType key2 = make_key(UINT64_MAX);
    KeyType key3 = make_key(UINT64_MAX / 2);
    
    tree.insert(key1, INT32_MIN);
    tree.insert(key2, INT32_MAX);
    tree.insert(key3, 0);
    
    EXPECT_EQ(*tree.at(key1), INT32_MIN);
    EXPECT_EQ(*tree.at(key2), INT32_MAX);
    EXPECT_EQ(*tree.at(key3), 0);
}

TEST_F(InsertUpdateTest, InsertNegativeValuesNode4) {
    KeyType key1 = make_key(1);
    KeyType key2 = make_key(2);
    
    tree.insert(key1, -100);
    tree.insert(key2, -1);
    
    EXPECT_EQ(*tree.at(key1), -100);
    EXPECT_EQ(*tree.at(key2), -1);
}

TEST_F(InsertUpdateTest, InsertZeroValueNode4) {
    KeyType key = make_key(999);
    int value = 0;
    
    tree.insert(key, value);
    
    int* result = tree.at(key);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, 0);
}

// ============================================================================
// Complex Sequences: Insert  ->  Update  ->  Erase
// ============================================================================

TEST_F(InsertUpdateTest, InsertUpdateEraseThreeKeysNode4) {
    // Tests: 3-key sequence {111, 222, 333}  ->  update 2 of them  ->  erase 2
    // Verifies state correctness after all operations
    KeyType key1 = make_key(111);
    KeyType key2 = make_key(222);
    KeyType key3 = make_key(333);
    
    // Insert phase
    tree.insert(key1, 1);
    tree.insert(key2, 2);
    tree.insert(key3, 3);
    
    // Update phase
    tree.update(key1, 10);
    tree.update(key2, 20);
    
    EXPECT_EQ(*tree.at(key1), 10);
    EXPECT_EQ(*tree.at(key2), 20);
    EXPECT_EQ(*tree.at(key3), 3);
    
    // Erase phase
    tree.erase(key1);
    tree.erase(key3);
    
    EXPECT_EQ(tree.at(key1), nullptr);
    EXPECT_EQ(*tree.at(key2), 20);
    EXPECT_EQ(tree.at(key3), nullptr);
}

TEST_F(InsertUpdateTest, ComplexSequenceWithSelectiveUpdateAndEraseNode4) {
    // Tests: Insert 5 base keys  ->  Update even indices  ->  Insert 4 new keys  ->  Erase even indices
    // Intricate sequence testing update and erase interactions
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
    
    // Phase 3: Add new keys
    std::vector<uint64_t> new_keys = {5, 50, 500, 5000};
    for (auto k : new_keys) {
        KeyType key = make_key(k);
        tree.insert(key, -static_cast<int>(k));
    }
    
    // Phase 4: Erase original every-other key
    for (size_t i = 0; i < base_keys.size(); i += 2) {
        KeyType key = make_key(base_keys[i]);
        tree.erase(key);
    }
    
    // Verify final state
    for (size_t i = 0; i < base_keys.size(); ++i) {
        KeyType key = make_key(base_keys[i]);
        if (i % 2 == 0) {
            EXPECT_EQ(tree.at(key), nullptr) << "Key " << base_keys[i] << " should be erased";
        } else {
            int* result = tree.at(key);
            ASSERT_NE(result, nullptr) << "Key " << base_keys[i] << " should exist";
            EXPECT_EQ(*result, static_cast<int>(base_keys[i]));
        }
    }
    
    for (auto k : new_keys) {
        KeyType key = make_key(k);
        int* result = tree.at(key);
        ASSERT_NE(result, nullptr) << "New key " << k << " should exist";
        EXPECT_EQ(*result, -static_cast<int>(k));
    }
}
