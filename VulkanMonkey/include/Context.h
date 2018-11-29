#pragma once

#include "Vulkan.h"
#include "VulkanContext.h"
#include "SDL.h"
#include "Math.h"
#include "Vertex.h"
#include "Image.h"
#include "Surface.h"
#include "Swapchain.h"
#include "Buffer.h"
#include "Pipeline.h"
#include "Object.h"
#include "GUI.h"
#include "Skybox.h"
#include "Terrain.h"
#include "Shadows.h"
#include "Light.h"
#include "Mesh.h"
#include "Model.h"
#include "Camera.h"
#include "SSAO.h"
#include "SSR.h"
#include "Deferred.h"
#include "Compute.h"
#include "Forward.h"

#include <tuple>
#include <map>

namespace vm {
	struct Context
	{
	public:
		// VULKAN CONTEXT
		VulkanContext &vulkan = VulkanContext::getVulkanContext();

		// RENDER TARGETS
		std::map<std::string, Image> renderTargets;

		// MODELS
		static std::vector<Model> models;

		// SHADOWS
		Shadows shadows;

		// COMPUTE
		Compute compute;

		// FORWARD
		Forward forward;

		// DEFERRED
		Deferred deferred;

		// SSR
		SSR ssr;

		// SSAO
		SSAO ssao;

		// SKYBOX
		SkyBox skyBox;

		// TERRAIN
		Terrain terrain;

		// GUI
		GUI gui;

		// LIGHTS
		LightUniforms lightUniforms;

		// MAIN CAMERA
		Camera camera_main;

		// CAMERAS
		std::vector<Camera> camera{};

		void initVulkanContext();
		void initRendering();
		void loadResources();
		void createUniforms();
		void destroyVkContext();
		void resizeViewport(uint32_t width, uint32_t height);

		PipelineInfo getPipelineSpecificationsModel();
		PipelineInfo getPipelineSpecificationsShadows();
		PipelineInfo getPipelineSpecificationsSkyBox();
		PipelineInfo getPipelineSpecificationsTerrain();
		PipelineInfo getPipelineSpecificationsGUI();
		PipelineInfo getPipelineSpecificationsDeferred();

		vk::DescriptorSetLayout getDescriptorSetLayoutLights();

	private:
		vk::Instance createInstance();
		Surface createSurface();
		int getGraphicsFamilyId();
		int getPresentFamilyId();
		int getComputeFamilyId();
		vk::PhysicalDevice findGpu();
		vk::PhysicalDeviceProperties getGPUProperties();
		vk::PhysicalDeviceFeatures getGPUFeatures();
		vk::SurfaceCapabilitiesKHR getSurfaceCapabilities();
		vk::SurfaceFormatKHR getSurfaceFormat();
		vk::PresentModeKHR getPresentationMode();
		vk::Device createDevice();
		vk::Queue getGraphicsQueue();
		vk::Queue getPresentQueue();
		vk::Queue getComputeQueue();
		Swapchain createSwapchain();
		vk::CommandPool createCommandPool();
		vk::CommandPool createComputeCommadPool();
		std::map<std::string, Image> createRenderTargets(std::vector<std::tuple<std::string, vk::Format>> RTtuples);
		vk::RenderPass createRenderPass();
		vk::RenderPass createDeferredRenderPass();
		vk::RenderPass createCompositionRenderPass();
		vk::RenderPass createSSAORenderPass();
		vk::RenderPass createSSAOBlurRenderPass();
		vk::RenderPass createReflectionRenderPass();
		vk::RenderPass createGUIRenderPass();
		Image createDepthResources();
		std::vector<vk::Framebuffer> createFrameBuffers();
		std::vector<vk::Framebuffer> createDeferredFrameBuffers();
		std::vector<vk::Framebuffer> createCompositionFrameBuffers();
		std::vector<vk::Framebuffer> createReflectionFrameBuffers();
		std::vector<vk::Framebuffer> createSSAOFrameBuffers();
		std::vector<vk::Framebuffer> createSSAOBlurFrameBuffers();
		std::vector<vk::Framebuffer> createGUIFrameBuffers();
		std::vector<vk::CommandBuffer> createCmdBuffers(const uint32_t bufferCount);
		vk::CommandBuffer createCmdBuffer();
		vk::CommandBuffer createComputeCmdBuffer();
		Pipeline createPipeline(const PipelineInfo& specificInfo);
		Pipeline createCompositionPipeline();
		Pipeline createReflectionPipeline();
		Pipeline createSSAOPipeline();
		Pipeline createSSAOBlurPipeline();
		Pipeline createComputePipeline();
		vk::DescriptorPool createDescriptorPool(const uint32_t maxDescriptorSets);
		std::vector<vk::Fence> createFences(const uint32_t fenceCount);
		std::vector<vk::Semaphore> createSemaphores(const uint32_t semaphoresCount);
	};
}