#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep
#include "GLAPI.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPITargets.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLKind.hpp"
#include "GLScalars.hpp"
#include "GLMutability.hpp"
#include "EnumUtils.hpp"
#include "detail/MagicConstructorsMacro.hpp"
#include "detail/MixinHeaderMacro.hpp"
#include "detail/RawGLHandle.hpp"
#include <span>
#include <cassert>


namespace josh {


template<trivially_copyable T, mutability_tag MutT>
class RawBuffer;

/*
FIXME: This really should be simpler than this.
*/
template<mutability_tag MutT>
class RawUntypedBuffer;


enum class StorageMode : GLuint
{
    StaticServer  = GLuint(gl::GL_NONE_BIT),            // STATIC_DRAW
    DynamicServer = GLuint(gl::GL_DYNAMIC_STORAGE_BIT), // DYNAMIC_DRAW
    StaticClient  = GLuint(gl::GL_CLIENT_STORAGE_BIT),  // STATIC_READ
    DynamicClient = GLuint(gl::GL_DYNAMIC_STORAGE_BIT | gl::GL_CLIENT_STORAGE_BIT) // DYNAMIC_READ
};
JOSH3D_DEFINE_ENUM_EXTRAS(StorageMode, StaticServer, DynamicServer, StaticClient, DynamicClient);

enum class PermittedMapping : GLuint
{
    NoMapping                   = GLuint(gl::GL_NONE_BIT),
    Read                        = GLuint(gl::GL_MAP_READ_BIT),
    Write                       = GLuint(gl::GL_MAP_WRITE_BIT),
    ReadWrite                   = Read | Write,
};
JOSH3D_DEFINE_ENUM_EXTRAS(PermittedMapping, NoMapping, Read, Write, ReadWrite);

enum class PermittedPersistence : GLuint
{
    NotPersistent      = GLuint(gl::GL_NONE_BIT),
    Persistent         = GLuint(gl::GL_MAP_PERSISTENT_BIT),
    PersistentCoherent = Persistent | GLuint(gl::GL_MAP_COHERENT_BIT),
};
JOSH3D_DEFINE_ENUM_EXTRAS(PermittedPersistence, NotPersistent, Persistent, PersistentCoherent);

struct StoragePolicies
{
    StorageMode          mode        = StorageMode::DynamicServer;
    PermittedMapping     mapping     = PermittedMapping::ReadWrite;
    PermittedPersistence persistence = PermittedPersistence::NotPersistent;
};


/*
Buffer mapping flags but split so that you can't pass the wrong combination
for each respective mapping access.
*/

enum class MappingAccess : GLuint
{
    Read      = GLuint(gl::GL_MAP_READ_BIT),
    Write     = GLuint(gl::GL_MAP_WRITE_BIT),
    ReadWrite = Read | Write,
};
JOSH3D_DEFINE_ENUM_EXTRAS(MappingAccess, Read, Write, ReadWrite);

enum class PendingOperations : GLuint
{
    SynchronizeOnMap = GLuint(gl::GL_NONE_BIT),
    DoNotSynchronize = GLuint(gl::GL_MAP_UNSYNCHRONIZED_BIT),
};
JOSH3D_DEFINE_ENUM_EXTRAS(PendingOperations, SynchronizeOnMap, DoNotSynchronize);

/*
Do you have one?
*/
enum class FlushPolicy : GLuint
{
    AutomaticOnUnmap   = GLuint(gl::GL_NONE_BIT),
    MustFlushExplicity = GLuint(gl::GL_MAP_FLUSH_EXPLICIT_BIT),
};
JOSH3D_DEFINE_ENUM_EXTRAS(FlushPolicy, AutomaticOnUnmap, MustFlushExplicity);

enum class PreviousContents : GLuint
{
    DoNotInvalidate       = GLuint(gl::GL_NONE_BIT),
    InvalidateAll         = GLuint(gl::GL_MAP_INVALIDATE_BUFFER_BIT),
    InvalidateMappedRange = GLuint(gl::GL_MAP_INVALIDATE_RANGE_BIT),
};
JOSH3D_DEFINE_ENUM_EXTRAS(PreviousContents, DoNotInvalidate, InvalidateAll, InvalidateMappedRange);

enum class Persistence : GLuint
{
    NotPersistent      = GLuint(gl::GL_NONE_BIT),
    Persistent         = GLuint(gl::GL_MAP_PERSISTENT_BIT),
    PersistentCoherent = Persistent | GLuint(gl::GL_MAP_COHERENT_BIT),
};
JOSH3D_DEFINE_ENUM_EXTRAS(Persistence, NotPersistent, Persistent, PersistentCoherent);


struct MappingReadPolicies
{
    PendingOperations pending_ops = PendingOperations::SynchronizeOnMap;
    Persistence       persistence = Persistence::NotPersistent;
};

struct MappingWritePolicies
{
    PendingOperations pending_ops       = PendingOperations::SynchronizeOnMap;
    FlushPolicy       flush_policy      = FlushPolicy::AutomaticOnUnmap;
    PreviousContents  previous_contents = PreviousContents::DoNotInvalidate;
    Persistence       persistence       = Persistence::NotPersistent;
};

struct MappingReadWritePolicies
{
    PendingOperations pending_ops  = PendingOperations::SynchronizeOnMap;
    FlushPolicy       flush_policy = FlushPolicy::AutomaticOnUnmap;
    Persistence       persistence  = Persistence::NotPersistent;
};

/*
Return type for querying all policies at once.
*/
struct MappingPolicies
{
    MappingAccess     access;
    PendingOperations pending_ops       = PendingOperations::SynchronizeOnMap;
    FlushPolicy       flush_policy      = FlushPolicy::AutomaticOnUnmap;
    PreviousContents  previous_contents = PreviousContents::DoNotInvalidate;
    Persistence       persistence       = Persistence::NotPersistent;
};


namespace detail {
namespace buffer_api {

template<typename T>
auto _map_buffer_range(
    GLuint object_id, GLintptr elem_offset, GLsizeiptr elem_count,
    gl::BufferAccessMask access, gl::BufferAccessMask rw_bits)
        -> std::span<T>
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
        gl::glMapNamedBufferRange(object_id,
            elem_offset * sizeof(T), elem_count * sizeof(T), access);

    return { static_cast<T*>(buf), elem_count };
}


template<typename CRTP>
struct CommonQueries
{
    JOSH3D_MIXIN_HEADER

    // Wraps `glGetNamedBufferParameteri64v` with `pname = GL_BUFFER_SIZE`.
    auto get_size_bytes() const -> GLsizeiptr
    {
        GLint64 size;
        gl::glGetNamedBufferParameteri64v(
            self_id(), gl::GL_BUFFER_SIZE, &size);
        return size;
    }

    // Wraps `glGetNamedBufferParameteriv` with `pname = GL_BUFFER_STORAGE_FLAGS`.
    auto get_storage_policies() const
        -> StoragePolicies
    {
        const GLuint flags = _get_storage_mode_flags();

        constexpr GLuint mode_mask        = GLuint(gl::GL_DYNAMIC_STORAGE_BIT | gl::GL_CLIENT_STORAGE_BIT);
        constexpr GLuint mapping_mask     = GLuint(gl::GL_MAP_READ_BIT | gl::GL_MAP_WRITE_BIT);
        constexpr GLuint persistence_mask = GLuint(gl::GL_MAP_PERSISTENT_BIT | gl::GL_MAP_COHERENT_BIT);

        return {
            .mode          = StorageMode         { flags & mode_mask        },
            .mapping     = PermittedMapping    { flags & mapping_mask     },
            .persistence = PermittedPersistence{ flags & persistence_mask },
        };
    }

private:
    auto _get_storage_mode_flags() const -> GLuint
    {
        GLenum flags;
        gl::glGetNamedBufferParameteriv(self_id(), gl::GL_BUFFER_STORAGE_FLAGS, &flags);
        return enum_cast<GLuint>(flags);
    }
};


template<typename CRTP>
struct Bind
{
    JOSH3D_MIXIN_HEADER

    // Wraps `glBindBuffer`.
    template<BufferTarget TargetV>
    [[nodiscard("BindTokens have to be provided to an API call that expects bound state.")]]
    auto bind() const -> BindToken<target_binding(TargetV)>
    {
        return glapi::bind_to_context<target_binding(TargetV)>(self_id());
    }

    // Wraps `glBindBufferBase`.
    template<BufferTargetI TargetV>
    // [[nodiscard("Discarding bound state is error-prone. Consider using BindGuard to automate unbinding.")]]
    auto bind_to_index(GLuint index) const -> BindToken<target_binding_indexed(TargetV)>
    {
        return glapi::bind_to_context<target_binding_indexed(TargetV)>(index, self_id());
    }
};


template<typename CRTP, typename T>
struct Mapping
{
    JOSH3D_MIXIN_HEADER

    // Wraps `glMapNamedBufferRange` with `access = GL_MAP_READ_BIT | [flags]`.
    [[nodiscard]] auto map_range_for_read(
        const ElemRange&           elem_range,
        const MappingReadPolicies& policies = {}) const
            -> std::span<const T>
    {
        gl::BufferAccessMask access{
            to_underlying(policies.pending_ops) |
            to_underlying(policies.persistence)
        };
        const auto& [offset, count] = elem_range;
        return _map_buffer_range<T>(self_id(), offset, count, access, gl::GL_MAP_READ_BIT);
    }

    // Wraps `glMapNamedBufferRange` with `offset = 0`, `length = get_size_bytes()`
    // and `access = GL_MAP_READ_BIT | [flags]`.
    //
    // Maps the entire buffer.
    [[nodiscard]] auto map_for_read(
        const MappingReadPolicies& policies = {}) const
            -> std::span<const T>
    {
        const ElemRange whole_buffer_range{ OffsetElems{ 0 }, self().get_num_elements() };
        return map_range_for_read(whole_buffer_range, policies);
    }

    // Wraps `glMapNamedBufferRange` with `access = GL_MAP_WRITE_BIT | [flags]`.
    [[nodiscard]] auto map_range_for_write(
        const ElemRange&            elem_range,
        const MappingWritePolicies& policies = {}) const
            -> std::span<T>
                requires mt::is_mutable
    {
        gl::BufferAccessMask access{
            to_underlying(policies.pending_ops)       |
            to_underlying(policies.flush_policy)      |
            to_underlying(policies.previous_contents) |
            to_underlying(policies.persistence)
        };
        const auto& [offset, count] = elem_range;
        return _map_buffer_range<T>(self_id(), offset, count, access, gl::GL_MAP_WRITE_BIT);
    }

    // Wraps `glMapNamedBufferRange` with `offset = 0`, `length = get_size_bytes()`
    // and `access = GL_MAP_WRITE_BIT | [flags]`.
    //
    // Maps the entire buffer.
    [[nodiscard]] auto map_for_write(
        const MappingWritePolicies& policies = {}) const
            -> std::span<T>
                requires mt::is_mutable
    {
        const ElemRange whole_buffer_range{ OffsetElems{ 0 }, self().get_num_elements() };
        return map_range_for_write(whole_buffer_range, policies);
    }

    // Wraps `glMapNamedBufferRange` with `access = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | [flags]`.
    [[nodiscard]] auto map_range_for_readwrite(
        const ElemRange&                elem_range,
        const MappingReadWritePolicies& policies = {}) const
            -> std::span<T>
                requires mt::is_mutable
    {
        gl::BufferAccessMask access{
            to_underlying(policies.pending_ops)  |
            to_underlying(policies.flush_policy) |
            to_underlying(policies.persistence)
        };
        const auto& [offset, count] = elem_range;
        return _map_buffer_range<T>(self_id(), offset, count, access, (gl::GL_MAP_READ_BIT | gl::GL_MAP_WRITE_BIT));
    }

    // Wraps `glMapNamedBufferRange` with `offset = 0`, `length = get_size_bytes()`
    // and `access = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | [flags]`.
    //
    // Maps the entire buffer.
    [[nodiscard]] auto map_for_readwrite(
        const MappingReadWritePolicies& policies = {}) const
            -> std::span<T>
                requires mt::is_mutable
    {
        const ElemRange whole_buffer_range{ OffsetElems{ 0 }, self().get_num_elements() };
        return map_range_for_readwrite(whole_buffer_range, policies);
    }

    // Wraps `glGetNamedBufferParameteriv` with `pname = GL_BUFFER_MAPPED`.
    auto is_currently_mapped() const -> bool
    {
        GLboolean is_mapped;
        gl::glGetNamedBufferParameteriv(self_id(), gl::GL_BUFFER_MAPPED, &is_mapped);
        return bool(is_mapped);
    }

    // Wraps `glGetNamedBufferParameteri64v` with `pname = GL_BUFFER_MAP_OFFSET` divided by element size.
    auto get_current_mapping_offset() const -> OffsetElems
    {
        GLint64 offset_bytes;
        gl::glGetNamedBufferParameteri64v(self_id(), gl::GL_BUFFER_MAP_OFFSET, &offset_bytes);
        return OffsetElems{ offset_bytes / sizeof(T) };
    }

    // Wraps `glGetNamedBufferParameteri64v` with `pname = GL_BUFFER_MAP_LENGTH` divided by element size.
    auto get_current_mapping_size() const -> NumElems
    {
        GLint64 size_bytes;
        gl::glGetNamedBufferParameteri64v(self_id(), gl::GL_BUFFER_MAP_LENGTH, &size_bytes);
        return NumElems{ size_bytes / sizeof(T) };
    }

    auto get_current_mapping_policies() const -> MappingPolicies
    {
        const GLuint flags = _get_mapping_access_flags();

        constexpr GLuint access_mask            = GLuint(gl::GL_MAP_READ_BIT | gl::GL_MAP_WRITE_BIT);
        constexpr GLuint pending_mask           = GLuint(gl::GL_MAP_UNSYNCHRONIZED_BIT);
        constexpr GLuint flush_mask             = GLuint(gl::GL_MAP_FLUSH_EXPLICIT_BIT);
        constexpr GLuint previous_contents_mask = GLuint(gl::GL_MAP_INVALIDATE_BUFFER_BIT | gl::GL_MAP_INVALIDATE_RANGE_BIT);
        constexpr GLuint persistence_mask       = GLuint(gl::GL_MAP_PERSISTENT_BIT | gl::GL_MAP_COHERENT_BIT);

        return {
            .access             = MappingAccess    { flags & access_mask            },
            .pending_ops        = PendingOperations{ flags & pending_mask           },
            .flush_policy       = FlushPolicy      { flags & flush_mask             },
            .previous_contents  = PreviousContents { flags & previous_contents_mask },
            .persistence        = Persistence      { flags & persistence_mask       },
        };
    }

    auto get_current_mapping_access() const -> MappingAccess
    {
        constexpr GLuint access_mask = GLuint(gl::GL_MAP_READ_BIT | gl::GL_MAP_WRITE_BIT);
        return MappingAccess{ _get_mapping_access_flags() & access_mask };
    }

    // Wraps `glGetNamedBufferPointerv` with `pname = GL_BUFFER_MAP_POINTER`.
    //
    // **Warning:** The current `MappingAccess` must be `Read` or `ReadWrite`, otherwise the behavior is undefined.
    // It is recommended to preserve the original span returned from `map[_range]_for_[read|write]` calls instead.
    [[nodiscard]]
    auto get_current_mapping_span_for_read() const -> std::span<const T>
    {
        assert(get_current_mapping_access() == MappingAccess::Read or
            get_current_mapping_access() == MappingAccess::ReadWrite);
        return _get_current_mapping_span();
    }

    // Wraps `glGetNamedBufferPointerv` with `pname = GL_BUFFER_MAP_POINTER`.
    //
    // **Warning:** The current `MappingAccess` must be `Write` or `ReadWrite`, otherwise the behavior is undefined.
    // It is recommended to preserve the original span returned from `map[_range]_for_[read|write]` calls instead.
    [[nodiscard]]
    auto get_current_mapping_span_for_write() const -> std::span<T>
        requires mt::is_mutable
    {
        assert(get_current_mapping_access() == MappingAccess::Write or
            get_current_mapping_access() == MappingAccess::ReadWrite);
        return _get_current_mapping_span();
    }

    // Wraps `glGetNamedBufferPointerv` with `pname = GL_BUFFER_MAP_POINTER`.
    //
    // **Warning:** The current `MappingAccess` must be `ReadWrite`, otherwise the behavior is undefined.
    // It is recommended to preserve the original span returned from `map[_range]_for_[read|write]` calls instead.
    [[nodiscard]]
    auto get_current_mapping_span_for_readwrite() const -> std::span<T>
        requires mt::is_mutable
    {
        assert(get_current_mapping_access() == MappingAccess::ReadWrite);
        return _get_current_mapping_span();
    }

    // Wraps `glUnmapNamedBuffer`.
    //
    // Returns `true` if unmapping succeded, `false` otherwise.
    //
    // "`glUnmapBuffer` returns `GL_TRUE` unless the data store contents
    // have become corrupt during the time the data store was mapped.
    // This can occur for system-specific reasons that affect
    // the availability of graphics memory, such as screen mode changes.
    // In such situations, `GL_FALSE` is returned and the data store contents
    // are undefined. The application must detect this rare condition
    // and reinitialize the data store."
    [[nodiscard]]
    auto unmap_current() const -> bool
    {
        GLboolean unmapping_succeded = gl::glUnmapNamedBuffer(self_id());
        return unmapping_succeded == gl::GL_TRUE;
    }

    // Wraps `glFlushMappedNamedBufferRange`.
    //
    // PRE: The buffer object must previously have been mapped with the
    // `BufferMapping[Read]WriteAccess` equal to one of the `*MustFlushExplicitly` options.
    void flush_mapped_range(ElemRange elem_range) const requires mt::is_mutable
    {
        const auto& [offset, count] = elem_range;
        gl::glFlushMappedNamedBufferRange(self_id(), offset * sizeof(T), count * sizeof(T));
    }

private:
    auto _get_mapping_access_flags() const -> GLuint
    {
        GLenum flags;
        gl::glGetNamedBufferParameteriv(self_id(), gl::GL_BUFFER_ACCESS_FLAGS, &flags);
        return enum_cast<GLuint>(flags);
    }

    auto _get_current_mapping_span() const -> std::span<T>
    {
        NumElems num_elements = get_current_mapping_size();
        void* ptr;
        gl::glGetNamedBufferPointerv(self_id(), gl::GL_BUFFER_MAP_POINTER, &ptr);
        return { static_cast<T*>(ptr), num_elements };
    }
};


/*
FIXME: This should probably just be a byte buffer.
*/
template<typename CRTP>
struct VoidBuffer
    : Bind<CRTP>
    , CommonQueries<CRTP>
{
    JOSH3D_MIXIN_HEADER

    // Wraps `glInvalidateBufferData`.
    void invalidate_contents() const requires mt::is_mutable
    {
        gl::glInvalidateBufferData(self_id());
    }
};


template<typename CRTP, typename T>
struct Buffer
    : Bind<CRTP>
    , Mapping<CRTP, T>
    , CommonQueries<CRTP>
{
    JOSH3D_MIXIN_HEADER

    // Binding Subranges.

    // Wraps `glBindBufferRange`.
    template<BufferTargetI TargetV>
    auto bind_range_to_index(
        OffsetElems elem_offset,
        NumElems    elem_count,
        GLuint      index) const
    {
        gl::glBindBufferRange(
            enum_cast<GLenum>(TargetV),
            index, self_id(),
            elem_offset.value * sizeof(T),
            elem_count.value  * sizeof(T)
        );
        return BindToken<target_binding_indexed(TargetV)>::from_index_and_id(index, self_id());
    }

    // Size Queries.

    // Wraps `glGetNamedBufferParameteri64v` with `pname = GL_BUFFER_SIZE`.
    //
    // Equivalent to `get_size_bytes()` divided by `sizeof(T)`.
    auto get_num_elements() const -> NumElems
    {
        return NumElems{ this->get_size_bytes() / sizeof(T) };
    }

    // Immutable Storage Allocation.

    // Wraps `glNamedBufferStorage` with `flags = mode | mapping | persistence`.
    //
    // Creates immutable storage and initializes it with the contents of `src_buf`.
    void specify_storage(
        std::span<const T>     src_buf,
        const StoragePolicies& policies) const
            requires mt::is_mutable
    {
        gl::BufferStorageMask flags{
            to_underlying(policies.mode)        |
            to_underlying(policies.mapping)     |
            to_underlying(policies.persistence)
        };
        gl::glNamedBufferStorage(self_id(), src_buf.size_bytes(), src_buf.data(), flags);
    }

    // Wraps `glNamedBufferStorage` with `data = nullptr` and `flags = mode | mapping | persistence`.
    //
    // Creates immutable storage leaving the contents undefined.
    void allocate_storage(
        NumElems               num_elements,
        const StoragePolicies& policies) const
            requires mt::is_mutable
    {
        gl::BufferStorageMask flags{
            to_underlying(policies.mode)        |
            to_underlying(policies.mapping)     |
            to_underlying(policies.persistence)
        };
        gl::glNamedBufferStorage(self_id(), num_elements.value * sizeof(T), nullptr, flags);
    }


    // Set/Get/Copy Buffer (Sub) Data.

    // Wraps `glNamedBufferSubData`.
    //
    // Will copy `src_buf.size()` elements from `src_buf` to this Buffer.
    void upload_data(
        std::span<const T> src_buf,
        OffsetElems        elem_offset = OffsetElems{ 0 }) const
            requires mt::is_mutable
    {
        gl::glNamedBufferSubData(self_id(),
            elem_offset.value * sizeof(T), src_buf.size_bytes(), src_buf.data());
    }

    // Wraps `glGetNamedBufferSubData`.
    //
    // Will copy `dst_buf.size()` elements from this Buffer to `dst_buf`.
    void download_data_into(
        std::span<T> dst_buf,
        OffsetElems  elem_offset = OffsetElems{ 0 }) const
    {
        gl::glGetNamedBufferSubData(self_id(),
            elem_offset.value * sizeof(T), dst_buf.size_bytes(), dst_buf.data());
    }

    // Wraps `glCopyNamedBufferSubData`.
    //
    // Will copy `src_elem_count` elements from this Buffer to `dst_buffer`.
    // No alignment or layout is considered. Copies bytes directly, similar to `memcpy`.
    //
    // TODO: Template parameter here suppresses implicit conversion from UniqueBuffer -> RawBuffer.
    template<typename DstT = T>
    void copy_data_to(
        mt::template type_template<DstT, GLMutable> dst_buffer,
        NumElems                                    src_elem_count,
        OffsetElems                                 src_elem_offset = OffsetElems{ 0 },
        OffsetElems                                 dst_elem_offset = OffsetElems{ 0 }) const
    {
        gl::glCopyNamedBufferSubData(
            self_id(), dst_buffer.id(),
            src_elem_offset.value * sizeof(T),
            dst_elem_offset.value * sizeof(DstT),
            src_elem_count.value  * sizeof(T));
    }

    // Buffer Data Invalidation.

    // Wraps `glInvalidateBufferData`.
    void invalidate_contents() const requires mt::is_mutable
    {
        gl::glInvalidateBufferData(self_id());
    }

    // Wraps `glInvalidateBufferSubData`.
    void invalidate_subrange(ElemRange elem_range) const requires mt::is_mutable
    {
        const auto [offset, count] = elem_range;
        gl::glInvalidateBufferSubData(self_id(),
            offset.value * sizeof(T), count.value  * sizeof(T));
    }
};


} // namespace buffer_api
} // namespace detail




template<trivially_copyable T, mutability_tag MutT = GLMutable>
class RawBuffer
    : public detail::RawGLHandle<>
    , public detail::buffer_api::Buffer<RawBuffer<T, MutT>, T>
{
public:
    static constexpr auto kind_type = GLKind::Buffer;
    JOSH3D_MAGIC_CONSTRUCTORS_2(RawBuffer, mutability_traits<RawBuffer>, detail::RawGLHandle<>)

    // Disable "type punning" through buffers. Gives better diagnostics.
    template<typename U, mutability_tag MutU>
    RawBuffer(const RawBuffer<U, MutU>& other) = delete;
    template<typename U, mutability_tag MutU>
    RawBuffer& operator=(const RawBuffer<U, MutU>& other) = delete;
};


template<trivially_copyable T, mutability_tag MutT>
struct mutability_traits<RawBuffer<T, MutT>>
{
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
    : public detail::RawGLHandle<>
    , public detail::buffer_api::VoidBuffer<RawUntypedBuffer<MutT>>
{
public:
    static constexpr auto kind_type = GLKind::Buffer;
    JOSH3D_MAGIC_CONSTRUCTORS_2(RawUntypedBuffer, mutability_traits<RawUntypedBuffer>, detail::RawGLHandle<>)

    template<convertible_mutability_to<MutT> MutU, typename T>
    RawUntypedBuffer(RawBuffer<T, MutU> typed_buffer)
        : RawUntypedBuffer{ typed_buffer.id() }
    {}

    // Explicit cast to a typed buffer, similar to a `static_cast` from a `void*`.
    template<typename T>
    auto as_typed() const -> RawBuffer<T, MutT>
    {
        return RawBuffer<T, MutT>::from_id(this->id());
    }

};




} // namespace josh
