#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep
#include "GLAPI.hpp"
#include "GLKind.hpp"
#include "GLScalars.hpp"
#include "GLMutability.hpp"
#include "EnumUtils.hpp"
#include "detail/MagicConstructorsMacro.hpp"
#include "detail/RawGLHandle.hpp"
#include <glbinding/gl/bitfield.h>
#include <span>
#include <cassert>


namespace josh::dsa {




template<trivially_copyable T, mutability_tag MutT>
class RawBuffer;
template<mutability_tag MutT>
class RawUntypedBuffer;




enum class BufferStorageType {
    Mutable,
    Immutable,
};


enum class BufferStorageMode : GLuint {
    StaticServer  = GLuint(gl::GL_NONE_BIT),            // STATIC_DRAW
    DynamicServer = GLuint(gl::GL_DYNAMIC_STORAGE_BIT), // DYNAMIC_DRAW
    StaticClient  = GLuint(gl::GL_CLIENT_STORAGE_BIT),  // STATIC_READ
    DynamicClient = GLuint(gl::GL_DYNAMIC_STORAGE_BIT | gl::GL_CLIENT_STORAGE_BIT) // DYNAMIC_READ
};


enum class BufferUsageHint : GLuint {
    StaticDraw  = GLuint(gl::GL_STATIC_DRAW),
    StaticRead  = GLuint(gl::GL_STATIC_READ),
    StaticCopy  = GLuint(gl::GL_STATIC_COPY),
    DynamicDraw = GLuint(gl::GL_DYNAMIC_DRAW),
    DynamicRead = GLuint(gl::GL_DYNAMIC_READ),
    DynamicCopy = GLuint(gl::GL_DYNAMIC_COPY),
    StreamDraw  = GLuint(gl::GL_STREAM_DRAW),
    StreamRead  = GLuint(gl::GL_STREAM_READ),
    StreamCopy  = GLuint(gl::GL_STREAM_COPY),
};


enum class BufferStoragePermittedMapping : GLuint {
    NoMapping                   = GLuint(gl::GL_NONE_BIT),

    Read                        = GLuint(gl::GL_MAP_READ_BIT),
    Write                       = GLuint(gl::GL_MAP_WRITE_BIT),
    ReadWrite                   = Read | Write,

    ReadPersistent              = Read      | GLuint(gl::GL_MAP_PERSISTENT_BIT),
    WritePersistent             = Write     | GLuint(gl::GL_MAP_PERSISTENT_BIT),
    ReadWritePersistent         = ReadWrite | GLuint(gl::GL_MAP_PERSISTENT_BIT),

    ReadPersistentCoherent      = ReadPersistent      | GLuint(gl::GL_MAP_COHERENT_BIT),
    WritePersistentCoherent     = WritePersistent     | GLuint(gl::GL_MAP_COHERENT_BIT),
    ReadWritePersistentCoherent = ReadWritePersistent | GLuint(gl::GL_MAP_COHERENT_BIT),
};


enum class BufferMappingPersistence : GLuint {
    NotPersistent      = GLuint(gl::GL_NONE_BIT),
    Persistent         = GLuint(gl::GL_MAP_PERSISTENT_BIT),
    PersistentCoherent = Persistent | GLuint(gl::GL_MAP_COHERENT_BIT),
};


enum class BufferMappingReadAccess : GLuint {
    Synchronized   = GLuint(gl::GL_NONE_BIT),
    Unsynchronized = GLuint(gl::GL_MAP_UNSYNCHRONIZED_BIT),
};


enum class BufferMappingWriteAccess : GLuint {
    Synchronized                      = GLuint(gl::GL_NONE_BIT),
    Unsynchronized                    = GLuint(gl::GL_MAP_UNSYNCHRONIZED_BIT),
    SynchronizedMustFlushExplicitly   = Synchronized | GLuint(gl::GL_MAP_FLUSH_EXPLICIT_BIT),
    UnsynchronizedMustFlushExplicitly = Unsynchronized | GLuint(gl::GL_MAP_FLUSH_EXPLICIT_BIT),
};


enum class BufferMappingReadWriteAccess : GLuint {
    Synchronized                      = GLuint(gl::GL_NONE_BIT),
    Unsynchronized                    = GLuint(gl::GL_MAP_UNSYNCHRONIZED_BIT),
    SynchronizedMustFlushExplicitly   = Synchronized | GLuint(gl::GL_MAP_FLUSH_EXPLICIT_BIT),
    UnsynchronizedMustFlushExplicitly = Unsynchronized | GLuint(gl::GL_MAP_FLUSH_EXPLICIT_BIT),
};


enum class BufferMappingPreviousContents : GLuint {
    DoNotInvalidate       = GLuint(gl::GL_NONE_BIT),
    InvalidateAll         = GLuint(gl::GL_MAP_INVALIDATE_BUFFER_BIT),
    InvalidateMappedRange = GLuint(gl::GL_MAP_INVALIDATE_RANGE_BIT),
};




enum class BufferTarget : GLuint {
    VertexArray      = GLuint(gl::GL_ARRAY_BUFFER),
    ElementArray     = GLuint(gl::GL_ELEMENT_ARRAY_BUFFER),
    DispatchIndirect = GLuint(gl::GL_DISPATCH_INDIRECT_BUFFER),
    DrawIndirect     = GLuint(gl::GL_DRAW_INDIRECT_BUFFER),
    PixelPack        = GLuint(gl::GL_PIXEL_PACK_BUFFER),
    PixelUnpack      = GLuint(gl::GL_PIXEL_UNPACK_BUFFER),
    Texture          = GLuint(gl::GL_TEXTURE_BUFFER),
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
    assert(bool(access & gl::GL_MAP_UNSYNCHRONIZED_BIT)    && !bool(access & gl::GL_MAP_READ_BIT) &&
        "Not sure why, but this flag may not be used in combination with GL_MAP_READ_BIT.");
    assert(bool(access & gl::GL_MAP_INVALIDATE_BUFFER_BIT) && !bool(access & gl::GL_MAP_READ_BIT) &&
        "This flag may not be used in combination with GL_MAP_READ_BIT.");
    assert(bool(access & gl::GL_MAP_INVALIDATE_RANGE_BIT)  && !bool(access & gl::GL_MAP_READ_BIT) &&
        "This flag may not be used in combination with GL_MAP_READ_BIT.");
    assert(bool(access & gl::GL_MAP_FLUSH_EXPLICIT_BIT)    &&  bool(access & gl::GL_MAP_WRITE_BIT) &&
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

    // Wraps `glGetNamedBufferParameteri64v` with `pname = GL_BUFFER_IMMUTABLE_STORAGE`.
    BufferStorageType get_storage_type() const noexcept {
        GLboolean is_immutable;
        gl::glGetNamedBufferParameteriv(
            self_id(), gl::GL_BUFFER_IMMUTABLE_STORAGE, &is_immutable
        );
        return is_immutable == gl::GL_TRUE ?
            BufferStorageType::Immutable :
            BufferStorageType::Mutable;
    }
};



/*
TODO: Now that I think about it, we could completely invert
the responsibility of binding, and delegate it to some
context object instead.

So that instead of individual objects supporting some bind
function:

    RawBuffer<GLMutable, float> buf;
    buf.bind_to_index<BufferTarget::shader_storage>(2);

We would instead have a Context type which accepts GL objects
for binding:

    GLContext context;
    RawBuffer<GLMutable, float> buf;

    context.ssbo_slots().bind_to_index(buf, 2);
    context.active_shader_program().bind(shp);
    context.draw_framebuffer().bind(fbo);
    context.vertex_attributes().bind(vao);
    context.draw_elements();
    context.ssbo_slots().unbind_at_index(2);
*/
template<typename CRTP>
struct BufferDSAInterface_Bindable {
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
public:
    // Wraps `glBindBuffer`.
    template<BufferTarget TargetV>
    void bind() const noexcept {
        gl::glBindBuffer(
            GLenum{ to_underlying(TargetV) }, self_id()
        );
    }

    // Wraps `glBindBufferBase`.
    template<BufferTargetIndexed TargetV>
    void bind_to_index(GLuint index) const noexcept {
        gl::glBindBufferBase(
            GLenum{ to_underlying(TargetV) }, index, self_id()
        );
    }

};






template<typename CRTP>
struct UntypedBufferDSAInterface
    : BufferDSAInterface_Bindable<CRTP>
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

    // Wraps `glUnmapNamedBuffer`.
    //
    // Returns `true` if unmapping succeded, `false` otherwise.
    //
    // `glUnmapBuffer` returns `GL_TRUE` unless the data store contents
    // have become corrupt during the time the data store was mapped.
    // This can occur for system-specific reasons that affect
    // the availability of graphics memory, such as screen mode changes.
    // In such situations, `GL_FALSE` is returned and the data store contents
    // are undefined. An application must detect this rare condition
    // and reinitialize the data store.
    bool unmap_current() const noexcept {
        GLboolean unmapping_succeded = gl::glUnmapNamedBuffer(self_id());
        return unmapping_succeded == gl::GL_TRUE;
    }

};








template<typename CRTP, typename T>
struct TypedBufferDSAInterface
    : BufferDSAInterface_Bindable<CRTP>
    , BufferDSAInterface_CommonQueries<CRTP>
{
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
    using mt = mutability_traits<CRTP>;
    using mutability = mt::mutability;
public:


    // Binding Subranges.

    // Wraps `glBindBufferRange`.
    template<BufferTargetIndexed TargetV>
    void bind_range_to_index(
        GLintptr elem_offset, GLsizeiptr elem_count,
        GLuint index) const noexcept
    {
        gl::glBindBufferRange(
            GLenum{ to_underlying(TargetV) },
            index, this->id(),
            elem_offset * sizeof(T), elem_count * sizeof(T)
        );
    }


    // Size Queries.

    // Wraps `glGetNamedBufferParameteri64v` with `pname = GL_BUFFER_SIZE`.
    //
    // Equivalent to `get_size_bytes()` divided by `sizeof(T)`.
    GLsizeiptr get_num_elements() const noexcept {
        return this->get_size_bytes() / sizeof(T);
    }


    // Mutable Storage Allocation.


    // Wraps `glNamedBufferData` with `usage = usage_hint`.
    //
    // Creates mutable storage and initializes it with the contents of `src_buf`.
    void specify_data(
        std::span<const T> src_buf,
        BufferUsageHint usage_hint = BufferUsageHint::StaticDraw) const noexcept
            requires gl_mutable<mutability>
    {
        gl::glNamedBufferData(
            self_id(), src_buf.size_bytes(), src_buf.data(), enum_cast<GLenum>(usage_hint)
        );
    }

    // Wraps `glNamedBufferData`.
    //
    // Creates mutable storage leaving the contents undefined.
    void allocate_data(
        GLsizeiptr num_elements,
        BufferUsageHint usage_hint = BufferUsageHint::StaticDraw) const noexcept
            requires gl_mutable<mutability>
    {
        gl::glNamedBufferData(
            self_id(), num_elements * sizeof(T), nullptr, enum_cast<GLenum>(usage_hint)
        );
    }


    // Immutable Storage Allocation.

    // Wraps `glNamedBufferStorage` with `flags = storage_mode | mapping_mode`.
    //
    // Creates immutable storage and initializes it with the contents of `src_buf`.
    void specify_storage(
        std::span<const T> src_buf,
        BufferStorageMode             storage_mode = BufferStorageMode::DynamicServer,
        BufferStoragePermittedMapping mapping_mode = BufferStoragePermittedMapping::ReadWrite) const noexcept
            requires gl_mutable<mutability>
    {
        gl::BufferStorageMask flags{
            to_underlying(storage_mode) | to_underlying(mapping_mode)
        };
        gl::glNamedBufferStorage(
            self_id(), src_buf.size_bytes(), src_buf.data(), flags
        );
    }

    // Wraps `glNamedBufferStorage` with `data = nullptr` and `flags = storage_mode | mapping_mode`.
    //
    // Creates immutable storage leaving the contents undefined.
    void allocate_storage(
        GLsizeiptr num_elements,
        BufferStorageMode             storage_mode = BufferStorageMode::DynamicServer,
        BufferStoragePermittedMapping mapping_mode = BufferStoragePermittedMapping::ReadWrite) const noexcept
            requires gl_mutable<mutability>
    {
        gl::BufferStorageMask flags{
            to_underlying(storage_mode) | to_underlying(mapping_mode)
        };
        gl::glNamedBufferStorage(
            self_id(), num_elements * sizeof(T), nullptr, flags
        );
    }


    // Set/Get/Copy Buffer (Sub) Data.

    // Wraps `glNamedBufferSubData`.
    //
    // Will copy `src_buf.size()` elements from `src_buf` to this Buffer.
    void sub_data(std::span<const T> src_buf, GLsizeiptr elem_offset = 0) const noexcept
        requires gl_mutable<mutability>
    {
        gl::glNamedBufferSubData(
            self_id(), elem_offset * sizeof(T),
            src_buf.size_bytes(), src_buf.data()
        );
    }

    // Wraps `glGetNamedBufferSubData`.
    //
    // Will copy `dst_buf.size()` elements from this Buffer to `dst_buf`.
    void get_sub_data_into(
        std::span<T> dst_buf, GLsizeiptr elem_offset = 0) const noexcept
    {
        gl::glGetNamedBufferSubData(
            self_id(), elem_offset * sizeof(T),
            dst_buf.size_bytes(), dst_buf.data()
        );
    }

    // Wraps `glCopyNamedBufferSubData`.
    //
    // Will copy `src_elem_count` elements from this Buffer to `dst_buffer`.
    // No alignment or layout is considered. Copies bytes directly, similar to `memcpy`.
    template<typename DstT = T>
    void copy_sub_data_to(
        mt::template type_template<GLMutable, DstT> dst_buffer,
        GLsizeiptr src_elem_count,
        GLintptr src_elem_offset = 0,
        GLintptr dst_elem_offset = 0) const noexcept
    {
        gl::glCopyNamedBufferSubData(
            self_id(),
            dst_buffer.id(),
            src_elem_offset * sizeof(T),
            dst_elem_offset * sizeof(DstT),
            src_elem_count  * sizeof(T)
        );
    }


    // Buffer Data Invalidation.

    // Wraps `glInvalidateBufferData`.
    void invalidate_contents() const noexcept
        requires gl_mutable<mutability>
    {
        gl::glInvalidateBufferData(self_id());
    }

    // Wraps `glInvalidateBufferSubData`.
    void invalidate_subrange(GLintptr elem_offset, GLsizeiptr elem_count) const noexcept
        requires gl_mutable<mutability>
    {
        gl::glInvalidateBufferSubData(
            self_id(), elem_offset * sizeof(T), elem_count * sizeof(T)
        );
    }


    // Buffer Mapping.

    // Wraps `glMapNamedBufferRange` with `access = GL_MAP_READ_BIT | read_access | persistence`.
    [[nodiscard]] std::span<const T> map_range_for_read(
        GLintptr elem_offset, GLsizeiptr elem_count,
        BufferMappingReadAccess  read_access = BufferMappingReadAccess::Synchronized,
        BufferMappingPersistence persistence = BufferMappingPersistence::NotPersistent) const noexcept
    {
        gl::BufferAccessMask access{
            to_underlying(persistence) | to_underlying(read_access)
        };
        return map_buffer_range_impl<T>(
            self_id(), elem_offset, elem_count,
            access, gl::GL_MAP_READ_BIT
        );
    }

    // Wraps `glMapNamedBufferRange` with `offset = 0`, `length = get_size_bytes()` and `access = GL_MAP_READ_BIT | read_access | persistence`.
    //
    // Maps the entire buffer.
    [[nodiscard]] std::span<const T> map_for_read(
        BufferMappingReadAccess  read_access = BufferMappingReadAccess::Synchronized,
        BufferMappingPersistence persistence = BufferMappingPersistence::NotPersistent) const noexcept
    {
        return map_for_read(0, get_num_elements(), persistence, read_access);
    }

    // Wraps `glMapNamedBufferRange` with `access = GL_MAP_WRITE_BIT | write_access | previous_contents | persistence`.
    [[nodiscard]] std::span<T> map_range_for_write(
        GLintptr elem_offset, GLsizeiptr elem_count,
        BufferMappingWriteAccess      write_access      = BufferMappingWriteAccess::Synchronized,
        BufferMappingPreviousContents previous_contents = BufferMappingPreviousContents::DoNotInvalidate,
        BufferMappingPersistence      persistence       = BufferMappingPersistence::NotPersistent) const noexcept
            requires gl_mutable<mutability>
    {
        gl::BufferAccessMask access{
            to_underlying(write_access) | to_underlying(previous_contents) | to_underlying(persistence)
        };
        return map_buffer_range_impl<T>(
            self_id(), elem_offset, elem_count,
            access, gl::GL_MAP_WRITE_BIT
        );
    }

    // Wraps `glMapNamedBufferRange` with `offset = 0`, `length = get_size_bytes()` and `access = GL_MAP_WRITE_BIT | write_access | previous_contents | persistence`.
    //
    // Maps the entire buffer.
    [[nodiscard]] std::span<T> map_for_write(
        BufferMappingWriteAccess      write_access      = BufferMappingWriteAccess::Synchronized,
        BufferMappingPreviousContents previous_contents = BufferMappingPreviousContents::DoNotInvalidate,
        BufferMappingPersistence      persistence       = BufferMappingPersistence::NotPersistent) const noexcept
            requires gl_mutable<mutability>
    {
        return map_for_write(0, get_num_elements(), write_access, previous_contents, persistence);
    }

    // Wraps `glMapNamedBufferRange` with `access = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | readwrite_access | persistence`.
    [[nodiscard]] std::span<T> map_range_for_readwrite(
        GLintptr elem_offset, GLsizeiptr elem_count,
        BufferMappingReadWriteAccess readwrite_access = BufferMappingReadWriteAccess::Synchronized,
        BufferMappingPersistence     persistence      = BufferMappingPersistence::NotPersistent) const noexcept
            requires gl_mutable<mutability>
    {
        gl::BufferAccessMask access{
            to_underlying(readwrite_access) | to_underlying(persistence)
        };
        return map_buffer_range_impl<T>(
            self_id(), elem_offset, elem_count,
            access, (gl::GL_MAP_READ_BIT | gl::GL_MAP_WRITE_BIT)
        );
    }

    // Wraps `glMapNamedBufferRange` with `offset = 0`, `length = get_size_bytes()` and `access = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | readwrite_access | persistence`.
    //
    // Maps the entire buffer.
    [[nodiscard]] std::span<T> map_for_readwrite(
        BufferMappingReadWriteAccess readwrite_access = BufferMappingReadWriteAccess::Synchronized,
        BufferMappingPersistence     persistence      = BufferMappingPersistence::NotPersistent) const noexcept
            requires gl_mutable<mutability>
    {
        return map_for_readwrite(0, get_num_elements(), readwrite_access, persistence);
    }


    // Mapped Buffer Control.

    // Wraps `glUnmapNamedBuffer`.
    GLboolean unmap_current() const noexcept {
        GLboolean unmapping_succeded = gl::glUnmapNamedBuffer(self_id());
        return unmapping_succeded;
    }

    // Wraps `glFlushMappedNamedBufferRange`.
    //
    // The buffer object must previously have been mapped with the
    // `BufferMapping[Read]WriteAccess` equal to one of the `*MustFlushExplicitly` options.
    void flush_mapped_range(GLintptr elem_offset, GLsizeiptr elem_count) const noexcept
        requires gl_mutable<mutability>
    {
        gl::glFlushMappedNamedBufferRange(
            self_id(), elem_offset * sizeof(T), elem_count * sizeof(T)
        );
    }

};


} // namespace detail




namespace detail {
using josh::detail::RawGLHandle;
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


// Lil namespace break, donnt worry aboutit
} namespace josh {
template<trivially_copyable T, mutability_tag MutT>
struct mutability_traits<dsa::RawBuffer<T, MutT>> {
    using mutability                 = MutT;
    using opposite_mutability        = MutT::opposite_mutability;
    template<typename ...ArgTs>
    using type_template              = dsa::RawBuffer<ArgTs...>;
    using const_type                 = dsa::RawBuffer<T, GLConst>;
    using mutable_type               = dsa::RawBuffer<T, GLMutable>;
    using opposite_type              = dsa::RawBuffer<T, opposite_mutability>;
    static constexpr bool is_mutable = gl_mutable<mutability>;
    static constexpr bool is_const   = gl_const<mutability>;
};
} namespace josh::dsa {




template<mutability_tag MutT = GLMutable>
class RawUntypedBuffer
    : public detail::RawGLHandle<MutT>
    , public detail::UntypedBufferDSAInterface<RawUntypedBuffer<MutT>>
{
public:
    static constexpr GLKind kind_type = GLKind::Buffer;
    JOSH3D_MAGIC_CONSTRUCTORS_2(RawUntypedBuffer, mutability_traits<RawUntypedBuffer>, detail::RawGLHandle<MutT>)


    template<convertible_mutability_to<MutT> MutU, typename T>
    RawUntypedBuffer(RawBuffer<MutU, T> typed_buffer)
        : RawUntypedBuffer{ typed_buffer.id() }
    {}

    // Explicit cast to a typed buffer, similar to a `static_cast` from a `void*`.
    template<typename T>
    auto as_typed() const noexcept
        -> RawBuffer<MutT, T>
    {
        return RawBuffer<MutT, T>{ this->id() };
    }

};




} // namespace josh::dsa
