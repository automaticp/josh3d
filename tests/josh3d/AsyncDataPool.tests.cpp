#include "AsyncDataPool.hpp"
#include "ThreadPool.hpp"
#include <doctest/doctest.h>
#include <limits>
#include <range/v3/algorithm/all_of.hpp>
#include <range/v3/algorithm/any_of.hpp>
#include <range/v3/all.hpp>
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <functional>
#include <iterator>
#include <numeric>
#include <optional>
#include <random>
#include <range/v3/iterator/operations.hpp>
#include <range/v3/numeric/accumulate.hpp>
#include <range/v3/utility/box.hpp>
#include <range/v3/view/generate.hpp>
#include <range/v3/view/generate_n.hpp>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <vector>


using namespace josh;
// It takes ELEVEN SECONDS to compile this translation unit
// THANK YOU, RANGES
namespace views = ranges::views;

static std::string random_string(size_t min_size, size_t max_size) {
    thread_local std::mt19937 rng{ std::random_device{}() };
    thread_local std::uniform_int_distribution<char> char_dist{ '0', 'z' };

    std::uniform_int_distribution<size_t> size_dist{ min_size, max_size };

    const size_t size = size_dist(rng);

    return views::generate_n([] { return char_dist(rng); }, size) | ranges::to<std::string>();
}


template<typename U, typename Proj>
auto map(const std::vector<U>& from, Proj&& proj) {
    using result_t = std::remove_cvref_t<std::invoke_result_t<Proj, const U&>>;
    return from | views::transform(proj) | ranges::to<std::vector<result_t>>();
};

template<typename U, typename Proj>
auto map(std::vector<U>& from, Proj&& proj) {
    using result_t = std::remove_cvref_t<std::invoke_result_t<Proj, U&>>;
    return from | views::transform(proj) | ranges::to<std::vector<result_t>>();
};



template<typename ResourceT>
static ResourceT make_expected(const std::string& path);




struct TestResourceHashed {
    size_t value;
    bool operator==(const TestResourceHashed& other) const noexcept { return value == other.value; }
};

template<>
auto make_expected<TestResourceHashed>(const std::string& path) -> TestResourceHashed {
    return { std::hash<std::string>{}(path) };
}

template<>
Shared<TestResourceHashed> AsyncDataPool<TestResourceHashed>::load_data_from(const std::string& path) {
    return std::make_shared<TestResourceHashed>(make_expected<TestResourceHashed>(path));
}




struct TestResourceHashedSleepy {
    size_t value;
    bool operator==(const TestResourceHashed& other) const noexcept { return value == other.value; }
};

template<>
auto make_expected<TestResourceHashedSleepy>(const std::string& path) -> TestResourceHashedSleepy {
    return { std::hash<std::string>{}(path) };
}

template<>
Shared<TestResourceHashedSleepy> AsyncDataPool<TestResourceHashedSleepy>::load_data_from(const std::string& path) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return std::make_shared<TestResourceHashedSleepy>(make_expected<TestResourceHashedSleepy>(path));
}




struct TestException { std::string what; };
struct TestResourceThrowing {};

template<>
auto make_expected<TestResourceThrowing>(const std::string&) -> TestResourceThrowing {
    return {};
}

template<>
Shared<TestResourceThrowing> AsyncDataPool<TestResourceThrowing>::load_data_from(const std::string& path) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    throw TestException{ path };
    return std::make_shared<TestResourceThrowing>(make_expected<TestResourceThrowing>(path));
}






TEST_SUITE("AsyncDataPool") {



TEST_CASE_TEMPLATE("Loading correctness", T, TestResourceHashed, TestResourceHashedSleepy) {
    ThreadPool thread_pool{};
    AsyncDataPool<T> data_pool{ thread_pool };

    SUBCASE("Loading of a single resource") {
        const std::string path = random_string(1, 64);
        const T expected = make_expected<T>(path);

        SUBCASE("Try load from cache when not in cache") {
            std::optional<Shared<T>> opt_res = data_pool.try_load_from_cache(path);
            CHECK(!opt_res.has_value());
        }

        SUBCASE("Load async if not in cache") {
            std::optional<Shared<T>> opt_res = data_pool.try_load_from_cache(path);
            CHECK(!opt_res.has_value());
            Shared<T> res = data_pool.load_async(path).get();
            REQUIRE(res != nullptr);
            CHECK(res->value == expected.value);
        }

        SUBCASE("Load async, wait until avaliable, then try load from cache") {
            // This subcase is mostly here as a form of documentation of what
            // can happen when you don't read the documentation.

            data_pool.load_async(path).wait();

            // This might fail, the loading thread is still running,
            // and might have not yet emplaced a resource or is holding a write lock.
            std::optional<Shared<T>> opt_res = data_pool.try_load_from_cache(path);

            INFO("try_load_from_cache() succeded: " << opt_res.has_value() << ". "
                << "The value is not guaranteed to be avaliable right after load_async() succeeds.");
            if (opt_res.has_value()) {
                CHECK(opt_res.value()->value == expected.value);
                CHECK(opt_res.value().use_count() == 2); // Me and pool
            }
        }

        SUBCASE("Load async then immediately try to load from cache; do not wait") {
            auto future = data_pool.load_async(path);
            std::optional<Shared<T>> opt_res = data_pool.try_load_from_cache(path);
            size_t n_attempts{ 1 };
            while (!opt_res.has_value()) {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                opt_res = data_pool.try_load_from_cache(path);
                ++n_attempts;
            }
            INFO("Retrieved the resource in " << n_attempts << " attempt(s)");
            Shared<T> first_copy = std::move(opt_res.value());
            Shared<T> second_copy = future.get();
            REQUIRE(first_copy != nullptr);
            REQUIRE(second_copy != nullptr);
            CHECK(first_copy.get() == second_copy.get());
            CHECK(first_copy->value == expected.value);
            CHECK(first_copy.use_count() == 3);
        }

        SUBCASE("Load async the same resource multiple times") {
            // This can easily crash the program if AsyncDataPool goes out of scope early.
            // If AsyncDataPool can be destroyed before all of the loading threads are complete
            // then this will UB on trying to emplace/cache a Shared<T> into the pool after it's
            // destroyed. Waiting on futures returned from AsyncDataPool is not enough
            // as they are fulfilled before caching happens.
            std::vector<std::future<Shared<T>>> futures = views::repeat(path)
                | views::take(1024)
                | views::transform([&](const std::string& path) { return data_pool.load_async(path); })
                | ranges::to<std::vector>();

            auto results = futures
                | views::transform([](std::future<Shared<T>>& f) { return f.get(); })
                | ranges::to<std::vector>();

            REQUIRE(ranges::all_of(results, [](auto& elem) { return elem != nullptr; }));
            CHECK(ranges::all_of(results, [&](auto& elem) { return elem.get() == results[0].get(); }));
            CHECK(results[0].use_count() == results.size() + 1);
            // Adding a delay before destruction will probably make the test pass.
            // std::this_thread::sleep_for(std::chrono::seconds(1));
            // DO NOT DO THIS.
            // AsyncDataPool should instead properly synchronize on destruction until
            // all of the loading threads are complete.
        }

    }

    SUBCASE("Loading of multiple resources") {
        auto paths = views::generate_n([] { return random_string(1, 64); }, 1024)
            | ranges::to<std::vector<std::string>>();

        std::vector<T> expected = map(paths, &make_expected<T>);

        SUBCASE("Try load from cache when there is no resource") {
            std::vector<std::optional<Shared<T>>> fails =
                map(paths, [&](const std::string& path) { return data_pool.try_load_from_cache(path); });
            CHECK(ranges::none_of(fails, [](auto& elem) { return elem.has_value(); }));
            // Do something more useful next time
        }

        SUBCASE("Load async all at once, wait until completion") {
            std::vector<std::future<Shared<T>>> futures =
                map(paths, [&](const std::string& path) { return data_pool.load_async(path); });

            std::vector<Shared<T>> results =
                map(futures, [](std::future<Shared<T>>& future) { return future.get(); });

            REQUIRE(ranges::all_of(results, [](auto& elem) { return elem != nullptr; }));
            CHECK(ranges::equal(results, expected, {}, [](Shared<T>& e) { return e->value; }, &T::value));
        }


        SUBCASE("Load async all at once then immediately try to load from cache") {
            std::vector<std::future<Shared<T>>> futures =
                map(paths, [&](const std::string& path) { return data_pool.load_async(path); });

            std::vector<Shared<T>> results =
                map(futures, [](std::future<Shared<T>>& future) { return future.get(); });

            std::vector<std::optional<Shared<T>>> opt_results =
                map(paths, [&](const std::string& path) { return data_pool.try_load_from_cache(path); });

            // No guarantee that the opt_results will contain any values.
            // If any do, compare against expected.

            auto r = views::zip(results, opt_results)
                | views::filter([](const auto& tuple) { return ranges::get<1>(tuple).has_value(); });

            CHECK_MESSAGE(
                ranges::all_of(r, [](const auto& tuple) {
                    return ranges::get<0>(tuple).get() == ranges::get<1>(tuple).value().get();
                }),
                "Number of try_load_from_cache() that succeded: " << ranges::distance(r) << "/" << paths.size()
            );
        }
    }


}

TEST_CASE("Loading from multiple threads") {
    // TODO
}


REGISTER_EXCEPTION_TRANSLATOR(TestException& ex) {
    return doctest::String(ex.what.c_str());
}


TEST_CASE("Exception propagation") {
    using T = TestResourceThrowing;
    ThreadPool thread_pool{};
    AsyncDataPool<T> data_pool{ thread_pool };

    // WARN: Exception propagation tests can trigger false positives from
    // ThreadSanitizer because of noninstrumented parts of STL.
    // A data race warning in the destructor/release of std::exception_ptr
    // is known and should be treated as a false positive.
    //
    // Similar to: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=92731
    //
    // Warnings from noninstrumented libs can be disabled through
    // an environment variable:
    // TSAN_OPTIONS="ignore_noninstrumented_modules=1"
    // Note that this can also suppress actual data races.

    // TODO: Make a more accurate suppression rule for TSan that targets
    // only this particular issue.

    // TODO: Rebuild GCC with TSan to check that I'm not an idiot
    // and this is actually a false positive.

    SUBCASE("Single resource load failure") {
        const std::string path = random_string(1, 64);

        SUBCASE("Load async once; expect to throw") {
            auto future = data_pool.load_async(path);

            CHECK_THROWS_WITH_AS(future.get(), path.c_str(), TestException);
        }

        SUBCASE("Load async multiple times; expect all to throw") {
            auto futures = views::generate_n([&] { return data_pool.load_async(path); }, 6)
                | ranges::to<std::vector>();

            for (auto& future : futures) {
                CHECK_THROWS_WITH_AS(future.get(), path.c_str(), TestException);
            }
        }
    }

    SUBCASE("Multiple resource load failures") {
        auto paths = views::generate_n([] { return random_string(1, 64); }, 6)
            | ranges::to<std::vector<std::string>>();

        SUBCASE("Load all async; expect to throw") {
            auto futures = map(paths, [&](const auto& path) { return data_pool.load_async(path); });

            for (auto [future, path] : views::zip(futures, paths)) {
                CHECK_THROWS_WITH_AS(future.get(), path.c_str(), TestException);
            }
        }

        SUBCASE("Load all async; expect to throw; expect none to be cached") {
            auto futures = map(paths, [&](const auto& path) { return data_pool.load_async(path); });

            for (auto [future, path] : views::zip(futures, paths)) {
                CHECK_THROWS_WITH_AS(future.get(), path.c_str(), TestException);
            }

            // WARN: This part of the test is flaky. There's no way to synchronize
            // the calling thread with completion of the loading thread in the current
            // interface of AsyncDataPool. But it's very unrealistic to take more
            // than 100 ms for a simple task of removing the entry from the pool.

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            auto opt_results = map(paths, [&](const auto& path) { return data_pool.try_load_from_cache(path); });

            CHECK(ranges::none_of(opt_results, [](auto& elem) { return elem.has_value(); }));
        }

    }
}


TEST_CASE("Stress testing") {
    // TODO
}

}
