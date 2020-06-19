#include "Mesh.h"
#include "../../include/tinygltf/stb_image.h"
#include "../VulkanContext/VulkanContext.h"
#include <vulkan/vulkan.hpp>

namespace vm
{
	Ref_t<vk::DescriptorSetLayout> Mesh::descriptorSetLayout = Ref_t<vk::DescriptorSetLayout>();
	Ref_t<vk::DescriptorSetLayout> Primitive::descriptorSetLayout = Ref_t<vk::DescriptorSetLayout>();
	std::map<std::string, Image> Mesh::uniqueTextures{};

	Primitive::Primitive() : pbrMaterial({})
	{
		if (!Primitive::descriptorSetLayout)
			Primitive::descriptorSetLayout = vk::DescriptorSetLayout();
		descriptorSet = vk::DescriptorSet();
	}

	Primitive::~Primitive()
	{
	}

	vk::DescriptorSetLayout* Primitive::getDescriptorSetLayout()
	{
		if (!descriptorSetLayout)
			descriptorSetLayout = vk::DescriptorSetLayout();

		if (!descriptorSetLayout.Value()) {
			auto const layoutBinding = [](uint32_t binding, vk::DescriptorType descriptorType, const vk::ShaderStageFlags& stageFlag) {
				return vk::DescriptorSetLayoutBinding{ binding, descriptorType, 1, stageFlag, nullptr };
			};
			std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings{
				layoutBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment),
				layoutBinding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment),
				layoutBinding(2, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment),
				layoutBinding(3, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment),
				layoutBinding(4, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment),
				layoutBinding(5, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex),
			};
			vk::DescriptorSetLayoutCreateInfo descriptorLayout;
			descriptorLayout.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
			descriptorLayout.pBindings = setLayoutBindings.data();
			descriptorSetLayout = VulkanContext::get()->device->createDescriptorSetLayout(descriptorLayout);
		}
		return &*descriptorSetLayout;
	}

	Mesh::Mesh()
	{
		if (!Mesh::descriptorSetLayout)
			Mesh::descriptorSetLayout = vk::DescriptorSetLayout();
		descriptorSet = vk::DescriptorSet();
	}

	Mesh::~Mesh()
	{
	}

	vk::DescriptorSetLayout* Mesh::getDescriptorSetLayout()
	{
		if (!Mesh::descriptorSetLayout)
			Mesh::descriptorSetLayout = vk::DescriptorSetLayout();

		if (!descriptorSetLayout.Value()) {
			auto const layoutBinding = [](uint32_t binding, vk::DescriptorType descriptorType) {
				return vk::DescriptorSetLayoutBinding{ binding, descriptorType, 1, vk::ShaderStageFlagBits::eVertex, nullptr };
			};
			std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings{
				layoutBinding(0, vk::DescriptorType::eUniformBuffer),
			};
			vk::DescriptorSetLayoutCreateInfo descriptorLayout;
			descriptorLayout.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
			descriptorLayout.pBindings = setLayoutBindings.data();
			descriptorSetLayout = VulkanContext::get()->device->createDescriptorSetLayout(descriptorLayout);
		}
		return &*descriptorSetLayout;
	}

	void Mesh::createUniformBuffers()
	{
		uniformBuffer.createBuffer(sizeof(ubo), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible);
		uniformBuffer.map();
		uniformBuffer.zero();
		uniformBuffer.flush();
		uniformBuffer.unmap();

		for (auto& primitive : primitives) {

			mat4 factors;
			factors[0] = primitive.pbrMaterial.baseColorFactor != vec4(0.f) ? primitive.pbrMaterial.baseColorFactor : vec4(1.f);
			factors[1] = vec4(primitive.pbrMaterial.emissiveFactor, 1.f);
			factors[2] = vec4(primitive.pbrMaterial.metallicFactor, primitive.pbrMaterial.roughnessFactor, primitive.pbrMaterial.alphaCutoff, 0.f);
			factors[3][0] = static_cast<float>(primitive.hasBones);

			const size_t size = sizeof(mat4);
			primitive.uniformBuffer.createBuffer(size, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible);
			primitive.uniformBuffer.map();
			primitive.uniformBuffer.copyData(&factors);
			primitive.uniformBuffer.flush();
			primitive.uniformBuffer.unmap();
		}
	}

	void Primitive::loadTexture(
		MaterialType type,
		const std::string& folderPath,
		const Microsoft::glTF::Image* image,
		const Microsoft::glTF::Document* document,
		const Microsoft::glTF::GLTFResourceReader* resourceReader)
	{
		std::string path = folderPath;
		if (image)
			path = folderPath + image->uri;

		// get the right texture
		Image* tex;
		switch (type)
		{
		case MaterialType::BaseColor:
			tex = &pbrMaterial.baseColorTexture;
			if (!image || image->uri.empty())
				path = "objects/black.png";
			break;
		case MaterialType::MetallicRoughness:
			tex = &pbrMaterial.metallicRoughnessTexture;
			if (!image || image->uri.empty())
				path = "objects/black.png";
			break;
		case MaterialType::Normal:
			tex = &pbrMaterial.normalTexture;
			if (!image || image->uri.empty())
				path = "objects/normal.png";
			break;
		case MaterialType::Occlusion:
			tex = &pbrMaterial.occlusionTexture;
			if (!image || image->uri.empty())
				path = "objects/white.png";
			break;
		case MaterialType::Emissive:
			tex = &pbrMaterial.emissiveTexture;
			if (!image || image->uri.empty())
				path = "objects/black.png";
			break;
		default:
			throw std::runtime_error("Load texture invalid type");
		}

		// Check if it is already loaded
		if (Mesh::uniqueTextures.find(path) != Mesh::uniqueTextures.end()) {
			*tex = Mesh::uniqueTextures[path];
		}
		else {
			int texWidth, texHeight, texChannels;
			unsigned char* pixels;
			//stbi_set_flip_vertically_on_load(true);
			if (!image || image->bufferViewId.empty())
				pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
			else
			{
				const auto data = resourceReader->ReadBinaryData(*document, *image);
				pixels = stbi_load_from_memory(data.data(), static_cast<int>(data.size()), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
			}

			if (!pixels)
				throw std::runtime_error("No pixel data loaded");

			const vk::DeviceSize imageSize = texWidth * texHeight * STBI_rgb_alpha;

			VulkanContext::get()->graphicsQueue->waitIdle();
			VulkanContext::get()->waitAndLockSubmits();

			Buffer staging;
			staging.createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible);
			staging.map();
			staging.copyData(pixels);
			staging.flush();
			staging.unmap();

			stbi_image_free(pixels);

			tex->format = vk::Format::eR8G8B8A8Unorm;
			tex->mipLevels = static_cast<uint32_t>(std::floor(std::log2(texWidth > texHeight ? texWidth : texHeight))) + 1;
			tex->createImage(texWidth, texHeight, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal);
			tex->transitionImageLayout(vk::ImageLayout::ePreinitialized, vk::ImageLayout::eTransferDstOptimal);
			tex->copyBufferToImage(staging.buffer.Value());
			tex->generateMipMaps();
			tex->createImageView(vk::ImageAspectFlagBits::eColor);
			tex->maxLod = static_cast<float>(tex->mipLevels.Value());
			tex->createSampler();

			staging.destroy();

			VulkanContext::get()->unlockSubmits();

			Mesh::uniqueTextures[path] = *tex;
		}
	}

	//void Mesh::calculateBoundingSphere()
	//{
	//	vec3 _max(-FLT_MAX);
	//	vec3 _min(FLT_MAX);
	//	for (auto& vertex : vertices) {
	//		_max = maximum(_max, vertex.position);
	//		_min = minimum(_min, vertex.position);
	//	}
	//	vec3 center = (_max + _min) * .5f;
	//	float sphereRadius = length(_max - center);
	//	boundingSphere = vec4(center, sphereRadius);
	//}

	void Mesh::destroy()
	{
		uniformBuffer.destroy();
		if (descriptorSetLayout.Value()) {
			VulkanContext::get()->device->destroyDescriptorSetLayout(descriptorSetLayout.Value());
			*descriptorSetLayout = nullptr;
		}

		for (auto& primitive : primitives) {
			primitive.uniformBuffer.destroy();
		}
		vertices.clear();
		vertices.shrink_to_fit();
		indices.clear();
		indices.shrink_to_fit();
		if (Primitive::descriptorSetLayout.Value()) {
			VulkanContext::get()->device->destroyDescriptorSetLayout(Primitive::descriptorSetLayout.Value());
			*Primitive::descriptorSetLayout = nullptr;
		}
	}
}