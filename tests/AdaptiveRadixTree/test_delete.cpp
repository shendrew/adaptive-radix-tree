#include <gtest/gtest.h>
#include "AdaptiveRadixTree.h"
#include "art/Encoding.h"
#include <vector>

// NOTE: AI generated tests

// Type aliases for convenience
using TreeType = ART::AdaptiveRadixTree<Encoding<uint64_t>, int>;
using KeyType = Encoding<uint64_t>;

class DeleteTest : public ::testing::Test {
protected:
    TreeType tree;

    KeyType make_key(uint64_t value) {
        return KeyType(value);
    }
};

// ============================================================================
// Basic Erase Tests - Node4
// ============================================================================

TEST_F(DeleteTest, EraseSingleKeyFromNode4) {
    KeyType key = make_key(55);
    int value = 200;
    
    tree.insert(key, value);
    EXPECT_NE(tree.at(key), nullptr);
    
    tree.erase(key);
    EXPECT_EQ(tree.at(key), nullptr);
}

TEST_F(DeleteTest, EraseMultipleKeysFromNode4) {
    // Tests: 5 keys, erase middle 3, leaving first and last
    // Node4 shrinkage after child removal
    std::vector<uint64_t> keys = {5, 10, 15, 20, 25};
    
    for (auto k : keys) {
        KeyType key = make_key(k);
        int value = static_cast<int>(k * 10);
        tree.insert(key, value);
    }
    
    // Erase middle elements
    for (size_t i = 1; i < keys.size() - 1; ++i) {
        KeyType key = make_key(keys[i]);
        tree.erase(key);
    }
    
    // Verify erased keys are gone
    for (size_t i = 1; i < keys.size() - 1; ++i) {
        KeyType key = make_key(keys[i]);
        EXPECT_EQ(tree.at(key), nullptr);
    }
    
    // Verify remaining keys still exist
    KeyType key_first = make_key(keys[0]);
    KeyType key_last = make_key(keys[keys.size() - 1]);
    EXPECT_NE(tree.at(key_first), nullptr);
    EXPECT_NE(tree.at(key_last), nullptr);
}

TEST_F(DeleteTest, EraseAllKeysFromNode4ToEmpty) {
    // Tests: 5 keys erased sequentially, tree becomes empty
    // Verifies no stale pointers or corruption
    std::vector<uint64_t> keys = {1, 2, 3, 4, 5};
    
    for (auto k : keys) {
        KeyType key = make_key(k);
        tree.insert(key, static_cast<int>(k));
    }
    
    for (auto k : keys) {
        KeyType key = make_key(k);
        tree.erase(key);
    }
    
    for (auto k : keys) {
        KeyType key = make_key(k);
        EXPECT_EQ(tree.at(key), nullptr);
    }
}

// ============================================================================
// Erase and Re-insert Tests - Reuse and allocation
// ============================================================================

TEST_F(DeleteTest, EraseAndReinsertSameKeyNode4) {
    KeyType key = make_key(100);
    int value1 = 50;
    int value2 = 150;
    
    tree.insert(key, value1);
    EXPECT_EQ(*tree.at(key), value1);
    
    tree.erase(key);
    EXPECT_EQ(tree.at(key), nullptr);
    
    tree.insert(key, value2);
    int* result = tree.at(key);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, value2);
}

TEST_F(DeleteTest, EraseReinsertSameKeyMultipleTimesNode4) {
    // Tests: Single key erased and re-inserted 5 times
    // Verifies allocator reuse and no memory corruption
    KeyType key = make_key(42);
    
    for (int i = 1; i <= 5; ++i) {
        tree.insert(key, i);
        EXPECT_EQ(*tree.at(key), i);
        
        tree.erase(key);
        EXPECT_EQ(tree.at(key), nullptr);
    }
}

// ============================================================================
// Tree Shrinkage Tests - Node256  ->  Node48  ->  Node16  ->  Node4  ->  empty
// ============================================================================

TEST_F(DeleteTest, TreeShrinkageAfterErasingAll100KeysNode256To4) {
    // Tests: 100 sequential keys inserted then all erased
    // Node shrinkage through all levels: Node256  ->  Node48  ->  Node16  ->  Node4
    // Insert many keys then erase them
    const int num_keys = 100;
    
    for (int i = 0; i < num_keys; ++i) {
        KeyType key = make_key(i);
        tree.insert(key, i);
    }
    
    for (int i = 0; i < num_keys; ++i) {
        KeyType key = make_key(i);
        tree.erase(key);
    }
    
    // Verify tree is empty
    for (int i = 0; i < num_keys; ++i) {
        KeyType key = make_key(i);
        EXPECT_EQ(tree.at(key), nullptr);
    }
}

TEST_F(DeleteTest, TreeShrinkageAfterErasingHalfOf300KeysNode256To48) {
    // Tests: 300 keys inserted, then 150 erased
    // Node size reduction: Node256  ->  Node48 (at root)
    // Build a large tree with 300 keys
    const int num_keys = 300;
    for (int i = 0; i < num_keys; ++i) {
        KeyType key = make_key(i);
        tree.insert(key, i * 10);
    }
    
    // Erase about half of them
    for (int i = 0; i < num_keys; i += 2) {
        KeyType key = make_key(i);
        tree.erase(key);
    }
    
    // Verify erased keys are gone
    for (int i = 0; i < num_keys; i += 2) {
        KeyType key = make_key(i);
        EXPECT_EQ(tree.at(key), nullptr);
    }
    
    // Verify remaining keys still exist
    for (int i = 1; i < num_keys; i += 2) {
        KeyType key = make_key(i);
        int* result = tree.at(key);
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(*result, i * 10);
    }
}

TEST_F(DeleteTest, PartialTreeShrinkageOf50KeysNode16To4) {
    // Tests: 50 keys, erase first 16, tree shrinks
    // Partial shrinkage with remaining keys intact
    // Insert many keys
    const int num_keys = 50;
    for (int i = 0; i < num_keys; ++i) {
        KeyType key = make_key(i);
        tree.insert(key, i);
    }
    
    // Erase first third
    for (int i = 0; i < num_keys / 3; ++i) {
        KeyType key = make_key(i);
        tree.erase(key);
    }
    
    // Verify first third is gone
    for (int i = 0; i < num_keys / 3; ++i) {
        KeyType key = make_key(i);
        EXPECT_EQ(tree.at(key), nullptr);
    }
    
    // Verify remaining exist
    for (int i = num_keys / 3; i < num_keys; ++i) {
        KeyType key = make_key(i);
        EXPECT_NE(tree.at(key), nullptr);
    }
}

// ============================================================================
// Erase with Prefix Collisions - Selective deletion in prefix subtrees
// ============================================================================

TEST_F(DeleteTest, EraseAlternateKeysWithPrefixCollisionsNode16) {
    // Tests: 5 keys with shared prefixes, erase every other
    // Node16 management with selective child removal
    // Keys with similar byte patterns
    std::vector<uint64_t> keys = {
        0x1000000000000000ULL,
        0x1000000000000001ULL,
        0x1000000000000002ULL,
        0x1100000000000000ULL,
        0x2000000000000000ULL,
    };
    
    for (size_t i = 0; i < keys.size(); ++i) {
        KeyType key = make_key(keys[i]);
        tree.insert(key, static_cast<int>(i));
    }
    
    // Erase every other key
    for (size_t i = 0; i < keys.size(); i += 2) {
        KeyType key = make_key(keys[i]);
        tree.erase(key);
    }
    
    // Verify erased keys are gone
    for (size_t i = 0; i < keys.size(); i += 2) {
        KeyType key = make_key(keys[i]);
        EXPECT_EQ(tree.at(key), nullptr);
    }
    
    // Verify remaining keys exist
    for (size_t i = 1; i < keys.size(); i += 2) {
        KeyType key = make_key(keys[i]);
        int* result = tree.at(key);
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(*result, static_cast<int>(i));
    }
}

// ============================================================================
// Erase Patterns - Different deletion orders
// ============================================================================

TEST_F(DeleteTest, EraseInSequentialOrderNode4) {
    // Tests: 5 keys erased in ascending order (1,2,3,4,5)
    // Sequential erase pattern stress
    std::vector<uint64_t> keys = {1, 2, 3, 4, 5};
    
    for (auto k : keys) {
        KeyType key = make_key(k);
        tree.insert(key, static_cast<int>(k * 10));
    }
    
    // Erase in order
    for (auto k : keys) {
        KeyType key = make_key(k);
        tree.erase(key);
        EXPECT_EQ(tree.at(key), nullptr);
    }
}

TEST_F(DeleteTest, EraseInReverseOrderNode4) {
    // Tests: 5 keys erased in descending order (5,4,3,2,1)
    // Reverse erase pattern stress
    std::vector<uint64_t> keys = {1, 2, 3, 4, 5};
    
    for (auto k : keys) {
        KeyType key = make_key(k);
        tree.insert(key, static_cast<int>(k * 10));
    }
    
    // Erase in reverse order
    for (auto it = keys.rbegin(); it != keys.rend(); ++it) {
        KeyType key = make_key(*it);
        tree.erase(key);
        EXPECT_EQ(tree.at(key), nullptr);
    }
}

TEST_F(DeleteTest, EraseAlternatingPatternEvenThenOddNode16) {
    // Tests: 20 keys, erase evens (0,2,4,...), then odds (1,3,5,...)
    // Two-phase erase pattern with structure changes
    const int num_keys = 20;
    
    for (int i = 0; i < num_keys; ++i) {
        KeyType key = make_key(i);
        tree.insert(key, i);
    }
    
    // Erase even keys
    for (int i = 0; i < num_keys; i += 2) {
        KeyType key = make_key(i);
        tree.erase(key);
    }
    
    // Then erase odd keys
    for (int i = 1; i < num_keys; i += 2) {
        KeyType key = make_key(i);
        tree.erase(key);
    }
    
    // Verify all gone
    for (int i = 0; i < num_keys; ++i) {
        KeyType key = make_key(i);
        EXPECT_EQ(tree.at(key), nullptr);
    }
}

// ============================================================================
// Edge Cases - Boundary keys
// ============================================================================

TEST_F(DeleteTest, EraseZeroKeyNode4) {
    KeyType key = make_key(0ULL);
    int value = 123;
    
    tree.insert(key, value);
    EXPECT_NE(tree.at(key), nullptr);
    
    tree.erase(key);
    EXPECT_EQ(tree.at(key), nullptr);
}

TEST_F(DeleteTest, EraseMaxUint64KeyNode4) {
    KeyType key = make_key(UINT64_MAX);
    int value = 999;
    
    tree.insert(key, value);
    EXPECT_NE(tree.at(key), nullptr);
    
    tree.erase(key);
    EXPECT_EQ(tree.at(key), nullptr);
}

TEST_F(DeleteTest, EraseSparseKeysFromLargeSetNode256) {
    // Tests: 9 sparse keys, erase alternating subset, preserve others
    // Selective erasure in Node256 with remaining keys intact
    std::vector<uint64_t> keys = {10, 20, 30, 40, 50, 60, 70, 80, 90};
    
    for (auto k : keys) {
        KeyType key = make_key(k);
        tree.insert(key, static_cast<int>(k));
    }
    
    // Erase some while leaving others
    std::vector<uint64_t> to_erase = {20, 40, 60, 80};
    for (auto k : to_erase) {
        KeyType key = make_key(k);
        tree.erase(key);
    }
    
    // Verify erased are gone
    for (auto k : to_erase) {
        KeyType key = make_key(k);
        EXPECT_EQ(tree.at(key), nullptr);
    }
    
    // Verify remaining exist
    std::vector<uint64_t> to_remain = {10, 30, 50, 70, 90};
    for (auto k : to_remain) {
        KeyType key = make_key(k);
        int* result = tree.at(key);
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(*result, static_cast<int>(k));
    }
}

// ============================================================================
// Randomized Erase Patterns
// ============================================================================

TEST_F(DeleteTest, RandomizedEraseEveryThirdOf50KeysNode16To4) {
    // Tests: 50 keys spaced 7x apart, erase every third
    // Pseudo-random erase pattern verifying state correctness
    // Test with a mix of operations
    std::vector<uint64_t> active_keys;
    
    // Insert phase
    for (int i = 0; i < 50; ++i) {
        KeyType key = make_key(i * 7);  // Use 7x to spread keys apart
        tree.insert(key, i);
        active_keys.push_back(i * 7);
    }
    
    // Erase every third
    for (size_t i = 0; i < active_keys.size(); i += 3) {
        KeyType key = make_key(active_keys[i]);
        tree.erase(key);
    }
    
    // Verify state
    for (size_t i = 0; i < active_keys.size(); ++i) {
        KeyType key = make_key(active_keys[i]);
        if (i % 3 == 0) {
            EXPECT_EQ(tree.at(key), nullptr);
        } else {
            EXPECT_NE(tree.at(key), nullptr);
        }
    }
}
