#include "Transform.hpp"
#include <doctest/doctest.h>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/ext/matrix_relational.hpp>


using namespace josh;


static constexpr float pi = glm::pi<float>();


TEST_CASE("Transform and MTransform result model matrices are equivalent") {

    constexpr glm::vec3 trans{ -0.1f, 1.6f,  0.3f };
    const     glm::quat rot  { glm::vec3{ pi / 16.f, pi * (13.f / 7.f), 0.3f * pi } }; // From Pitch/Yaw/Roll.
    constexpr glm::vec3 scale{ 0.3f,  1.7f, 0.95f };

    Transform  tf  = Transform().translate(trans).rotate(rot).scale(scale);
    MTransform mtf = MTransform().translate(trans).rotate(rot).scale(scale); // Order matters for MTransform.

    glm::mat4 tf_mat  = tf.mtransform().model();
    glm::mat4 mtf_mat = mtf.model();

    for (size_t i{ 0 }; i < 4; ++i) {
        for (size_t j{ 0 }; j < 4; ++j) {
            INFO("(Row, Column): ("<< j << ", " << i << ")");
            CHECK(tf_mat[i][j] == doctest::Approx(mtf_mat[i][j]));
        }
    }

}

