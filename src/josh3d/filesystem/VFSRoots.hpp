#pragma once
#include "Filesystem.hpp"
#include <list>



namespace josh {


/*
The container of VFS that is responsible for storing and managing
insertion/removal of roots.

Iterators are only invalidated on removal and only for removed
elements.

Is ordered by push operations, with newly pushed elements inserted
at the front.

N.B. Originally planned to have set-like semantics based on the
equivalence of the actual filesystem entries, but that carried
too much trouble because the equivalence check can fail if
the directory is no longer valid, which quickly cascaded into
the game of "Who wants to handle invalid entries?", with unclear
responsibilities and a mess overall. So now this is just
a list wrapper, that disallows modification in-place.
*/
class VFSRoots {
private:
    using list_type = std::list<Directory>;
    list_type roots_;

public:
    using const_iterator = list_type::const_iterator;

    VFSRoots() = default;

    auto begin()  const noexcept { return roots_.cbegin(); }
    auto cbegin() const noexcept { return roots_.cbegin(); }
    auto end()    const noexcept { return roots_.cend();   }
    auto cend()   const noexcept { return roots_.cend();   }


    // Push the Directory to the front of the list.
    //
    // A shorthand for:
    //   `insert_before(begin(), dir);`
    auto push_front(const Directory& dir)
        -> const_iterator
    {
        return insert_before(begin(), dir);
    }

    // Push the Directory to the front of the list.
    auto push_front(Directory&& dir)
        -> const_iterator
    {
        return insert_before(begin(), std::move(dir));
    }


    // Insert the Directory before the element `pos`.
    // Returns iterator to the newly inserted element.
    auto insert_before(const_iterator pos, const Directory& dir)
        -> const_iterator
    {
        return roots_.insert(pos, dir);
    }


    // Insert the Directory before the element `pos`.
    // Returns iterator to the newly inserted element.
    auto insert_before(const_iterator pos, Directory&& dir)
        -> const_iterator
    {
        return roots_.insert(pos, std::move(dir));
    }


    // Reorder an element to be placed before another one.
    void order_before(
        const_iterator before_this_element,
        const_iterator element_to_reorder) noexcept
    {
        roots_.splice(before_this_element, roots_, element_to_reorder);
    }


    // Shorthand for `order_before(++target, to_reorder);`
    void order_after(
        const_iterator after_this_element,
        const_iterator element_to_reorder) noexcept
    {
        order_before(++after_this_element, element_to_reorder);
    }


    // Behaves like list::erase, except returns a const_iterator
    // because you're not supposed to modify the elements.
    //
    // UB if `iter` does not refer to an element of VFSRoots.
    //
    // Returns iterator to the next element or the updated end()
    // iterator if the erased element was last.
    auto erase(const_iterator iter) noexcept
        -> const_iterator
    {
        return roots_.erase(iter);
    }


    // Removes currently stored root Directories for which
    // is_valid() is no longer true.
    // Returns the number of elements removed.
    size_t remove_invalid() {
        return remove_invalid(roots_.cbegin());
    }


    // Removes currently stored root Directories for which
    // is_valid() is no longer true beginning with `start_from`.
    // Returns the number of elements removed.
    size_t remove_invalid(const_iterator start_from) {
        size_t num_removed{ 0 };
        for (auto it = start_from; it != roots_.end();) {
            if (!it->is_valid()) {
                it = roots_.erase(it);
                ++num_removed;
            } else {
                ++it;
            }
        }
        return num_removed;
    }


    // Removes currently stored root Directories for which
    // is_valid() is no longer true.
    // Returns the number of elements removed.
    // Outputs invalidated Directory entries through `out_it`.
    template<std::output_iterator<Directory> OutputIt>
    size_t remove_invalid_into(OutputIt out_it) {
        return remove_invalid_into(roots_.cbegin(), std::move(out_it));
    }


    // Removes currently stored root Directories for which
    // is_valid() is no longer true beginning with `start_from`.
    // Returns the number of elements removed.
    // Outputs invalidated Directory entries through `out_it`.
    template<std::output_iterator<Directory> OutputIt>
    size_t remove_invalid_into(const_iterator start_from,
        OutputIt out_it)
    {

        auto remove_const_lmao = [this](const_iterator it) {
            return roots_.erase(it, it);
        };
        auto nonconst_start = remove_const_lmao(start_from);

        size_t num_removed{ 0 };
        for (auto it = nonconst_start; it != roots_.end();) {
            if (!it->is_valid()) {
                *out_it++ = std::move(*it);
                it = roots_.erase(it);
                ++num_removed;
            } else {
                ++it;
            }
        }
        return num_removed;
    }

};




} // namespace josh
