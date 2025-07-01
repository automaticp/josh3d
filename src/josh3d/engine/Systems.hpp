#pragma once
#include "HashedString.hpp"
#include "KitchenSink.hpp"
#include "SystemKey.hpp"
#include "UniqueFunction.hpp"


/*
WIP
*/
namespace josh {

struct Runtime;

struct SystemContext
{
    Runtime& runtime;
};

JOSH3D_DERIVE_TYPE(System, UniqueFunction<void(SystemContext&)>);

} // namespace josh
