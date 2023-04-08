#include "AsyncDataPool.hpp"
#include "ThreadPool.hpp"
#include <doctest/doctest.h>
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
#include <range/v3/numeric/accumulate.hpp>
#include <range/v3/view/generate.hpp>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>


using namespace learn;
namespace views = ranges::views;

static std::string random_string(size_t min_size, size_t max_size) {
    thread_local std::mt19937 rng{ std::random_device{}() };
    thread_local std::uniform_int_distribution<char> char_dist{};

    std::uniform_int_distribution<size_t> size_dist{ min_size, max_size };

    const size_t size = size_dist(rng);

    std::string result;
    result.reserve(size);
    std::generate_n(std::back_inserter(result), size, [] { return char_dist(rng); });

    return result;
}


template<typename U, typename Proj>
auto map(const std::vector<U>& from, Proj&& proj) {
    using result_t = std::remove_cvref_t<std::invoke_result_t<Proj, const U&>>;
    return from | views::transform(proj) | ranges::to<std::vector<result_t>>();
    // std::vector<result_t> results;
    // results.reserve(from.size());
    // for (const U& element : from) { results.emplace_back(std::forward<Proj>(proj)(element)); }
    // return results;
};

template<typename U, typename Proj>
auto map(std::vector<U>& from, Proj&& proj) {
    using result_t = std::remove_cvref_t<std::invoke_result_t<Proj, U&>>;
    return from | views::transform(proj) | ranges::to<std::vector<result_t>>();
    // std::vector<result_t> results;
    // results.reserve(from.size());
    // for (U& element : from) { results.emplace_back(std::forward<Proj>(proj)(element)); }
    // return results;
};




struct TestResourceHashed {
    size_t value;
    bool operator==(const TestResourceHashed& other) const noexcept { return value == other.value; }
};

struct TestResourceHashedSleepy {
    size_t value;
    bool operator==(const TestResourceHashed& other) const noexcept { return value == other.value; }
};

template<typename ResourceT>
static ResourceT make_expected(const std::string& path);

template<>
auto make_expected<TestResourceHashed>(const std::string& path) -> TestResourceHashed {
    return { std::hash<std::string>{}(path) };
}

template<>
auto make_expected<TestResourceHashedSleepy>(const std::string& path) -> TestResourceHashedSleepy {
    return { std::hash<std::string>{}(path) };
}


template<>
Shared<TestResourceHashed> AsyncDataPool<TestResourceHashed>::load_data_from(const std::string& path) {
    return std::make_shared<TestResourceHashed>(make_expected<TestResourceHashed>(path));
}

template<>
Shared<TestResourceHashedSleepy> AsyncDataPool<TestResourceHashedSleepy>::load_data_from(const std::string& path) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return std::make_shared<TestResourceHashedSleepy>(make_expected<TestResourceHashedSleepy>(path));
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
            data_pool.load_async(path).wait();
            std::optional<Shared<T>> opt_res = data_pool.try_load_from_cache(path);
            REQUIRE(opt_res.has_value());
            CHECK(opt_res.value()->value == expected.value);
            CHECK(opt_res.value().use_count() == 2); // Me and pool
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
        }

    }

    SUBCASE("Loading of multiple resources") {
        auto paths = views::generate_n([] { return random_string(1, 64); }, 1024)
            | ranges::to<std::vector<std::string>>();

        std::vector<T> expected = map(paths, &make_expected<T>);

        SUBCASE("Why would I even test this?") {
            std::vector<std::optional<Shared<T>>> fails =
                map(paths, [&](const std::string& path) { return data_pool.try_load_from_cache(path); });
            CHECK(std::none_of(fails.begin(), fails.end(), [](auto& elem) { return elem.has_value(); }));
            // Do something more useful next time
        }

        SUBCASE("Load async all at once, wait until completion") {
            std::vector<std::future<Shared<T>>> futures =
                map(paths, [&](const std::string& path) { return data_pool.load_async(path); });

            std::vector<Shared<T>> results =
                map(futures, [](std::future<Shared<T>>& future) { return future.get(); });

            REQUIRE(std::all_of(results.begin(), results.end(), [](auto& elem) { return elem != nullptr; }));
            CHECK(ranges::equal(results, expected, {}, [](Shared<T>& e) { return e->value; }, &T::value));
        }


        SUBCASE("Load async all at once then immediately try to load from cache") {
            // TODO
        }
    }


}

TEST_CASE("Loading from multiple threads") {
    // TODO
}

TEST_CASE("Exception propagation") {
    // TODO
}


TEST_CASE("Stress testing") {
    // TODO
}

}
