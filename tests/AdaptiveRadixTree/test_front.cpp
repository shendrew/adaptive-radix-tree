#include <gtest/gtest.h>
#include "AdaptiveRadixTree.h"
#include "art/Encoding.h"
#include <vector>

// NOTE: AI generated tests

// Type aliases for convenience
using TreeType = ART::AdaptiveRadixTree<Encoding<uint64_t>, int>;
using KeyType = Encoding<uint64_t>;

class FrontTest : public ::testing::Test {
protected:
    TreeType tree;

    KeyType make_key(uint64_t value) {
        return KeyType(value);
    }
};

// ============================================================================
// Basic Front Tests - Node4
// ============================================================================

TEST_F(FrontTest, FrontReturnsSmallestKeyWhenTreeHasSingleElement) {
    // Tests: Single leaf in Node4
    uint64_t key = 123;

    int value = 99;
    tree.insert(make_key(key), value);
    
    auto result = tree.front();
    EXPECT_TRUE(result);
    EXPECT_EQ(result.value(), value);
}

TEST_F(FrontTest, FrontReturnsSmallestKeyWhenMultipleKeysInNode4) {
    // Tests: Multiple keys in same Node4
    // front() should return the lexicographically smallest key
    std::vector<uint64_t> keys = {10, 20, 30};
    for (auto k : keys) {
        KeyType key = make_key(k);
        tree.insert(key, static_cast<int>(k * 2));
    }
    
    auto result = tree.front();
    EXPECT_TRUE(result);
    
    // front() should return the smallest key (10), so value should be 20
    EXPECT_EQ(result.value(), 20);
}

// ============================================================================
// Front with Various Tree Sizes - Node4, Node16, Node48
// ============================================================================

TEST_F(FrontTest, FrontReturnsSmallestKeyInSmallTreeNode4) {
    // Tests: Small tree with Node4 at root and leaves
    for (int i = 1; i <= 5; ++i) {
        KeyType key = make_key(i);
        tree.insert(key, i * 10);
    }
    
    auto result = tree.front();
    EXPECT_TRUE(result);
    
    // front() should return smallest key (1), so value should be 10
    EXPECT_EQ(result.value(), 10);
}

TEST_F(FrontTest, FrontReturnsSmallestKeyInMediumTreeNode4To16) {
    // Tests: Medium tree with Node4/Node16 transitions
    // At 50 keys, root transitions from Node4 to Node16
    const int num_keys = 50;
    for (int i = 0; i < num_keys; ++i) {
        KeyType key = make_key(i);
        tree.insert(key, i * 5);
    }
    
    auto result = tree.front();
    EXPECT_TRUE(result);
    
    // front() should return smallest key (0), so value should be 0
    EXPECT_EQ(result.value(), 0);
}

TEST_F(FrontTest, FrontReturnsSmallestKeyInLargeTreeNode16To48To256) {
    // Tests: Large tree with Node16 -> Node48 -> Node256 transitions
    // At 300 keys, root traverses through multiple node types
    const int num_keys = 300;
    for (int i = 0; i < num_keys; ++i) {
        KeyType key = make_key(i);
        tree.insert(key, i * 2);
    }
    
    auto result = tree.front();
    EXPECT_TRUE(result);
    
    // front() should return smallest key (0), so value should be 0
    EXPECT_EQ(result.value(), 0);
}

// ============================================================================
// Front with Prefix Collisions - Node16, Node48 (shared prefixes)
// ============================================================================

TEST_F(FrontTest, FrontReturnsSmallestKeyWithPrefixCollisions) {
    // Tests: Keys with common prefixes create Node4 children from Node16
    // Tree structure: Node4(root) -> Node4 children with shared prefix bytes
    std::vector<uint64_t> keys = {
        0x1000000000000000ULL,
        0x1000000000000001ULL,
        0x1000000000000002ULL,
        0x1100000000000000ULL,
        0x2000000000000000ULL,
    };
    
    for (size_t i = 0; i < keys.size(); ++i) {
        KeyType key = make_key(keys[i]);
        tree.insert(key, static_cast<int>(i * 10));
    }
    
    auto result = tree.front();
    EXPECT_TRUE(result);
    
    // front() should return smallest key (0x1000000000000000), which has value 0
    EXPECT_EQ(result.value(), 0);
}

// ============================================================================
// Front After Modifications - Insert, Update, Erase
// ============================================================================

TEST_F(FrontTest, FrontAfterInsertReturnsSmallestKeyNode4) {
    // Tests: Insert operations maintain smallest key invariant
    // Node4 expansion as keys are added
    KeyType key1 = make_key(100);
    tree.insert(key1, 1000);
    
    auto result1 = tree.front();
    EXPECT_TRUE(result1);
    EXPECT_EQ(result1.value(), 1000);
    
    KeyType key2 = make_key(200);
    tree.insert(key2, 2000);
    
    auto result2 = tree.front();
    EXPECT_TRUE(result2);
    // Should still return smallest key (100), so value is 1000
    EXPECT_EQ(result2.value(), 1000);
}

TEST_F(FrontTest, FrontAfterUpdateReturnsSmallestKeyNode4) {
    // Tests: Update operation on smallest key updates returned value
    // Single leaf node remains in Node4
    KeyType key = make_key(50);
    tree.insert(key, 100);
    
    auto result1 = tree.front();
    EXPECT_EQ(result1.value(), 100);
    
    tree.update(key, 200);
    
    auto result2 = tree.front();
    EXPECT_TRUE(result2);
    // After update, the smallest key's value should be 200
    EXPECT_EQ(result2.value(), 200);
}

TEST_F(FrontTest, FrontAfterMultipleUpdatesReturnsSmallestKeyNode4) {
    // Tests: Repeated updates maintain smallest key tracking
    // Updates to smallest key propagate correctly through Node4
    std::vector<uint64_t> keys = {1, 2, 3};
    
    for (auto k : keys) {
        KeyType key = make_key(k);
        tree.insert(key, static_cast<int>(k));
    }
    
    auto result1 = tree.front();
    EXPECT_TRUE(result1);
    // front() returns smallest key (1), so value is 1
    EXPECT_EQ(result1.value(), 1);
    
    // Update all values
    for (auto k : keys) {
        KeyType key = make_key(k);
        tree.update(key, static_cast<int>(k * 10));
    }
    
    auto result2 = tree.front();
    EXPECT_TRUE(result2);
    // front() still returns smallest key (1), now with value 10
    EXPECT_EQ(result2.value(), 10);
}

// ============================================================================
// Front with Erased Elements - Node Shrinkage
// ============================================================================

TEST_F(FrontTest, FrontAfterEraseOneLeavesSmallestKeyNode4) {
    // Tests: Erase smallest leaf, find next smallest
    // Tree shrinkage from Node4 doesn't affect remaining keys
    KeyType key1 = make_key(1);
    KeyType key2 = make_key(2);
    
    tree.insert(key1, 10);
    tree.insert(key2, 20);
    
    tree.erase(key1);
    
    auto result = tree.front();
    EXPECT_TRUE(result);
    EXPECT_EQ(result.value(), 20);
}

TEST_F(FrontTest, FrontAfterEraseMultipleKeysShrinksToSmallestNode4) {
    // Tests: Erase first keys, smallest shifts to next
    // Node4 shrinkage as children are removed
    std::vector<uint64_t> keys = {1, 2, 3, 4, 5};
    
    for (auto k : keys) {
        KeyType key = make_key(k);
        tree.insert(key, static_cast<int>(k * 10));
    }
    
    // Erase first two keys
    tree.erase(make_key(1));
    tree.erase(make_key(2));
    
    auto result = tree.front();
    EXPECT_TRUE(result);
    
    // front() should now return smallest remaining key (3), so value is 30
    EXPECT_EQ(result.value(), 30);
}

TEST_F(FrontTest, FrontAfterPartialEraseFromLargeTreeNode256To16) {
    // Tests: Erase 50 keys from 100, tree shrinks from Node256 -> Node48 -> Node16
    // Returns smallest remaining key after significant shrinkage
    const int num_keys = 100;
    
    for (int i = 0; i < num_keys; ++i) {
        KeyType key = make_key(i);
        tree.insert(key, i * 5);
    }
    
    // Erase half the keys (0 to 49)
    for (int i = 0; i < num_keys / 2; ++i) {
        KeyType key = make_key(i);
        tree.erase(key);
    }
    
    auto result = tree.front();
    EXPECT_TRUE(result);
    
    // front() should return smallest remaining key (50), so value is 250
    EXPECT_EQ(result.value(), (num_keys / 2) * 5);
}

// ============================================================================
// Front with Edge Cases
// ============================================================================

TEST_F(FrontTest, FrontReturnsSmallestKeyWithZeroKeyNode4) {
    KeyType key = make_key(0ULL);
    int value = 123;
    
    tree.insert(key, value);
    
    auto result = tree.front();
    EXPECT_TRUE(result);
    EXPECT_EQ(result.value(), value);
}

TEST_F(FrontTest, FrontReturnsSmallestKeyWithMaxKeyNode4) {
    KeyType key = make_key(UINT64_MAX);
    int value = 999;
    
    tree.insert(key, value);
    
    auto result = tree.front();
    EXPECT_TRUE(result);
    EXPECT_EQ(result.value(), value);
}

TEST_F(FrontTest, FrontReturnsSmallestKeyWithNegativeValueNode4) {
    KeyType key = make_key(42);
    int value = -100;
    
    tree.insert(key, value);
    
    auto result = tree.front();
    EXPECT_TRUE(result);
    EXPECT_EQ(result.value(), value);
}

TEST_F(FrontTest, FrontReturnsSmallestKeyWithZeroValueNode4) {
    KeyType key = make_key(999);
    int value = 0;
    
    tree.insert(key, value);
    
    auto result = tree.front();
    EXPECT_TRUE(result);
    EXPECT_EQ(result.value(), 0);
}

// ============================================================================
// Complex Sequences: Insert -> Update -> Erase
// ============================================================================

TEST_F(FrontTest, FrontAfterInsertUpdateEraseSequenceShrinksToSmallest) {
    // Tests: Insert 5 keys  ->  Update even indices  ->  Erase even indices
    // Verifies smallest key tracking through all operations and shrinkage
    std::vector<uint64_t> base_keys = {1, 10, 100, 1000, 10000};
    
    // Insert phase
    for (auto k : base_keys) {
        KeyType key = make_key(k);
        tree.insert(key, static_cast<int>(k));
    }
    
    auto result1 = tree.front();
    EXPECT_TRUE(result1);
    // Smallest key is 1
    EXPECT_EQ(result1.value(), 1);
    
    // Update phase
    for (size_t i = 0; i < base_keys.size(); i += 2) {
        KeyType key = make_key(base_keys[i]);
        tree.update(key, static_cast<int>(base_keys[i] * 2));
    }
    
    auto result2 = tree.front();
    EXPECT_TRUE(result2);
    // Smallest key is still 1 (updated to 2)
    EXPECT_EQ(result2.value(), 2);
    
    // Erase phase
    for (size_t i = 0; i < base_keys.size(); i += 2) {
        KeyType key = make_key(base_keys[i]);
        tree.erase(key);
    }
    
    auto result3 = tree.front();
    EXPECT_TRUE(result3);
    
    // After erasing 1, 100, 10000, smallest remaining is 10
    EXPECT_EQ(result3.value(), 10);
}

TEST_F(FrontTest, FrontAlwaysReturnsSmallestKeyConsistentlyNode4) {
    // Tests: Multiple calls to front() return same smallest key
    // Verifies no state corruption or random behavior
    // Insert several values and verify front always returns the lexicographically smallest
    std::vector<uint64_t> keys = {5, 15, 25, 35, 45};
    
    for (auto k : keys) {
        KeyType key = make_key(k);
        int value = static_cast<int>(k * k);
        tree.insert(key, value);
    }
    
    // Call front multiple times - should always return the smallest key (5)
    for (int i = 0; i < 5; ++i) {
        auto result = tree.front();
        EXPECT_TRUE(result);
        // front() should consistently return smallest key (5) with value 25
        EXPECT_EQ(result.value(), 25);
    }
}
