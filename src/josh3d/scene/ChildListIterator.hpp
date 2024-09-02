#pragma once
#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>
#include <iterator>


namespace josh {


/*
Customization point for custom hierarchies.

Specialize if your "AsChild" component has a different way to get
to the next/previous entities in the list.
*/
template<typename ValueT, typename HandleT, typename AsChildT>
struct child_list_traits {

    static auto next_entity(entt::const_handle handle) noexcept
        -> entt::entity
    {
        return handle.get<AsChildT>().next;
    }

    static auto prev_entity(entt::const_handle handle) noexcept
        -> entt::entity
    {
        return handle.get<AsChildT>().prev;
    }

    static auto to_value(HandleT handle) noexcept
        -> ValueT
    {
        // entt::handle and entt::const_handle will decay to entt::entity
        // automatically due to the implicit conversion operator.
        return handle;
    }

};


/*
An iterator type for interned lists of children at a given depth of a hierarchy.

ValueT   is likely one of: entt::entity, entt::handle, entt::const_handle;
HandleT  is likely one of: entt::handle, entt::const_handle.
AsChildT is a component that marks an element in an interned list of children.
*/
template<typename ValueT, typename HandleT, typename AsChildT>
class child_list_iterator {
public:
    using value_type           = ValueT;
    using difference_type      = ptrdiff_t;
    using iterator_category    = std::bidirectional_iterator_tag;
    using handle_type          = HandleT;
    using child_component_type = AsChildT;
    using traits_type          = child_list_traits<ValueT, HandleT, AsChildT>;

    constexpr child_list_iterator() = default;
    child_list_iterator(HandleT handle) : handle_{ handle } {}

    constexpr auto operator*() const noexcept
        -> value_type
    {
        return traits_type::to_value(handle_);
    }

    auto operator++() noexcept
        -> child_list_iterator&
    {
        handle_ = { *handle_.registry(), traits_type::next_entity(handle_) };
        return *this;
    }

    auto operator--() noexcept
        -> child_list_iterator&
    {
        handle_ = { *handle_.registry(), traits_type::prev_entity(handle_) };
        return *this;
    }

    auto operator++(int) noexcept
        -> child_list_iterator
    {
        auto old_handle = handle_;
        ++*this;
        return old_handle;
    }

    auto operator--(int) noexcept
        -> child_list_iterator
    {
        auto old_handle = handle_;
        --*this;
        return old_handle;
    }

    constexpr bool operator==(const child_list_iterator& other) const noexcept {
        // End iterator is described by a null entity and *must* have the same registry.
        // This is guaranteed by a normal sequence of increments, where an increment
        // from last element to the "end" will construct an iterator { registry, entt::null }.
        // The same applies when a decrement happens past-the-beginning.
        assert(handle_.registry() == other.handle_.registry());
        return handle_.entity() == other.handle_.entity();
    }

private:
    HandleT handle_{};
};




template<typename ValueT, typename HandleT, typename AsChildT>
class child_list_view {
public:
    using iterator = child_list_iterator<ValueT, HandleT, AsChildT>;

    child_list_view(HandleT first) : first_{ first } {}

    auto begin() const noexcept { return iterator(first_);                             }
    auto end()   const noexcept { return iterator({ *first_.registry(), entt::null }); }

private:
    HandleT first_;
};




} // namespace josh
