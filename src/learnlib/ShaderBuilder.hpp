#pragma once
#include <utility>
#include <string>

#include "GLObjects.hpp"
#include "ShaderSource.hpp"


namespace learn {



class ShaderBuilder {
private:
    ShaderProgram sp_{};

public:
    ShaderBuilder& load_frag(const std::string& path) {
        FragmentShader fs;
        fs.set_source(FileReader{}(path)).compile();
        sp_.attach_shader(fs);
        return *this;
    }

    ShaderBuilder& load_vert(const std::string& path) {
        VertexShader vs;
        vs.set_source(FileReader{}(path)).compile();
        sp_.attach_shader(vs);
        return *this;
    }

    [[nodiscard]]
    ShaderProgram get() {
        sp_.link();
        return std::move(sp_);
    }

};


} // namespace learn
