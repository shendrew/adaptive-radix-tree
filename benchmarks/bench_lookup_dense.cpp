#include <benchmark/benchmark.h>

#include "AdaptiveRadixTree.h"
#include "art/Encoding.h"
#include "Mempool.h"
#include "btree_map.h"

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

namespace {
using KeyType = Encoding<uint64_t>;
using ValueType = uint64_t;
using ArtAllocator = mempool::MempoolAllocator<uint8_t>;
using ArtTree = ART::AdaptiveRadixTree<KeyType, ValueType, ArtAllocator>;

using BtreeValue = std::pair<const uint64_t, uint64_t>;
using BtreeAllocator = mempool::MempoolAllocator<BtreeValue>;
using Btree = btree::btree_map<uint64_t, uint64_t, std::less<uint64_t>, BtreeAllocator>;

constexpr uint64_t kDenseSeed = 0x9E3779B97F4A7C15ULL;


struct PoolCounts {
	size_t small;
	size_t medium;
	size_t large;
};

struct Keys {
	std::vector<uint64_t> insert_order;
	std::vector<uint64_t> query_order;
};

PoolCounts estimate_pool_counts(size_t count) {
	return {
		.small = (size_t)(count * 1.5) + 1024,
		.medium = count / 40 + 1024,
		.large = count / 200 + 1024,
	};
}

Keys build_dense_keys(size_t count) {
	Keys keys;
	keys.insert_order.resize(count);
	std::iota(keys.insert_order.begin(), keys.insert_order.end(), 1ULL);

	std::mt19937_64 rng(kDenseSeed);
	std::shuffle(keys.insert_order.begin(), keys.insert_order.end(), rng);

	keys.query_order = keys.insert_order;
	std::shuffle(keys.query_order.begin(), keys.query_order.end(), rng);
	return keys;
}

struct BaseFixture {
	mempool::Mempool pool;
	Keys keys;

	explicit BaseFixture(size_t count)
		: pool(estimate_pool_counts(count).small,
			  estimate_pool_counts(count).medium,
			  estimate_pool_counts(count).large)
		, keys(build_dense_keys(count)) {}
};

struct ArtFixture : BaseFixture {
	ArtAllocator allocator;
	ArtTree tree;
	std::vector<KeyType> queries;

	explicit ArtFixture(size_t count)
		: BaseFixture(count)
		, allocator(&pool)
		, tree(allocator) {
		queries.reserve(keys.query_order.size());
		for (uint64_t value : keys.insert_order) {
			KeyType key(value);
			tree.insert(key, value);
		}

		for (uint64_t value : keys.query_order) {
			queries.emplace_back(value);
		}
	}
};

struct BtreeFixture : BaseFixture {
	BtreeAllocator allocator;
	Btree tree;
	std::vector<uint64_t> queries;

	explicit BtreeFixture(size_t count)
		: BaseFixture(count)
		, allocator(&pool)
		, tree(std::less<uint64_t>(), allocator)
		, queries(keys.query_order) {
		for (uint64_t value : keys.insert_order) {
			tree.insert({value, value});
		}
	}
};
}  // namespace

static void BenchLookupDenseART(benchmark::State& state) {
	const size_t count = static_cast<size_t>(state.range(0));

	ArtFixture fixture(count);

	const int64_t batch = static_cast<int64_t>(fixture.queries.size());
	while (state.KeepRunningBatch(batch)) {
		for (const auto& key : fixture.queries) {
			auto* value = fixture.tree.at(key);
			benchmark::DoNotOptimize(value);
		}
	}

	state.SetLabel("lookups");
	state.counters["lookups"] =
		benchmark::Counter(static_cast<double>(state.iterations()),
					   benchmark::Counter::kIsRate);
}

static void BenchLookupDenseBtree(benchmark::State& state) {
	const size_t count = static_cast<size_t>(state.range(0));

	BtreeFixture fixture(count);

	const int64_t batch = static_cast<int64_t>(fixture.queries.size());
	while (state.KeepRunningBatch(batch)) {
		for (uint64_t key : fixture.queries) {
			auto it = fixture.tree.find(key);
			benchmark::DoNotOptimize(it);
			if (it != fixture.tree.end()) {
				benchmark::DoNotOptimize(it->second);
			}
		}
	}

	state.SetLabel("lookups");
	state.counters["lookups"] =
		benchmark::Counter(static_cast<double>(state.iterations()),
					   benchmark::Counter::kIsRate);
}

BENCHMARK(BenchLookupDenseART)
	->Arg(64 * 1024)            // 64K
	->Arg(1 * 1024 * 1024)      // 1M
	->Arg(16 * 1024 * 1024)     // 16M
	// ->Arg(256 * 1024 * 1024)    // 256M
	->UseRealTime();

BENCHMARK(BenchLookupDenseBtree)
	->Arg(64 * 1024)            // 64K
	->Arg(1 * 1024 * 1024)      // 1M
	->Arg(16 * 1024 * 1024)     // 16M
	// ->Arg(256 * 1024 * 1024)    // 256M
	->UseRealTime();

