#pragma once
#include <concepts>


namespace josh {


namespace detail {


/*
A generic trait that allows you to invoke any
callable during bound state. Shold help minimize
creation of bound dummies as lvalues in certain cases.

Example 1:

dst_framebuffer.bind_draw()
    .and_then([&](BoundDrawFramebuffer& dfbo) {
        auto [w, h] = window_.getSize();
        src_framebuffer.bind_read()
            .blit_to(dfbo, 0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST)
            .unbind();
    })
    .unbind();


Example 2:

vao_.bind()
    .and_then([&](BoundVAO& self) {

        vbo_.bind()
            .attach_data(data.vertices().size(), data.vertices().data(), GL_STATIC_DRAW)
            .associate_with<VertexT>(self);

        ebo_.bind(self)
            .attach_data(data.elements().size(), data.elements().data(), GL_STATIC_DRAW);

    })
    .unbind();

*/
template<typename CRTP>
class AndThen {
public:

    template<std::invocable CallableT>
    CRTP& and_then(CallableT&& f) noexcept(noexcept(f())) {
        f();
        return static_cast<CRTP&>(*this);
    }

    template<std::invocable CallableT>
    const CRTP& and_then(CallableT&& f) const noexcept(noexcept(f())) {
        f();
        return static_cast<const CRTP&>(*this);
    }


    template<std::invocable<CRTP&> CallableT>
    CRTP& and_then(CallableT&& f)
        noexcept(noexcept(f(static_cast<CRTP&>(*this))))
    {
        f(static_cast<CRTP&>(*this));
        return static_cast<CRTP&>(*this);
    }

    template<std::invocable<const CRTP&> CallableT>
    const CRTP& and_then(CallableT&& f) const
        noexcept(noexcept(f(static_cast<const CRTP&>(*this))))
    {
        f(static_cast<const CRTP&>(*this));
        return static_cast<const CRTP&>(*this);
    }

};


} // namespace detail


} // namespace josh

