#pragma once

#include "../../include/Vulkan.h"
#include "../../include/SDL.h"
#include <fstream>
#include <vector>
#include <string>

constexpr auto VULKAN_CONTEXT_INSTANCES = 2;

#define WIDTH VulkanContext::get().surface->actualExtent.width
#define HEIGHT VulkanContext::get().surface->actualExtent.height
#define WIDTH_f static_cast<float>(WIDTH)
#define HEIGHT_f static_cast<float>(HEIGHT)

namespace vm {

	struct Image;
	struct Surface;
	struct Swapchain;

	struct VulkanContext {
		static VulkanContext& get(uint32_t index = 0)
		{
			static VulkanContext VkCTX[VULKAN_CONTEXT_INSTANCES];

			assert(index < VULKAN_CONTEXT_INSTANCES);
			return VkCTX[index];
		}

		SDL_Window* window = nullptr;
		vk::Instance instance;
		Surface* surface = nullptr;
		int graphicsFamilyId{}, presentFamilyId{}, computeFamilyId{};
		vk::PhysicalDevice gpu;
		vk::PhysicalDeviceProperties gpuProperties;
		vk::PhysicalDeviceFeatures gpuFeatures;
		vk::Device device;
		vk::Queue graphicsQueue, presentQueue, computeQueue;
		vk::CommandPool commandPool;
		vk::CommandPool commandPool2;
		vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e4;
		Swapchain* swapchain = nullptr;
		Image* depth = nullptr;
		vk::CommandBuffer dynamicCmdBuffer;
		std::vector<vk::CommandBuffer> shadowCmdBuffer{};
		vk::DescriptorPool descriptorPool;
		std::vector<vk::Fence> fences{};
		std::vector<vk::Semaphore> semaphores{};

	private:
		VulkanContext() {};
		VulkanContext(VulkanContext const&) {};
		VulkanContext& operator=(VulkanContext const&) {};
		~VulkanContext() {};
	};

	static std::vector<char> readFile(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}
		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}
}