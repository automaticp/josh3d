#include "RingBuffer.hpp"
#include <doctest/doctest.h>


using namespace josh;


TEST_CASE("Ring buffer resizes as expected on push/pop and preserves the trivially-copyable values") {

    BadRingBuffer<int> rb;

    CHECK(rb.size() == 0);
    CHECK(rb.is_empty());

    rb.emplace_front(5);

    CHECK(rb.size() == 1);

    rb.emplace_front(42);

    CHECK(rb.size() == 2);

    const int val_5 = rb.pop_back();

    CHECK(rb.size() == 1);
    CHECK(val_5 == 5);
    CHECK(rb.back() == 42);

    const int val_42 = rb.pop_back();

    CHECK(val_42 == 42);
    CHECK(rb.size() == 0);

    rb.emplace_front(14);
    rb.emplace_front(15);
    rb.emplace_front(16);

    CHECK(rb.pop_back() == 14);
    CHECK(rb.pop_back() == 15);
    rb.emplace_front(17);
    CHECK(rb.pop_back() == 16);
    CHECK(rb.pop_back() == 17);

}

