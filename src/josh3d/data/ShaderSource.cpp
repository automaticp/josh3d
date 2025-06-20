#include "ShaderSource.hpp"
#include "Common.hpp"
#include "Scalars.hpp"


namespace josh {


auto ShaderSource::replace_subrange(const_subrange subrange, StrView contents)
    -> const_subrange
{
    const auto offset = subrange.begin() - _text.begin();
    const auto count  = contents.length();
    _text.replace(subrange.begin(), subrange.end(), contents);
    return { _text.begin() + offset, _text.begin() + offset + ptrdiff(count) };
}

auto ShaderSource::remove_subrange(const_subrange subrange)
    -> const_iterator
{
    return _text.erase(subrange.begin(), subrange.end());
}

auto ShaderSource::insert_before(const_iterator pos, StrView contents)
    -> const_subrange
{
    const auto offset = pos - _text.begin();
    const auto count  = contents.length();
    _text.insert(pos, contents.begin(), contents.end());
    return { _text.begin() + offset, _text.begin() + offset + ptrdiff(count) };
}

auto ShaderSource::insert_after(const_iterator pos, StrView contents)
    -> const_subrange
{
    return insert_before(pos + 1, contents);
}

auto ShaderSource::insert_line_on_line_before(const_iterator pos, StrView contents)
    -> const_subrange
{
    // We operate under the assumption that a "line" is a sequence of chars
    // terminated with a newline, *or* an EOF.

    const auto prev_line_begin = [&](const_iterator pos)
        -> const_iterator
    {
        const_iterator cur = pos;
        while (cur != _text.begin())
        {
            --cur;

            if (*cur == '\n')
                return pos;

            --pos;
        }
        return pos;
    };

    pos = prev_line_begin(pos);

    const String line_contents = fmt::format("{}\n", contents);

    // Have to update the iterator after insertion, to account for possible invalidation.
    pos = _text.insert(pos, line_contents.begin(), line_contents.end());

    const usize          num_inserted  = line_contents.size();
    const const_iterator insertion_end = pos + ptrdiff(num_inserted);

    return { pos, insertion_end };
}

auto ShaderSource::insert_line_on_line_after(const_iterator pos, StrView contents)
    -> const_subrange
{
    // We operate under the assumption that a "line" is a sequence of chars
    // terminated with a newline, or an EOF. The EOF case is annoying though,
    // because you can't form a past-the-end iterator after the EOF.

    // Newline or EOF.
    const auto next_line_tail = [&](const_iterator pos)
        -> const_iterator
    {
        while (pos != _text.end())
        {
            if (*pos == '\n')
                return pos;

            ++pos;
        }
        return pos;
    };

    // Search for tail, not past-the-end because the line could end in EOF.
    pos = next_line_tail(pos);

    // If this is the last line and it doesn't terminate in a newline,
    // then insert a newline at the end.
    if (pos == _text.end())
        pos = _text.insert(pos, '\n');

    // Then safely advance to the would-be begin of the next line.
    ++pos;

    // Now same as above.
    const String line_contents = fmt::format("{}\n", contents);

    pos = _text.insert(pos, line_contents.begin(), line_contents.end());

    const usize          num_inserted  = line_contents.size();
    const const_iterator insertion_end = pos + ptrdiff(num_inserted);

    // The one extra newline that can be added before the EOF is not included here.
    // Not sure if it matters to anyone.
    return { pos, insertion_end };
}


} // namespace josh
