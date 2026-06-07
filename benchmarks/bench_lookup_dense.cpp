#include <benchmark/benchmark.h>

#include "util/bench.h"

#include <algorithm>
#include <cstdint>
#include <random>
#include <vector>

namespace {
constexpr uint64_t kDenseSeed = 0x9E3779B97F4A7C15ULL;

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

struct ArtFixture : BaseFixture {
	ArtAllocator allocator;
	ArtTree tree;
	Keys keys;
	std::vector<KeyType> queries;

	explicit ArtFixture(size_t count)
		: BaseFixture(count)
		, allocator(&pool)
		, tree(allocator)
		, keys(build_dense_keys(count))
		, queries() {

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
	Keys keys;
	std::vector<uint64_t> queries;

	explicit BtreeFixture(size_t count)
		: BaseFixture(count)
		, allocator(&pool)
		, tree(std::less<uint64_t>(), allocator)
		, keys(build_dense_keys(count))
		, queries() {

		queries = keys.query_order;
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

