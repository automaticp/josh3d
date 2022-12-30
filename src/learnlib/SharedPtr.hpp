#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>


// Shared pointer that always stores the control block next to the data.
// This makes the memory footprint equal to sizeof(void*)
// at the cost of inability to take ownership from raw T*.
// (Which you can avoid in most cases anyways).
//
// Lacks a lot of features like dynamic pointer casts,
// implicit converions, ordering and comparison operators,
// custom allocators (yes, allocators, not just deleters), etc.
//
// Even though this uses atomics for refcounting,
// doesn't mean the whole thing is thread safe.
template<typename T>
class SharedPtr {
private:
    struct Storage {
        std::atomic<size_t> count_{};
        T value_;
    };

    Storage* storage_{ nullptr };
public:
    // Will interfere with construction of types
    // that take nullptr_t as a single c-tor argument?
    explicit SharedPtr(std::nullptr_t) {}

    template<typename ...Args>
    SharedPtr(Args&&... args) :
        storage_{ new Storage(std::forward<Args>(args)...) }
    {}


    T* get() const noexcept {
        return storage_ ? &storage_->value_ : nullptr;
    }

    T& operator*() const noexcept {
        return *get();
    }

    T* operator->() const noexcept {
        return get();
    }


    size_t use_count() const noexcept {
        if (!storage_) return 0;
        return storage_->count_.load(std::memory_order_relaxed);
    }

    explicit operator bool() const noexcept { return storage_; }



    void swap(SharedPtr& rhs) noexcept {
        std::swap(this->storage_, rhs.storage_);
    }


    SharedPtr(SharedPtr&& other) noexcept :
        storage_{ other.storage_ }
    {
        other.storage_ = nullptr;
    }

    SharedPtr& operator=(SharedPtr&& other) noexcept {
        decrement_count(); // Previous storage

        storage_ = other.storage_;
        other.storage_ = nullptr;
        return *this;
    }

    SharedPtr(const SharedPtr& other) noexcept :
        storage_{ other.storage_ }
    {
        increment_count();
    }

    SharedPtr& operator=(const SharedPtr& other) noexcept(std::is_nothrow_destructible_v<T>) {
        if (&other != this) {
            decrement_count(); // Previous storage
            storage_ = other.storage_;
            increment_count(); // New storage
        }
        return *this;
    }

    ~SharedPtr() noexcept(std::is_nothrow_destructible_v<T>) {
        decrement_count();
    }

private:
    void increment_count() noexcept {
        // Taken from boost atomic examples on refcounting
        // https://www.boost.org/doc/libs/1_80_0/libs/atomic/doc/html/atomic/usage_examples.html
        if (storage_) {
            storage_->count_.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void decrement_count() noexcept(std::is_nothrow_destructible_v<T>) {
        // Taken from boost atomic examples on refcounting
        // https://www.boost.org/doc/libs/1_80_0/libs/atomic/doc/html/atomic/usage_examples.html
        if (storage_ && storage_->count_.fetch_sub(1, std::memory_order_release) == 1) {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete storage_;
        }
    }

};
