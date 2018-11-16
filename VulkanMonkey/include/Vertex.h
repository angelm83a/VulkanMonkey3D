#pragma once
#include "Vulkan.h"
#include "Math.h"

struct Vertex
{
	Vertex();
	Vertex(cfloat x, cfloat y, cfloat z,
		cfloat u, cfloat v,
		cfloat nX, cfloat nY, cfloat nZ,
		cfloat tX, cfloat tY, cfloat tZ,
		cfloat bX, cfloat bY, cfloat bZ,
		cfloat r, cfloat g, cfloat b, cfloat a);
	Vertex(vm::vec3 pos, vm::vec2 uv, vm::vec3 norm, vm::vec3 tang, vm::vec3 bitang, vm::vec4 color);

	bool operator==(const Vertex& other) const;
	static std::vector<vk::VertexInputBindingDescription> getBindingDescriptionGeneral();
	static std::vector<vk::VertexInputBindingDescription> getBindingDescriptionGUI();
	static std::vector<vk::VertexInputBindingDescription> getBindingDescriptionSkyBox();
	static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptionGeneral();
	static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptionGUI();
	static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptionSkyBox();

	float 
		x, y, z,			// Vertex Position
		u, v,				// Texture u, _v
		nX, nY, nZ,			// Normals x, y, z
		tX, tY, tZ,			// Tangents
		bX, bY, bZ,			// Bitangents
		r, g, b, a;			// color rgba

};