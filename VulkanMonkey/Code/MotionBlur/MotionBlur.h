#pragma once

#include "../VulkanContext/VulkanContext.h"
#include "../Buffer/Buffer.h"
#include "../Pipeline/Pipeline.h"
#include "../Image/Image.h"
#include "../Surface/Surface.h"
#include "../Swapchain/Swapchain.h"
#include "../GUI/GUI.h"
#include "../Timer/Timer.h"
#include "../Camera/Camera.h"
#include <vector>
#include <string>
#include <map>

namespace vm {
	struct MotionBlur
	{
		VulkanContext* vulkan = &VulkanContext::get();

		Buffer UBmotionBlur;
		std::vector<vk::Framebuffer> frameBuffers{};
		Pipeline pipeline;
		vk::RenderPass renderPass;
		vk::DescriptorSet DSMotionBlur;
		vk::DescriptorSetLayout DSLayoutMotionBlur;
		
		void update(Camera& camera);
		void createRenderPass();
		void createFrameBuffers();
		void createPipeline();
		void createMotionBlurUniforms(std::map<std::string, Image>& renderTargets);
		void updateDescriptorSets(std::map<std::string, Image>& renderTargets);
		void draw(uint32_t imageIndex, const vec2 UVOffset[2]);
		void destroy();
	};
}