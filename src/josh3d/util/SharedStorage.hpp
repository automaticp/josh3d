#pragma once
#include <algorithm>
#include <memory>
#include <optional>
#include <utility>


namespace josh {


/*
Abstraction over shared_ptr and weak_ptr that models some off-to-the-side
storage that can have:
    - a set of full owners (SharedStorage);
    - a set of shareable owning read-only viewers (SharedStorageView);
    - a set of move-only owning read-write viewers (SharedStorageMutableView);
    - and a set of shareable non-owning weak observers (SharedStorageObserver).

This isn't faster than just throwing shared_ptrs around, but instead is
focused on constraining the "all over the place" semantics of a naked shared_ptr.
There's certain implied hierarchy based on the names and access interfaces,
where some few instances (preferably, one) are truly owning, and others
have just been given a certain type of access while optionally extending
the lifetime. Also "Storage" in the name implies that this probably
shares and exchanges data and not behavioral objects or systems.

The interface is extremely minimalistic and doesn't suffer from the pains
of shared_ptrs potentially being empty. If you have a storage or an owning
view object, then the objects in the storage are alive.

Moved-from storage and view objects follow the usual rules for the language:
any operation other than reassignment or destruction is UB.

Can be used to communicate data across rendering passes, for example,
where one pass is a writer with full ownership of shared data, and
other passes are simple readers of the data.
*/
template<typename T>
class SharedStorage;

template<typename T>
class SharedStorageObserver;




/*
A const view of SharedStorage that shares ownership.
*/
template<typename T>
class SharedStorageView {
private:
    std::shared_ptr<const T> const_storage_view_;

    friend class SharedStorage<T>;
    friend class SharedStorageObserver<T>;

    SharedStorageView(std::shared_ptr<T> storage_ptr)
        : const_storage_view_{ std::move(storage_ptr) }
    {}

    SharedStorageView(std::shared_ptr<const T> view_ptr)
        : const_storage_view_{ std::move(view_ptr) }
    {}

public:
    const T& operator*() const noexcept { return *const_storage_view_; }
    const T* operator->() const noexcept { return const_storage_view_.get(); }
};


template<typename T>
using SharedView = SharedStorageView<T>; // Typedef for simplicity.




/*
A non-const move-only view of SharedStorage that shares ownership.
*/
template<typename T>
class SharedStorageMutableView {
private:
    std::shared_ptr<T> mutable_storage_view_;

    friend class SharedStorage<T>;

    SharedStorageMutableView(std::shared_ptr<T> storage)
        : mutable_storage_view_{ std::move(storage) }
    {}

public:

    T& operator*() noexcept { return *mutable_storage_view_; }
    const T& operator*() const noexcept { return *mutable_storage_view_; }

    T* operator->() noexcept { return mutable_storage_view_.get(); }
    const T* operator->() const noexcept { return mutable_storage_view_.get(); }

    SharedStorageMutableView(const SharedStorageMutableView&) = delete;
    SharedStorageMutableView& operator=(const SharedStorageMutableView&) = delete;

    SharedStorageMutableView(SharedStorageMutableView&&) = default;
    SharedStorageMutableView& operator=(SharedStorageMutableView&&) = default;
};


template<typename T>
using SharedMutableView = SharedStorageMutableView<T>;




/*
A const reference of SharedStorage that does not participate in ownership.
Behaves similarly to weak_ptr.
*/
template<typename T>
class SharedStorageObserver {
private:
    std::weak_ptr<const T> weak_storage_view_;

    friend class SharedStorage<T>;

    SharedStorageObserver(const std::shared_ptr<T>& storage)
        : weak_storage_view_{ storage }
    {}

public:
    auto try_view() const noexcept
        -> std::optional<SharedStorageView<T>>
    {
        std::shared_ptr<const T> ptr = weak_storage_view_.lock();
        if (ptr) {
            return SharedStorageView<T>{ std::move(ptr) };
        } else {
            return {};
        }
    }

};




template<typename T>
class SharedStorage {
private:
    std::shared_ptr<T> storage_;

    // Copy constructor/assignment are private. Use share_storage() instead.
    // Sharing storage can only be done on a non-const object, however.
    // This is by design. Whether by intelligent or not, is up to time.
    SharedStorage(const SharedStorage&) = default;
    SharedStorage& operator=(const SharedStorage&) = default;

public:
    template<typename ...Args>
    SharedStorage(Args&&... args)
        : storage_{ std::make_shared<T>(std::forward<Args>(args)...) }
    {}


    auto share_view() const noexcept
        -> SharedStorageView<T> { return { storage_ }; }

    auto share_mutable_view() noexcept
        -> SharedStorageMutableView<T> { return { storage_ }; }

    auto observe() const noexcept
        -> SharedStorageObserver<T> { return { storage_ }; }

    auto share_storage() noexcept
        -> SharedStorage { return { storage_ }; }


    T& operator*() noexcept { return *storage_; }
    const T& operator*() const noexcept { return *storage_; }

    T* operator->() noexcept { return storage_.get(); }
    const T* operator->() const noexcept { return storage_.get(); }


    SharedStorage(SharedStorage&&) = default;
    SharedStorage& operator=(SharedStorage&&) = default;

};




} // namespace josh
