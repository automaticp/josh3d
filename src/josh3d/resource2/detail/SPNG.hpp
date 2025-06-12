#pragma once
#include <memory>
#include <spng.h>


namespace josh::detail {


struct SPNGContextDeleter
{
    void operator()(spng_ctx* p) const noexcept { spng_ctx_free(p); }
};

using spng_ctx_ptr = std::unique_ptr<spng_ctx, SPNGContextDeleter>;

// For some bizzare reason, each encode should allocate a new context.
inline auto make_spng_encoding_context()
    -> spng_ctx_ptr
{
    return spng_ctx_ptr{ spng_ctx_new(SPNG_CTX_ENCODER) };
}

inline auto make_spng_decoding_context()
    -> spng_ctx_ptr
{
    return spng_ctx_ptr{ spng_ctx_new(0) };
}


} // namespace josh::detail
