#include "ShaderSource.hpp"
#include <cstddef>
#include <string_view>
#include <string>


namespace josh {


auto ShaderSource::replace_subrange(const_subrange subrange, std::string_view contents)
    -> const_subrange
{
    const auto offset = subrange.begin() - text_.begin();
    const auto count  = contents.length();
    text_.replace(subrange.begin(), subrange.end(), contents);
    return { text_.begin() + offset, text_.begin() + offset + ptrdiff_t(count) };
}


auto ShaderSource::remove_subrange(const_subrange subrange)
    -> const_iterator
{
    return text_.erase(subrange.begin(), subrange.end());
}


auto ShaderSource::insert_before(const_iterator pos, std::string_view contents)
    -> const_subrange
{
    const auto offset = pos - text_.begin();
    const auto count  = contents.length();
    text_.insert(pos, contents.begin(), contents.end());
    return { text_.begin() + offset, text_.begin() + offset + ptrdiff_t(count) };
}


auto ShaderSource::insert_after(const_iterator pos, std::string_view contents)
    -> const_subrange
{
    return insert_before(pos + 1, contents);
}


auto ShaderSource::insert_line_on_line_before(const_iterator pos, std::string_view contents)
    -> const_subrange
{
    // We operate under the assumption that a "line" is a sequence of chars
    // terminated with a newline, or an EOF.

    auto prev_line_begin = [&](const_iterator pos) -> const_iterator {
        const_iterator cur = pos;
        while (cur != text_.begin()) {
            --cur;
            if (*cur == '\n') {
                return pos;
            }
            --pos;
        }
        return pos;
    };

    pos = prev_line_begin(pos);

    const std::string line_contents = std::string(contents) + '\n';

    // Have to update the iterator after insertion, to account for possible invalidation.
    pos = text_.insert(pos, line_contents.begin(), line_contents.end());

    const size_t         num_inserted  = line_contents.size();
    const const_iterator insertion_end = pos + ptrdiff_t(num_inserted);

    return { pos, insertion_end };
}




auto ShaderSource::insert_line_on_line_after(const_iterator pos, std::string_view contents)
    -> const_subrange
{
    // We operate under the assumption that a "line" is a sequence of chars
    // terminated with a newline, or an EOF. The EOF case is annoying though,
    // because you can't form a past-the-end iterator after the EOF.

    // Newline or EOF.
    auto next_line_tail = [&](const_iterator pos) -> const_iterator {
        while (pos != text_.end()) {
            if (*pos == '\n') {
                return pos;
            }
            ++pos;
        }
        return pos;
    };

    // Search for tail, not past-the-end because the line could end in EOF.
    pos = next_line_tail(pos);

    // If this is the last line and it doesn't terminate in a newline,
    // then insert a newline at the end.
    if (pos == text_.end()) {
        pos = text_.insert(pos, '\n');
    }

    // Then safely advance to the would-be begin of the next line.
    ++pos;

    // Now same as above.
    const std::string line_contents = std::string(contents) + '\n';

    pos = text_.insert(pos, line_contents.begin(), line_contents.end());

    const size_t         num_inserted  = line_contents.size();
    const const_iterator insertion_end = pos + ptrdiff_t(num_inserted);

    // The one extra newline that can be added before the EOF is not included here.
    // Not sure if it matters to anyone.
    return { pos, insertion_end };
}




} // namespace josh
