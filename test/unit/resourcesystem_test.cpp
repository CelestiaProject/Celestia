#include <atomic>
#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <celengine/asyncresourcecache.h>
#include <celengine/resourcesystem.h>

#include <doctest.h>

using celestia::engine::AsyncResourceCache;
using celestia::engine::ResourceSystem;
using celestia::engine::ResourceCacheBase;

namespace
{

enum class TestHandle : std::uint32_t
{
    A       = 1,
    B       = 2,
    C       = 3,
    D       = 4,
    Invalid = 0xFFFFFFFFu,
};

struct TestInfo
{
    int  value{ 0 };
    bool failDecode{ false };
};

struct TestGpuResource
{
    int         value{ 0 };
    std::size_t bytes{ 0 };
    // Bumped on destruction so tests can verify ~unique_ptr<> actually fired.
    std::shared_ptr<std::atomic<int>> destroyCounter;

    ~TestGpuResource()
    {
        if (destroyCounter)
            ++(*destroyCounter);
    }
};

// Shared mutable state for the trait. Held via shared_ptr so a trait copy
// inside the cache observes the same counters/maps as the test.
struct TestState
{
    std::unordered_map<TestHandle, TestInfo> infos;
    std::atomic<int> decodeCount{ 0 };
    std::atomic<int> uploadCount{ 0 };
    std::shared_ptr<std::atomic<int>> destroyCount{
        std::make_shared<std::atomic<int>>(0) };
    std::size_t      bytesPerResource{ 1024 };
    TestGpuResource* placeholder{ nullptr };

    // Optional delay inside decode() to widen race windows during testing.
    std::chrono::microseconds decodeDelay{ 0 };
};

struct TestTraits
{
    using Handle      = TestHandle;
    using Info        = TestInfo;
    using CpuData     = int;
    using GpuResource = TestGpuResource;

    std::shared_ptr<TestState> state;

    bool getInfo(Handle h, Info& out) const
    {
        auto it = state->infos.find(h);
        if (it == state->infos.end())
            return false;
        out = it->second;
        return true;
    }

    std::optional<CpuData> decode(const Info& info) const
    {
        ++state->decodeCount;
        if (state->decodeDelay.count() > 0)
            std::this_thread::sleep_for(state->decodeDelay);
        if (info.failDecode)
            return std::nullopt;
        return info.value;
    }

    std::unique_ptr<GpuResource> upload(CpuData&& data) const
    {
        ++state->uploadCount;
        auto r = std::make_unique<GpuResource>();
        r->value          = data;
        r->bytes          = state->bytesPerResource;
        r->destroyCounter = state->destroyCount;
        return r;
    }

    std::size_t gpuBytes(const GpuResource& r) const { return r.bytes; }

    GpuResource* placeholder() const noexcept { return state->placeholder; }
};

// Wait until predicate is true or timeout elapses. Returns true on success.
template <typename Pred>
bool waitFor(Pred&& pred,
             std::chrono::milliseconds timeout = std::chrono::seconds(5))
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!pred())
    {
        if (std::chrono::steady_clock::now() >= deadline)
            return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}

// Run one frame: beginFrame, wait for the expected number of pending ready
// items, then drain. Returns bytes uploaded by the drain.
std::size_t runFrame(ResourceSystem& sys,
                     AsyncResourceCache<TestTraits>& cache,
                     std::size_t expectedReady)
{
    sys.beginFrame();
    REQUIRE(waitFor([&]{ return cache.pendingReady() >= expectedReady; }));
    std::size_t uploaded = 0;
    auto* base = static_cast<ResourceCacheBase*>(&cache);
    uploaded += base->drainReady(sys.uploadBudgetBytes());
    return uploaded;
}

} // namespace

// ---------------------------------------------------------------------------
// ResourceSystem
// ---------------------------------------------------------------------------

TEST_CASE("ResourceSystem worker count")
{
    SUBCASE("explicit count is honoured")
    {
        ResourceSystem sys(3);
        CHECK(sys.workerCount() == 3);
    }
    SUBCASE("auto-detect produces at least one worker")
    {
        ResourceSystem sys;
        CHECK(sys.workerCount() >= 1);
    }
}

TEST_CASE("ResourceSystem runs submitted tasks")
{
    ResourceSystem sys(1);
    std::atomic<int> ran{ 0 };
    sys.submit([&]{ ++ran; });
    sys.submit([&]{ ++ran; });
    sys.submit([&]{ ++ran; });
    REQUIRE(waitFor([&]{ return ran.load() == 3; }));
}

TEST_CASE("ResourceSystem runs tasks in submission order (FIFO)")
{
    ResourceSystem sys(1); // single worker => deterministic ordering
    std::promise<void> gate;
    std::shared_future<void> gateFut = gate.get_future();

    std::vector<int> order;
    std::mutex orderMu;
    auto record = [&](int id) {
        return [&, id]{
            std::lock_guard<std::mutex> lock(orderMu);
            order.push_back(id);
        };
    };

    // Blocker keeps the worker busy until we release `gate` so all the
    // tasks are queued before any can run.
    sys.submit([gateFut]{ gateFut.wait(); });

    sys.submit(record(1));
    sys.submit(record(5));
    sys.submit(record(3));
    sys.submit(record(50));

    gate.set_value();

    REQUIRE(waitFor([&]{
        std::lock_guard<std::mutex> lock(orderMu);
        return order.size() == 4;
    }));

    std::lock_guard<std::mutex> lock(orderMu);
    // Tasks run strictly in the order they were submitted.
    CHECK(order == std::vector<int>{1, 5, 3, 50});
}

TEST_CASE("ResourceSystem joins workers cleanly on destruction")
{
    // A worker blocked on a task that never returns until we release it.
    auto gate = std::make_shared<std::promise<void>>();
    auto fut  = gate->get_future().share();

    {
        ResourceSystem sys(2);
        sys.submit([fut]{ fut.wait(); });
        // Release before ResourceSystem destructor so the worker can exit.
        gate->set_value();
    }
    // If we get here without hanging, join succeeded.
    CHECK(true);
}

TEST_CASE("ResourceSystem beginFrame increments counter")
{
    ResourceSystem sys(1);
    const std::uint64_t start = sys.currentFrame();
    sys.beginFrame();
    sys.beginFrame();
    sys.beginFrame();
    CHECK(sys.currentFrame() == start + 3);
}

// ---------------------------------------------------------------------------
// AsyncResourceCache
// ---------------------------------------------------------------------------

TEST_CASE("AsyncResourceCache loads asynchronously and serves via find()")
{
    ResourceSystem sys(1);
    auto state = std::make_shared<TestState>();
    state->infos[TestHandle::A] = { 42, false };

    TestGpuResource placeholder{ -1, 0 };
    state->placeholder = &placeholder;

    AsyncResourceCache<TestTraits> cache(sys, TestTraits{ state });

    sys.beginFrame();
    auto* first = cache.find(TestHandle::A);
    CHECK(first == &placeholder); // placeholder while loading

    runFrame(sys, cache, 1);

    auto* loaded = cache.find(TestHandle::A);
    REQUIRE(loaded != nullptr);
    CHECK(loaded != &placeholder);
    CHECK(loaded->value == 42);
    CHECK(loaded->bytes == state->bytesPerResource);
    CHECK(state->decodeCount.load() == 1);
    CHECK(state->uploadCount.load() == 1);
}

TEST_CASE("AsyncResourceCache returns nullptr for Invalid handle")
{
    ResourceSystem sys(1);
    auto state = std::make_shared<TestState>();
    AsyncResourceCache<TestTraits> cache(sys, TestTraits{ state });

    CHECK(cache.find(TestHandle::Invalid) == nullptr);
    CHECK(cache.size() == 0);
}

TEST_CASE("AsyncResourceCache reports Failed for missing info / failed decode")
{
    ResourceSystem sys(1);
    auto state = std::make_shared<TestState>();
    // Handle A has no info entry => getInfo fails.
    // Handle B has info but decode is marked to fail.
    state->infos[TestHandle::B] = { 0, true };

    AsyncResourceCache<TestTraits> cache(sys, TestTraits{ state });

    sys.beginFrame();
    CHECK(cache.find(TestHandle::A) == nullptr);
    CHECK(cache.find(TestHandle::B) == nullptr); // no placeholder configured

    // Decode is scheduled for B; wait for the ready queue then drain.
    runFrame(sys, cache, 1);

    // Both A and B should be Failed; another find() should observe that
    // (still returns nullptr placeholder, doesn't crash, doesn't reschedule
    // until grace passes).
    CHECK(cache.find(TestHandle::A) == nullptr);
    CHECK(cache.find(TestHandle::B) == nullptr);
    CHECK(state->decodeCount.load() == 1); // only B made it to a worker
    CHECK(state->uploadCount.load() == 0);
}

TEST_CASE("AsyncResourceCache purgeStale evicts old entries, keeps recent")
{
    ResourceSystem sys(1);
    auto state = std::make_shared<TestState>();
    state->infos[TestHandle::A] = { 10, false };
    state->infos[TestHandle::B] = { 20, false };

    AsyncResourceCache<TestTraits> cache(sys, TestTraits{ state });

    sys.beginFrame();
    cache.find(TestHandle::A);
    cache.find(TestHandle::B);
    runFrame(sys, cache, 2);

    // Bump frame counter past the grace window, but only re-touch A.
    constexpr std::uint64_t grace = 5;
    for (std::uint64_t i = 0; i < grace + 1; ++i)
    {
        sys.beginFrame();
        cache.find(TestHandle::A);
    }

    CHECK(cache.size() == 2);
    cache.purgeStale(grace);
    CHECK(cache.size() == 1);
    // B's resource must have been destroyed (proves the unique_ptr fired).
    CHECK(state->destroyCount->load() == 1);

    // Re-find on the evicted handle re-enqueues a decode.
    sys.beginFrame();
    CHECK(cache.find(TestHandle::B) == nullptr); // placeholder is nullptr here
    runFrame(sys, cache, 1);
    auto* reloaded = cache.find(TestHandle::B);
    REQUIRE(reloaded != nullptr);
    CHECK(reloaded->value == 20);
    CHECK(state->decodeCount.load() == 3);  // A once, B twice
}

TEST_CASE("AsyncResourceCache pin prevents eviction")
{
    ResourceSystem sys(1);
    auto state = std::make_shared<TestState>();
    state->infos[TestHandle::A] = { 7, false };

    AsyncResourceCache<TestTraits> cache(sys, TestTraits{ state });

    sys.beginFrame();
    cache.find(TestHandle::A);
    runFrame(sys, cache, 1);

    cache.pin(TestHandle::A);

    // Advance frames well past grace without touching A.
    for (int i = 0; i < 20; ++i)
        sys.beginFrame();

    cache.purgeStale(5);
    CHECK(cache.size() == 1);
    CHECK(state->destroyCount->load() == 0);

    cache.unpin(TestHandle::A);
    cache.purgeStale(5);
    CHECK(cache.size() == 0);
    CHECK(state->destroyCount->load() == 1);
}

TEST_CASE("AsyncResourceCache drainReady respects byte budget")
{
    ResourceSystem sys(1);
    auto state = std::make_shared<TestState>();
    state->bytesPerResource = 100;
    for (auto h : { TestHandle::A, TestHandle::B, TestHandle::C, TestHandle::D })
        state->infos[h] = { static_cast<int>(h), false };

    AsyncResourceCache<TestTraits> cache(sys, TestTraits{ state });

    sys.beginFrame();
    for (auto h : { TestHandle::A, TestHandle::B, TestHandle::C, TestHandle::D })
        cache.find(h);

    REQUIRE(waitFor([&]{ return cache.pendingReady() == 4; }));

    // Budget of 250 bytes should upload at most 3 entries (300 > 250 stops it).
    auto* base = static_cast<ResourceCacheBase*>(&cache);
    const std::size_t uploaded = base->drainReady(250);
    CHECK(uploaded >= 250);          // at least cleared budget
    CHECK(uploaded <= 300);          // didn't go too far
    CHECK(state->uploadCount.load() == 3);
    CHECK(cache.pendingReady() == 1);

    // Next frame drains the remaining one.
    base->drainReady(sys.uploadBudgetBytes());
    CHECK(state->uploadCount.load() == 4);
    CHECK(cache.pendingReady() == 0);
}

TEST_CASE("ResourceSystem::drainCaches routes to registered caches")
{
    ResourceSystem sys(1);
    auto state = std::make_shared<TestState>();
    state->infos[TestHandle::A] = { 99, false };

    AsyncResourceCache<TestTraits> cache(sys, TestTraits{ state });

    sys.beginFrame();
    cache.find(TestHandle::A);
    REQUIRE(waitFor([&]{ return cache.pendingReady() == 1; }));

    sys.drainCaches();
    CHECK(state->uploadCount.load() == 1);
    CHECK(cache.pendingReady() == 0);
}

TEST_CASE("ResourceSystem::purgeIfDue fires only at the configured interval")
{
    ResourceSystem sys(1);
    sys.setPurgeIntervalFrames(4);
    sys.setGraceFrames(1);

    auto state = std::make_shared<TestState>();
    state->infos[TestHandle::A] = { 1, false };

    AsyncResourceCache<TestTraits> cache(sys, TestTraits{ state });

    sys.beginFrame();
    cache.find(TestHandle::A);
    runFrame(sys, cache, 1);
    // After runFrame, frameCount=2. cache has A at lastUsed=2.

    sys.beginFrame(); // frame 3
    sys.purgeIfDue(); // 3 % 4 != 0 -> no-op
    CHECK(cache.size() == 1);

    sys.beginFrame(); // frame 4
    sys.purgeIfDue(); // 4 % 4 == 0; grace=1; 4-2 > 1 -> evict
    CHECK(cache.size() == 0);
}
