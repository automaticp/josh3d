#pragma once
#include <glm/glm.hpp>

namespace light {

	struct Colored {
		glm::vec3 color;
	};

	struct Attenuation {
		float constant;
		float linear;
		float quadratic;
	};


	struct Directional : Colored {
		glm::vec3 direction;
	};

	struct Point : Colored {
		glm::vec3 position;
		Attenuation attenuation;
	};

	struct Spotlight : Colored {
		glm::vec3 position;
		glm::vec3 direction;
		Attenuation attenuation;
		float innerCutoffRad;
		float outerCutoffRad;
	};

}
