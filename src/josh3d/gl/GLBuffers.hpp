#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep
#include "GLAPI.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLKind.hpp"
#include "GLScalars.hpp"
#include "GLMutability.hpp"
#include "EnumUtils.hpp"
#include "detail/MagicConstructorsMacro.hpp"
#include "detail/RawGLHandle.hpp"
#include "detail/StaticAssertFalseMacro.hpp"
#include <glbinding/gl/bitfield.h>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <span>
#include <cassert>


namespace josh {



template<trivially_copyable T, mutability_tag MutT>
class RawBuffer;
template<mutability_tag MutT>
class RawUntypedBuffer;




enum class StorageMode : GLuint {
    StaticServer  = GLuint(gl::GL_NONE_BIT),            // STATIC_DRAW
    DynamicServer = GLuint(gl::GL_DYNAMIC_STORAGE_BIT), // DYNAMIC_DRAW
    StaticClient  = GLuint(gl::GL_CLIENT_STORAGE_BIT),  // STATIC_READ
    DynamicClient = GLuint(gl::GL_DYNAMIC_STORAGE_BIT | gl::GL_CLIENT_STORAGE_BIT) // DYNAMIC_READ
};


enum class PermittedMapping : GLuint {
    NoMapping                   = GLuint(gl::GL_NONE_BIT),
    Read                        = GLuint(gl::GL_MAP_READ_BIT),
    Write                       = GLuint(gl::GL_MAP_WRITE_BIT),
    ReadWrite                   = Read | Write,
};


enum class PermittedPersistence : GLuint {
    NotPersistent      = GLuint(gl::GL_NONE_BIT),
    Persistent         = GLuint(gl::GL_MAP_PERSISTENT_BIT),
    PersistentCoherent = Persistent | GLuint(gl::GL_MAP_COHERENT_BIT),
};




struct StoragePolicies {
    StorageMode          storage_mode          = StorageMode::DynamicServer;
    PermittedMapping     permitted_mapping     = PermittedMapping::ReadWrite;
    PermittedPersistence permitted_persistence = PermittedPersistence::NotPersistent;
};








/*
Buffer mapping flags but split so that you can't pass the wrong combination
for each respective mapping access.
*/


enum class MappingAccess : GLuint {
    Read      = GLuint(gl::GL_MAP_READ_BIT),
    Write     = GLuint(gl::GL_MAP_WRITE_BIT),
    ReadWrite = Read | Write,
};






enum class PendingOperations : GLuint {
    SynchronizeOnMap = GLuint(gl::GL_NONE_BIT),
    DoNotSynchronize = GLuint(gl::GL_MAP_UNSYNCHRONIZED_BIT),
};


// Do you have one?
enum class FlushPolicy : GLuint {
    AutomaticOnUnmap   = GLuint(gl::GL_NONE_BIT),
    MustFlushExplicity = GLuint(gl::GL_MAP_FLUSH_EXPLICIT_BIT),
};


enum class PreviousContents : GLuint {
    DoNotInvalidate       = GLuint(gl::GL_NONE_BIT),
    InvalidateAll         = GLuint(gl::GL_MAP_INVALIDATE_BUFFER_BIT),
    InvalidateMappedRange = GLuint(gl::GL_MAP_INVALIDATE_RANGE_BIT),
};


enum class Persistence : GLuint {
    NotPersistent      = GLuint(gl::GL_NONE_BIT),
    Persistent         = GLuint(gl::GL_MAP_PERSISTENT_BIT),
    PersistentCoherent = Persistent | GLuint(gl::GL_MAP_COHERENT_BIT),
};




// Return type for querying all policies at once.
struct MappingPolicies {
    MappingAccess     access;
    PendingOperations pending_operations = PendingOperations::SynchronizeOnMap;
    FlushPolicy       flush_policy       = FlushPolicy::AutomaticOnUnmap;
    PreviousContents  previous_contents  = PreviousContents::DoNotInvalidate;
    Persistence       persistence        = Persistence::NotPersistent;
};





enum class BufferTarget : GLuint {
    // VertexArray      = GLuint(gl::GL_ARRAY_BUFFER),
    // ElementArray     = GLuint(gl::GL_ELEMENT_ARRAY_BUFFER),
    DispatchIndirect = GLuint(gl::GL_DISPATCH_INDIRECT_BUFFER),
    DrawIndirect     = GLuint(gl::GL_DRAW_INDIRECT_BUFFER),
    Parameter        = GLuint(gl::GL_PARAMETER_BUFFER),
    PixelPack        = GLuint(gl::GL_PIXEL_PACK_BUFFER),
    PixelUnpack      = GLuint(gl::GL_PIXEL_UNPACK_BUFFER),
    // Texture          = GLuint(gl::GL_TEXTURE_BUFFER),
    // QUERY target is redundant in presence of `glGetQueryBufferObjectui64v`.
    // Query            = GLuint(gl::GL_QUERY_BUFFER),
    // COPY_READ/WRITE targets are redundant in presence of DSA copy commands.
    // CopyRead         = GLuint(gl::GL_COPY_READ_BUFFER),
    // CopyWrite        = GLuint(gl::GL_COPY_WRITE_BUFFER),
};


enum class BufferTargetIndexed : GLuint {
    ShaderStorage     = GLuint(gl::GL_SHADER_STORAGE_BUFFER),
    Uniform           = GLuint(gl::GL_UNIFORM_BUFFER),
    TransformFeedback = GLuint(gl::GL_TRANSFORM_FEEDBACK_BUFFER),
    AtomicCounter     = GLuint(gl::GL_ATOMIC_COUNTER_BUFFER),
};









namespace detail {


template<typename T>
std::span<T> map_buffer_range_impl(
    GLuint object_id, GLintptr elem_offset, GLsizeiptr elem_count,
    gl::BufferAccessMask access, gl::BufferAccessMask rw_bits)
{
    constexpr auto rw_maskout =
        gl::BufferAccessMask{
            ~to_underlying( // Bitfields in glbinding don't support `operator~`...
                gl::BufferAccessMask{ gl::GL_MAP_READ_BIT | gl::GL_MAP_WRITE_BIT }
            )
        };
    access = (access & rw_maskout) | rw_bits;

    // If any of these asserts trigger, then I probably messed up with the public interface somewhere.
    // Ideally, we should construct a public interface where triggering this is not possible
    // unless you explicitly use private enum members or do something else completely unreasonable.
    assert(bool(access & gl::GL_MAP_READ_BIT) || bool(access & gl::GL_MAP_WRITE_BIT) &&
        "At least one of GL_MAP_READ_BIT or GL_MAP_WRITE_BIT must be set.");
    assert((!bool(access & gl::GL_MAP_UNSYNCHRONIZED_BIT)    || !bool(access & gl::GL_MAP_READ_BIT)) &&
        "Not sure why, but this flag may not be used in combination with GL_MAP_READ_BIT.");
    assert((!bool(access & gl::GL_MAP_INVALIDATE_BUFFER_BIT) || !bool(access & gl::GL_MAP_READ_BIT)) &&
        "This flag may not be used in combination with GL_MAP_READ_BIT.");
    assert((!bool(access & gl::GL_MAP_INVALIDATE_RANGE_BIT)  || !bool(access & gl::GL_MAP_READ_BIT)) &&
        "This flag may not be used in combination with GL_MAP_READ_BIT.");
    assert((!bool(access & gl::GL_MAP_FLUSH_EXPLICIT_BIT)    || bool(access & gl::GL_MAP_WRITE_BIT)) &&
        "This flag may only be used in conjunction with GL_MAP_WRITE_BIT.");

    void* buf =
        gl::glMapNamedBufferRange(
            object_id,
            elem_offset * sizeof(T),
            elem_count * sizeof(T),
            access
        );

    return { static_cast<T*>(buf), elem_count };
}





template<typename CRTP>
struct BufferDSAInterface_CommonQueries {
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
public:
    // Wraps `glGetNamedBufferParameteri64v` with `pname = GL_BUFFER_SIZE`.
    GLsizeiptr get_size_bytes() const noexcept {
        GLint64 size;
        gl::glGetNamedBufferParameteri64v(
            self_id(), gl::GL_BUFFER_SIZE, &size
        );
        return size;
    }

private:
    GLuint get_storage_mode_flags() const noexcept {
        GLenum flags;
        gl::glGetNamedBufferParameteriv(self_id(), gl::GL_BUFFER_STORAGE_FLAGS, &flags);
        return enum_cast<GLuint>(flags);
    }
public:

    // Wraps `glGetNamedBufferParameteriv` with `pname = GL_BUFFER_STORAGE_FLAGS`.
    auto get_storage_policies() const noexcept
        -> StoragePolicies
    {
        const GLuint flags = get_storage_mode_flags();

        constexpr GLuint mode_mask        = GLuint(gl::GL_DYNAMIC_STORAGE_BIT | gl::GL_CLIENT_STORAGE_BIT);
        constexpr GLuint mapping_mask     = GLuint(gl::GL_MAP_READ_BIT | gl::GL_MAP_WRITE_BIT);
        constexpr GLuint persistence_mask = GLuint(gl::GL_MAP_PERSISTENT_BIT | gl::GL_MAP_COHERENT_BIT);

        return {
            .storage_mode          = StorageMode         { flags & mode_mask        },
            .permitted_mapping     = PermittedMapping    { flags & mapping_mask     },
            .permitted_persistence = PermittedPersistence{ flags & persistence_mask },
        };
    }

};




template<typename CRTP>
struct BufferDSAInterface_Bind {
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
public:
    // Wraps `glBindBuffer`.
    template<BufferTarget TargetV>
    [[nodiscard("BindTokens have to be provided to an API call that expects bound state.")]]
    auto bind() const noexcept {
        gl::glBindBuffer(enum_cast<GLenum>(TargetV), self_id());
        if constexpr      (TargetV == BufferTarget::DispatchIndirect) { return BindToken<Binding::DispatchIndirectBuffer>{ self_id() }; }
        else if constexpr (TargetV == BufferTarget::DrawIndirect)     { return BindToken<Binding::DrawIndirectBuffer>    { self_id() }; }
        else if constexpr (TargetV == BufferTarget::Parameter)        { return BindToken<Binding::ParameterBuffer>       { self_id() }; }
        else if constexpr (TargetV == BufferTarget::PixelPack)        { return BindToken<Binding::PixelPackBuffer>       { self_id() }; }
        else if constexpr (TargetV == BufferTarget::PixelUnpack)      { return BindToken<Binding::PixelUnpackBuffer>     { self_id() }; }
        else { JOSH3D_STATIC_ASSERT_FALSE(CRTP); }
    }

    // Wraps `glBindBufferBase`.
    template<BufferTargetIndexed TargetV>
    // [[nodiscard("Discarding bound state is error-prone. Consider using BindGuard to automate unbinding.")]]
    auto bind_to_index(GLuint index) const noexcept {
        gl::glBindBufferBase(enum_cast<GLenum>(TargetV), index, self_id());
        if constexpr      (TargetV == BufferTargetIndexed::Uniform)           { return BindToken<BindingIndexed::UniformBuffer>          { self_id(), index }; }
        else if constexpr (TargetV == BufferTargetIndexed::ShaderStorage)     { return BindToken<BindingIndexed::ShaderStorageBuffer>    { self_id(), index }; }
        else if constexpr (TargetV == BufferTargetIndexed::TransformFeedback) { return BindToken<BindingIndexed::TransformFeedbackBuffer>{ self_id(), index }; }
        else if constexpr (TargetV == BufferTargetIndexed::AtomicCounter)     { return BindToken<BindingIndexed::AtomicCounterBuffer>    { self_id(), index }; }
        else { JOSH3D_STATIC_ASSERT_FALSE(CRTP); }
    }

};





template<typename CRTP, typename T>
struct BufferDSAInterface_Mapping {
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
    const CRTP& self() const noexcept { return static_cast<const CRTP&>(*this); }
    using mt = mutability_traits<CRTP>;
public:

    // Wraps `glMapNamedBufferRange` with `access = GL_MAP_READ_BIT | [flags]`.
    [[nodiscard]] std::span<const T> map_range_for_read(
        OffsetElems       elem_offset,
        NumElems          elem_count,
        PendingOperations pending_ops  = PendingOperations::SynchronizeOnMap,
        Persistence       persistence  = Persistence::NotPersistent) const noexcept
    {
        gl::BufferAccessMask access{
            to_underlying(pending_ops) |
            to_underlying(persistence)
        };
        return map_buffer_range_impl<T>(
            self_id(), elem_offset, elem_count,
            access, gl::GL_MAP_READ_BIT
        );
    }

    // Wraps `glMapNamedBufferRange` with `offset = 0`, `length = get_size_bytes()` and `access = GL_MAP_READ_BIT | [flags]`.
    //
    // Maps the entire buffer.
    [[nodiscard]] std::span<const T> map_for_read(
        PendingOperations pending_ops = PendingOperations::SynchronizeOnMap,
        Persistence       persistence = Persistence::NotPersistent) const noexcept
    {
        return map_range_for_read(OffsetElems{ 0 }, self().get_num_elements(), pending_ops, persistence);
    }

    // Wraps `glMapNamedBufferRange` with `access = GL_MAP_WRITE_BIT | [flags]`.
    [[nodiscard]] std::span<T> map_range_for_write(
        OffsetElems       elem_offset,
        NumElems          elem_count,
        PendingOperations pending_ops       = PendingOperations::SynchronizeOnMap,
        FlushPolicy       flush_policy      = FlushPolicy::AutomaticOnUnmap,
        PreviousContents  previous_contents = PreviousContents::DoNotInvalidate,
        Persistence       persistence       = Persistence::NotPersistent) const noexcept
            requires mt::is_mutable
    {
        gl::BufferAccessMask access{
            to_underlying(pending_ops)       |
            to_underlying(flush_policy)      |
            to_underlying(previous_contents) |
            to_underlying(persistence)
        };
        return map_buffer_range_impl<T>(
            self_id(), elem_offset, elem_count,
            access, gl::GL_MAP_WRITE_BIT
        );
    }

    // Wraps `glMapNamedBufferRange` with `offset = 0`, `length = get_size_bytes()` and `access = GL_MAP_WRITE_BIT | [flags]`.
    //
    // Maps the entire buffer.
    [[nodiscard]] std::span<T> map_for_write(
        PendingOperations pending_ops       = PendingOperations::SynchronizeOnMap,
        FlushPolicy       flush_policy      = FlushPolicy::AutomaticOnUnmap,
        PreviousContents  previous_contents = PreviousContents::DoNotInvalidate,
        Persistence       persistence       = Persistence::NotPersistent) const noexcept
            requires mt::is_mutable
    {
        return map_range_for_write(OffsetElems{ 0 }, self().get_num_elements(), pending_ops, flush_policy, previous_contents, persistence);
    }

    // Wraps `glMapNamedBufferRange` with `access = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | [flags]`.
    [[nodiscard]] std::span<T> map_range_for_readwrite(
        OffsetElems       elem_offset,
        NumElems          elem_count,
        PendingOperations pending_ops = PendingOperations::SynchronizeOnMap,
        FlushPolicy       flush_polcy = FlushPolicy::AutomaticOnUnmap,
        Persistence       persistence = Persistence::NotPersistent) const noexcept
            requires mt::is_mutable
    {
        gl::BufferAccessMask access{
            to_underlying(pending_ops)  |
            to_underlying(flush_polcy) |
            to_underlying(persistence)
        };
        return map_buffer_range_impl<T>(
            self_id(), elem_offset, elem_count,
            access, (gl::GL_MAP_READ_BIT | gl::GL_MAP_WRITE_BIT)
        );
    }

    // Wraps `glMapNamedBufferRange` with `offset = 0`, `length = get_size_bytes()` and `access = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | [flags]`.
    //
    // Maps the entire buffer.
    [[nodiscard]] std::span<T> map_for_readwrite(
        PendingOperations      pending_ops = PendingOperations::SynchronizeOnMap,
        FlushPolicy            flush_polcy = FlushPolicy::AutomaticOnUnmap,
        Persistence            persistence = Persistence::NotPersistent) const noexcept
            requires mt::is_mutable
    {
        return map_range_for_readwrite(OffsetElems{ 0 }, self().get_num_elements(), pending_ops, flush_polcy, persistence);
    }


    // Wraps `glGetNamedBufferParameteriv` with `pname = GL_BUFFER_MAPPED`.
    bool is_currently_mapped() const noexcept {
        GLboolean is_mapped;
        gl::glGetNamedBufferParameteriv(self_id(), gl::GL_BUFFER_MAPPED, &is_mapped);
        return bool(is_mapped);
    }

    // Wraps `glGetNamedBufferParameteri64v` with `pname = GL_BUFFER_MAP_OFFSET` divided by element size.
    OffsetElems get_current_mapping_offset() const noexcept {
        GLint64 offset_bytes;
        gl::glGetNamedBufferParameteri64v(self_id(), gl::GL_BUFFER_MAP_OFFSET, &offset_bytes);
        return OffsetElems{ offset_bytes / sizeof(T) };
    }

    // Wraps `glGetNamedBufferParameteri64v` with `pname = GL_BUFFER_MAP_LENGTH` divided by element size.
    NumElems get_current_mapping_size() const noexcept {
        GLint64 size_bytes;
        gl::glGetNamedBufferParameteri64v(self_id(), gl::GL_BUFFER_MAP_LENGTH, &size_bytes);
        return NumElems{ size_bytes / sizeof(T) };
    }


private:
    GLuint get_mapping_access_flags() const noexcept {
        GLenum flags;
        gl::glGetNamedBufferParameteriv(self_id(), gl::GL_BUFFER_ACCESS_FLAGS, &flags);
        return enum_cast<GLuint>(flags);
    }
public:

    auto get_current_mapping_policies() const noexcept
        -> MappingPolicies
    {
        const GLuint flags = get_mapping_access_flags();

        constexpr GLuint access_mask            = GLuint(gl::GL_MAP_READ_BIT | gl::GL_MAP_WRITE_BIT);
        constexpr GLuint pending_mask           = GLuint(gl::GL_MAP_UNSYNCHRONIZED_BIT);
        constexpr GLuint flush_mask             = GLuint(gl::GL_MAP_FLUSH_EXPLICIT_BIT);
        constexpr GLuint previous_contents_mask = GLuint(gl::GL_MAP_INVALIDATE_BUFFER_BIT | gl::GL_MAP_INVALIDATE_RANGE_BIT);
        constexpr GLuint persistence_mask       = GLuint(gl::GL_MAP_PERSISTENT_BIT | gl::GL_MAP_COHERENT_BIT);

        return {
            .access             = MappingAccess    { flags & access_mask            },
            .pending_operations = PendingOperations{ flags & pending_mask           },
            .flush_policy       = FlushPolicy      { flags & flush_mask             },
            .previous_contents  = PreviousContents { flags & previous_contents_mask },
            .persistence        = Persistence      { flags & persistence_mask       },
        };
    }


    auto get_current_mapping_access() const noexcept
        -> MappingAccess
    {
        constexpr GLuint access_mask = GLuint(gl::GL_MAP_READ_BIT | gl::GL_MAP_WRITE_BIT);
        return MappingAccess{ get_mapping_access_flags() & access_mask };
    }


private:
    std::span<T> get_current_mapping_span_impl() const noexcept {
        NumElems num_elements = get_current_mapping_size();
        void* ptr;
        gl::glGetNamedBufferPointerv(self_id(), gl::GL_BUFFER_MAP_POINTER, &ptr);
        return { static_cast<T*>(ptr), num_elements };
    }
public:

    // Wraps `glGetNamedBufferPointerv` with `pname = GL_BUFFER_MAP_POINTER`.
    //
    // **Warning:** The current `MappingAccess` must be `Read` or `ReadWrite`, otherwise the behavior is undefined.
    // It is recommended to preserve the original span returned from `map[_range]_for_[read|write]` calls instead.
    [[nodiscard]]
    std::span<const T> get_current_mapping_span_for_read() const noexcept {
        assert(get_current_mapping_access() == MappingAccess::Read || get_current_mapping_access() == MappingAccess::ReadWrite);
        return get_current_mapping_span_impl();
    }

    // Wraps `glGetNamedBufferPointerv` with `pname = GL_BUFFER_MAP_POINTER`.
    //
    // **Warning:** The current `MappingAccess` must be `Write` or `ReadWrite`, otherwise the behavior is undefined.
    // It is recommended to preserve the original span returned from `map[_range]_for_[read|write]` calls instead.
    [[nodiscard]]
    std::span<T> get_current_mapping_span_for_write() const noexcept
        requires mt::is_mutable
    {
        assert(get_current_mapping_access() == MappingAccess::Write || get_current_mapping_access() == MappingAccess::ReadWrite);
        return get_current_mapping_span_impl();
    }

    // Wraps `glGetNamedBufferPointerv` with `pname = GL_BUFFER_MAP_POINTER`.
    //
    // **Warning:** The current `MappingAccess` must be `ReadWrite`, otherwise the behavior is undefined.
    // It is recommended to preserve the original span returned from `map[_range]_for_[read|write]` calls instead.
    [[nodiscard]]
    std::span<T> get_current_mapping_span_for_readwrite() const noexcept
        requires mt::is_mutable
    {
        assert(get_current_mapping_access() == MappingAccess::ReadWrite);
        return get_current_mapping_span_impl();
    }




    // Wraps `glUnmapNamedBuffer`.
    //
    // Returns `true` if unmapping succeded, `false` otherwise.
    //
    // " `glUnmapBuffer` returns `GL_TRUE` unless the data store contents
    // have become corrupt during the time the data store was mapped.
    // This can occur for system-specific reasons that affect
    // the availability of graphics memory, such as screen mode changes.
    // In such situations, `GL_FALSE` is returned and the data store contents
    // are undefined. The application must detect this rare condition
    // and reinitialize the data store."
    [[nodiscard]]
    bool unmap_current() const noexcept {
        GLboolean unmapping_succeded = gl::glUnmapNamedBuffer(self_id());
        return unmapping_succeded == gl::GL_TRUE;
    }

    // Wraps `glFlushMappedNamedBufferRange`.
    //
    // The buffer object must previously have been mapped with the
    // `BufferMapping[Read]WriteAccess` equal to one of the `*MustFlushExplicitly` options.
    void flush_mapped_range(
        OffsetElems elem_offset,
        NumElems    elem_count) const noexcept
            requires mt::is_mutable
    {
        gl::glFlushMappedNamedBufferRange(
            self_id(),
            elem_offset.value * sizeof(T),
            elem_count.value  * sizeof(T)
        );
    }

};








template<typename CRTP>
struct UntypedBufferDSAInterface
    : BufferDSAInterface_Bind<CRTP>
    , BufferDSAInterface_CommonQueries<CRTP>
{
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
    using mt = mutability_traits<CRTP>;
    using mutability = mt::mutability;
public:
    // Wraps `glInvalidateBufferData`.
    void invalidate_contents() const noexcept
        requires gl_mutable<mutability>
    {
        gl::glInvalidateBufferData(self_id());
    }

};








template<typename CRTP, typename T>
struct TypedBufferDSAInterface
    : BufferDSAInterface_Bind<CRTP>
    , BufferDSAInterface_Mapping<CRTP, T>
    , BufferDSAInterface_CommonQueries<CRTP>
{
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
    using mt = mutability_traits<CRTP>;
public:


    // Binding Subranges.

    // Wraps `glBindBufferRange`.
    template<BufferTargetIndexed TargetV>
    auto bind_range_to_index(
        OffsetElems elem_offset,
        NumElems    elem_count,
        GLuint      index) const noexcept
    {
        gl::glBindBufferRange(
            enum_cast<GLenum>(TargetV),
            index, self_id(),
            elem_offset.value * sizeof(T),
            elem_count.value  * sizeof(T)
        );
        if constexpr      (TargetV == BufferTargetIndexed::Uniform)           { return BindToken<BindingIndexed::UniformBuffer>          { self_id(), index }; }
        else if constexpr (TargetV == BufferTargetIndexed::ShaderStorage)     { return BindToken<BindingIndexed::ShaderStorageBuffer>    { self_id(), index }; }
        else if constexpr (TargetV == BufferTargetIndexed::TransformFeedback) { return BindToken<BindingIndexed::TransformFeedbackBuffer>{ self_id(), index }; }
        else if constexpr (TargetV == BufferTargetIndexed::AtomicCounter)     { return BindToken<BindingIndexed::AtomicCounterBuffer>    { self_id(), index }; }
        else { JOSH3D_STATIC_ASSERT_FALSE(CRTP); }
    }


    // Size Queries.

    // Wraps `glGetNamedBufferParameteri64v` with `pname = GL_BUFFER_SIZE`.
    //
    // Equivalent to `get_size_bytes()` divided by `sizeof(T)`.
    NumElems get_num_elements() const noexcept {
        return NumElems{ this->get_size_bytes() / sizeof(T) };
    }


    // Immutable Storage Allocation.

    // Wraps `glNamedBufferStorage` with `flags = storage_mode | mapping_policy | persistence_policy`.
    //
    // Creates immutable storage and initializes it with the contents of `src_buf`.
    void specify_storage(
        std::span<const T>   src_buf,
        StorageMode          storage_mode       = StorageMode::DynamicServer,
        PermittedMapping     mapping_policy     = PermittedMapping::ReadWrite,
        PermittedPersistence persistence_policy = PermittedPersistence::NotPersistent) const noexcept
            requires mt::is_mutable
    {
        gl::BufferStorageMask flags{
            to_underlying(storage_mode)       |
            to_underlying(mapping_policy)     |
            to_underlying(persistence_policy)
        };
        gl::glNamedBufferStorage(
            self_id(), src_buf.size_bytes(), src_buf.data(), flags
        );
    }

    // Wraps `glNamedBufferStorage` with `data = nullptr` and `flags = storage_mode | mapping_policy | persistence_policy`.
    //
    // Creates immutable storage leaving the contents undefined.
    void allocate_storage(
        NumElems             num_elements,
        StorageMode          storage_mode       = StorageMode::DynamicServer,
        PermittedMapping     mapping_policy     = PermittedMapping::ReadWrite,
        PermittedPersistence persistence_policy = PermittedPersistence::NotPersistent) const noexcept
            requires mt::is_mutable
    {
        gl::BufferStorageMask flags{
            to_underlying(storage_mode)       |
            to_underlying(mapping_policy)     |
            to_underlying(persistence_policy)
        };
        gl::glNamedBufferStorage(
            self_id(), num_elements.value * sizeof(T), nullptr, flags
        );
    }


    // Set/Get/Copy Buffer (Sub) Data.

    // Wraps `glNamedBufferSubData`.
    //
    // Will copy `src_buf.size()` elements from `src_buf` to this Buffer.
    void upload_data(
        std::span<const T> src_buf,
        OffsetElems        elem_offset = OffsetElems{ 0 }) const noexcept
            requires mt::is_mutable
    {
        gl::glNamedBufferSubData(
            self_id(),
            elem_offset.value * sizeof(T),
            src_buf.size_bytes(), src_buf.data()
        );
    }

    // Wraps `glGetNamedBufferSubData`.
    //
    // Will copy `dst_buf.size()` elements from this Buffer to `dst_buf`.
    void download_data_into(
        std::span<T> dst_buf,
        OffsetElems  elem_offset = OffsetElems{ 0 }) const noexcept
    {
        gl::glGetNamedBufferSubData(
            self_id(),
            elem_offset.value * sizeof(T),
            dst_buf.size_bytes(), dst_buf.data()
        );
    }

    // Wraps `glCopyNamedBufferSubData`.
    //
    // Will copy `src_elem_count` elements from this Buffer to `dst_buffer`.
    // No alignment or layout is considered. Copies bytes directly, similar to `memcpy`.
    template<typename DstT = T>
    void copy_data_to(
        mt::template type_template<DstT, GLMutable> dst_buffer,
        NumElems                                    src_elem_count,
        OffsetElems                                 src_elem_offset = OffsetElems{ 0 },
        OffsetElems                                 dst_elem_offset = OffsetElems{ 0 }) const noexcept
    {
        gl::glCopyNamedBufferSubData(
            self_id(),
            dst_buffer.id(),
            src_elem_offset.value * sizeof(T),
            dst_elem_offset.value * sizeof(DstT),
            src_elem_count.value  * sizeof(T)
        );
    }


    // Buffer Data Invalidation.

    // Wraps `glInvalidateBufferData`.
    void invalidate_contents() const noexcept
        requires mt::is_mutable
    {
        gl::glInvalidateBufferData(self_id());
    }

    // Wraps `glInvalidateBufferSubData`.
    void invalidate_subrange(
        OffsetElems elem_offset,
        NumElems    elem_count) const noexcept
            requires mt::is_mutable
    {
        gl::glInvalidateBufferSubData(
            self_id(),
            elem_offset.value * sizeof(T),
            elem_count.value  * sizeof(T)
        );
    }


};


} // namespace detail








template<trivially_copyable T, mutability_tag MutT = GLMutable>
class RawBuffer
    : public detail::RawGLHandle<MutT>
    , public detail::TypedBufferDSAInterface<RawBuffer<T, MutT>, T>
{
public:
    static constexpr GLKind kind_type = GLKind::Buffer;
    JOSH3D_MAGIC_CONSTRUCTORS_2(RawBuffer, mutability_traits<RawBuffer>, detail::RawGLHandle<MutT>)


    // Disable "type punning" through buffers. Gives better diagnostics.
    template<typename U, mutability_tag MutU>
    RawBuffer(const RawBuffer<U, MutU>& other) = delete;
    template<typename U, mutability_tag MutU>
    RawBuffer& operator=(const RawBuffer<U, MutU>& other) = delete;
};




template<trivially_copyable T, mutability_tag MutT>
struct mutability_traits<RawBuffer<T, MutT>> {
    using mutability                 = MutT;
    using opposite_mutability        = MutT::opposite_mutability;
    template<typename ...ArgTs>
    using type_template              = RawBuffer<ArgTs...>;
    using const_type                 = RawBuffer<T, GLConst>;
    using mutable_type               = RawBuffer<T, GLMutable>;
    using opposite_type              = RawBuffer<T, opposite_mutability>;
    static constexpr bool is_mutable = gl_mutable<mutability>;
    static constexpr bool is_const   = gl_const<mutability>;
};




template<mutability_tag MutT = GLMutable>
class RawUntypedBuffer
    : public detail::RawGLHandle<MutT>
    , public detail::UntypedBufferDSAInterface<RawUntypedBuffer<MutT>>
{
public:
    static constexpr GLKind kind_type = GLKind::Buffer;
    JOSH3D_MAGIC_CONSTRUCTORS_2(RawUntypedBuffer, mutability_traits<RawUntypedBuffer>, detail::RawGLHandle<MutT>)


    template<convertible_mutability_to<MutT> MutU, typename T>
    RawUntypedBuffer(RawBuffer<T, MutU> typed_buffer)
        : RawUntypedBuffer{ typed_buffer.id() }
    {}

    // Explicit cast to a typed buffer, similar to a `static_cast` from a `void*`.
    template<typename T>
    auto as_typed() const noexcept
        -> RawBuffer<T, MutT>
    {
        return RawBuffer<T, MutT>::from_id(this->id());
    }

};




} // namespace josh
