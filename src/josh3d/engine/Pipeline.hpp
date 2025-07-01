#pragma once
#include "Common.hpp"
#include "ContainerUtils.hpp"
#include "HashedString.hpp"
#include "PipelineStage.hpp"
#include "SystemKey.hpp"


namespace josh {

/*
Ordered container of PipelineStages segregated by their kind.

HMM: The Pipeline and the RenderEngine are coupled though the StageContext.
This is, at the very least, interesting. Can we avoid that somehow?
*/
struct Pipeline
{
    // If the name is empty, then a stripped unqualified type name
    // of T will be used instead.
    template<typename T>
    auto push(
        StageKind kind,
        T&&       stage,
        HashedID  instance_id = {},
        String    name        = {}) -> SystemKey;

    struct StoredStage
    {
        String        name;
        PipelineStage stage;
    };

    template<typename T>
    auto try_get(HashedID instance_id = {}) -> T*;
    auto try_get(SystemKey key) -> StoredStage*;

    auto view(StageKind kind) const noexcept -> Span<const SystemKey>;


    HashMap<SystemKey, StoredStage> _stages;
    Vector<SystemKey>               _precompute;
    Vector<SystemKey>               _primary;
    Vector<SystemKey>               _postprocess;
    Vector<SystemKey>               _overlay;

    auto _push_stage(
        Vector<SystemKey>& list,
        auto&&             stage,
        HashedID           instance_id,
        String             name)
            -> SystemKey;
};


inline auto Pipeline::try_get(SystemKey key)
    -> StoredStage*
{
    return try_find_value(_stages, key);
}

template<typename T>
auto Pipeline::try_get(HashedID instance_id)
    -> T*
{
    const SystemKey key = { type_id<T>(), instance_id };
    auto it = _stages.find(key);
    if (it == _stages.end()) return nullptr;
    return &it->second.stage.target_unchecked<T>();
}

template<typename T>
auto Pipeline::push(
    StageKind kind,
    T&&       stage,
    HashedID  instance_id,
    String    name)
    -> SystemKey
{
    const auto make_stage_name = [](String src) -> String
    {
        if (src.empty())
        {
            // Take the qualified typename returned from pretty name
            // (ex. "josh::Bloom") and remove the leading namespaces.
            String pretty = type_id<T>().pretty_name();
            const uindex last_colon = pretty.find_last_of(':');
            if (last_colon != String::npos)
                pretty = pretty.substr(last_colon + 1);

            return pretty;
        }

        return MOVE(src);
    };

    auto& list = eval%[&]() -> auto& {
        switch (kind)
        {
            case StageKind::Precompute:  return _precompute;
            case StageKind::Primary:     return _primary;
            case StageKind::Postprocess: return _postprocess;
            case StageKind::Overlay:     return _overlay;
        }
        panic();
    };
    return _push_stage(list, FORWARD(stage), instance_id, make_stage_name(name));
}

inline auto Pipeline::view(StageKind kind) const noexcept
    -> Span<const SystemKey>
{
    switch (kind)
    {
        case StageKind::Precompute:  return _precompute;
        case StageKind::Primary:     return _primary;
        case StageKind::Postprocess: return _postprocess;
        case StageKind::Overlay:     return _overlay;
    }
    panic();
}

template<typename T>
auto Pipeline::_push_stage(
    Vector<SystemKey>& list,
    T&&                stage,
    HashedID           instance_id,
    String             name)
        -> SystemKey
{
    const SystemKey key = {
        .type        = type_id<T>(),
        .instance_id = instance_id,
    };
    auto [it, was_emplaced] = _stages.try_emplace(key, MOVE(name), FORWARD(stage));
    if (not was_emplaced)
    {
        // HMM: What do we do? Just return the key as is?
        // Do we want to assert? Why?
        return key;
    }
    list.push_back(key);
    return key;
}


} // namespace josh
