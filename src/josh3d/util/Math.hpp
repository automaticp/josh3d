#include <glm/glm.hpp>          // IWYU pragma: export
#include <glm/gtc/type_ptr.hpp> // IWYU pragma: export


/*
Re-exports of glm types and math utilities.

HMM: We could DERIVE_TYPE instead so that the error messages
would not have the awful `mat<4, 4, float, glm::packed_highp>`
and just name the types as `mat4`. Why, glm, why?
*/
namespace josh {

using glm::vec1;
using glm::vec2,   glm::vec3,   glm::vec4;
using glm::uvec2,  glm::uvec3,  glm::uvec4;
using glm::ivec2,  glm::ivec3,  glm::ivec4;
using glm::mat2,   glm::mat2x3, glm::mat2x4;
using glm::mat3x2, glm::mat3,   glm::mat3x4;
using glm::mat4x2, glm::mat4x3, glm::mat4;
using glm::quat;

} // namespace josh
