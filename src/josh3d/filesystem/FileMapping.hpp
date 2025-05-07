#pragma once
#include "Common.hpp"
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/file_mapping.hpp>


namespace josh {


using MappedRegion = boost::interprocess::mapped_region;

// TODO: Replace with a version that is constructible from FILE* or the like.
using FileMapping = boost::interprocess::file_mapping;


template<typename T = byte>
auto to_span(MappedRegion& mregion) noexcept
    -> Span<T>
{ return { (T*)mregion.get_address(), mregion.get_size() }; }

template<typename T = byte>
auto to_span(const MappedRegion& mregion) noexcept
    -> Span<const T>
{ return { (const T*)mregion.get_address(), mregion.get_size() }; }

template<typename T = byte>
auto to_span(MappedRegion&& mregion) noexcept
    -> Span<T>
= delete;




} // namespace josh
