#pragma once

#include "AdaptiveRadixTree.h"
#include "absl/container/btree_map.h"

#include "art/Encoding.h"
#include "Mempool.h"


using KeyType = Encoding<uint64_t>;
using ValueType = uint64_t;
using ArtAllocator = mempool::MempoolAllocator<uint8_t>;
using ArtTree = ART::AdaptiveRadixTree<KeyType, ValueType, ArtAllocator>;

using BtreeValue = std::pair<const uint64_t, uint64_t>;
using BtreeAllocator = mempool::MempoolAllocator<BtreeValue>;
using Btree = absl::btree_map<uint64_t, uint64_t, std::less<uint64_t>, BtreeAllocator>;

struct PoolCounts {
	size_t small;
	size_t medium;
	size_t large;
};

struct Keys {
	std::vector<uint64_t> insert_order;
	std::vector<uint64_t> query_order;
};

inline PoolCounts estimate_pool_counts(size_t count) {
	return {
		.small = (size_t)(count * 1.5) + 1024,
		.medium = count / 40 + 1024,
		.large = count / 200 + 1024,
	};
}

struct BaseFixture {
	mempool::Mempool pool;

	explicit BaseFixture(size_t count)
		: pool(estimate_pool_counts(count).small,
			  estimate_pool_counts(count).medium,
			  estimate_pool_counts(count).large)
		{}
};