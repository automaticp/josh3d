#pragma once
#include <boost/type_index.hpp>
#include <entt/core/type_info.hpp>


/*
Because providing something as advanced as a *readable type name*
would be surely too much to ask from a mere language standard.
*/
namespace josh {


/*
NOTE: In boost, type_index serves as both a type_info and type_index
replacement, which is more sane, but contradicts conventions in std
and entt.
*/
using TypeIndex = boost::typeindex::type_index;
using TypeInfo  = boost::typeindex::type_info;
using boost::typeindex::type_id;


/*
TODO: I'm not fully happy with either implementation. Boost pretty_name()
returns a std::string and can just fail for random reasons, and entt does not
work with dynamic types at all. And these two downsides seem to be in a
direct contradiction with one another.
*/


} // namespace josh
