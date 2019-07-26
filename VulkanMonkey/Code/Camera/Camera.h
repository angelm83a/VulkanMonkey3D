#pragma once
#include "../Math/Math.h"
#include "../../include/Vulkan.h"

namespace vm {
	struct Camera
	{
		enum struct RelativeDirection {
			FORWARD,
			BACKWARD,
			LEFT,
			RIGHT
		};
		struct TargetArea {
			vk::Viewport viewport;
			vk::Rect2D scissor;
			void update(const vec2& position, const vec2& size, float minDepth = 0.f, float maxDepth = 1.f);
		};

		struct Plane
		{
			vec3 normal{};
			float d{};
		};

		mat4 view;
		mat4 previousView;
		mat4 projection;
		mat4 previousProjection;
		mat4 invView;
		mat4 invProjection;
		mat4 invViewProjection;
		quat orientation;
		vec3 position, euler, worldOrientation;
		vec3 front, right, up;
		float nearPlane, farPlane, FOV, speed, rotationSpeed;
		vec2 projOffset, projOffsetPrevious;
		TargetArea renderArea;
		Plane frustum[6];

		Camera();
		void update();
		void updatePerspective();
		void updateView();
		[[nodiscard]] vec3 worldFront() const;
		[[nodiscard]] vec3 worldRight() const;
		[[nodiscard]] vec3 worldUp() const;
		void move(RelativeDirection direction, float velocity);
		void rotate(float xoffset, float yoffset);
		void ExtractFrustum();
		[[nodiscard]] bool SphereInFrustum(const vec4& boundingSphere) const;
	};
}
