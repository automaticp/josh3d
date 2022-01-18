#pragma once
#include <glm/glm.hpp>

namespace light {

	struct Attenuation {
		float constant;
		float linear;
		float quadratic;
	};


	struct Directional {
		glm::vec3 color;
		glm::vec3 direction;
	};

	struct Point {
		glm::vec3 color;
		glm::vec3 position;
		Attenuation attenuation;
	};

	struct Spotlight {
		glm::vec3 color;
		glm::vec3 position;
		glm::vec3 direction;
		Attenuation attenuation;
		float innerCutoffRad;
		float outerCutoffRad;
	};

}
