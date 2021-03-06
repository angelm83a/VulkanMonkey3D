#pragma once
#include "Math.h"
#include "Pointer.h"
#include "../Camera/Camera.h"

namespace vm
{
	class Mesh;
	class Node;

	struct Skin
	{
		std::string name;
		Pointer<Node> skeletonRoot;
		std::vector<mat4> inverseBindMatrices;
		std::vector<Pointer<Node>> joints;
	};

	// It is invalid to have both 'matrix' and any of 'translation'/'rotation'/'scale'
	//   spec: "A node can have either a 'matrix' or any combination of 'translation'/'rotation'/'scale' (TRS) properties"
	enum TransformationType
	{
		TRANSFORMATION_IDENTITY = 0,
		TRANSFORMATION_MATRIX,
		TRANSFORMATION_TRS
	};

	class Node
	{
	public:
		Pointer<Node> parent;
		uint32_t index;
		std::vector<Pointer<Node>> children;
		mat4 matrix;
		std::string name;
		Pointer<Mesh> mesh;
		Pointer<Skin> skin;
		int32_t skinIndex = -1;
		vec3 translation;
		vec3 scale;
		quat rotation;
		TransformationType transformationType;

		mat4 localMatrix() const;
		mat4 getMatrix() const;
		void update(Camera& camera);
	};
}
