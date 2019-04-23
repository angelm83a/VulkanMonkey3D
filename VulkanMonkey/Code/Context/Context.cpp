#include "Context.h"
#include "../Queue/Queue.h"
#include <filesystem>
#include "../Window/Window.h"

using namespace vm;

//std::vector<Model> Context::models = {};

void Context::initVulkanContext()
{
	vulkan.instance = createInstance();
	vulkan.surface = new Surface(createSurface());
	vulkan.graphicsFamilyId = getGraphicsFamilyId();
	vulkan.presentFamilyId = getPresentFamilyId();
	vulkan.computeFamilyId = getComputeFamilyId();
	vulkan.gpu = findGpu();
	vulkan.gpuProperties = getGPUProperties();
	vulkan.gpuFeatures = getGPUFeatures();
	vulkan.surface->capabilities = getSurfaceCapabilities();
	vulkan.surface->formatKHR = getSurfaceFormat();
	vulkan.surface->presentModeKHR = getPresentationMode();
	vulkan.device = createDevice();
	vulkan.graphicsQueue = getGraphicsQueue();
	vulkan.presentQueue = getPresentQueue();
	vulkan.computeQueue = getComputeQueue();
	vulkan.semaphores = createSemaphores(3);
	vulkan.fences = createFences(2);
	vulkan.swapchain = new Swapchain(createSwapchain());
	vulkan.commandPool = createCommandPool();
	vulkan.commandPool2 = createCommandPool();
	vulkan.descriptorPool = createDescriptorPool(15000); // max number of all descriptor sets to allocate
	vulkan.dynamicCmdBuffer = createCmdBuffers().at(0);
	vulkan.shadowCmdBuffer = createCmdBuffers(3);
	vulkan.depth = new Image(createDepthResources());
}

void Context::initRendering()
{
	addRenderTarget("depth", vk::Format::eR32Sfloat);
	addRenderTarget("normal", vk::Format::eR32G32B32A32Sfloat);
	addRenderTarget("albedo", vk::Format::eR8G8B8A8Unorm);
	addRenderTarget("srm", vk::Format::eR8G8B8A8Unorm); // Specular Roughness Metallic
	addRenderTarget("ssao", vk::Format::eR16Unorm);
	addRenderTarget("ssaoBlur", vk::Format::eR8Unorm);
	addRenderTarget("ssr", vk::Format::eR8G8B8A8Unorm);
	addRenderTarget("composition", vk::Format::eR8G8B8A8Unorm);
	addRenderTarget("composition2", vk::Format::eR8G8B8A8Unorm);
	addRenderTarget("velocity", vk::Format::eR16G16B16A16Sfloat);
	addRenderTarget("brightFilter", vk::Format::eR8G8B8A8Unorm);
	addRenderTarget("gaussianBlurHorizontal", vk::Format::eR8G8B8A8Unorm);
	addRenderTarget("gaussianBlurVertical", vk::Format::eR8G8B8A8Unorm);
	addRenderTarget("emissive", vk::Format::eR8G8B8A8Unorm);

	// render passes
	deferred.createRenderPasses(renderTargets);
	ssao.createRenderPasses(renderTargets);
	ssr.createRenderPass(renderTargets);
	fxaa.createRenderPass(renderTargets);
	motionBlur.createRenderPass();
	gui.createRenderPass();
	shadows.createRenderPass();
	bloom.createRenderPasses(renderTargets);
	skyBoxDay.createRenderPass();
	skyBoxNight.createRenderPass();

	// frame buffers
	deferred.createFrameBuffers(renderTargets);
	ssao.createFrameBuffers(renderTargets);
	ssr.createFrameBuffers(renderTargets);
	fxaa.createFrameBuffers(renderTargets);
	motionBlur.createFrameBuffers();
	gui.createFrameBuffers();
	shadows.createFrameBuffers();
	bloom.createFrameBuffers(renderTargets);
	skyBoxDay.createFrameBuffers();
	skyBoxNight.createFrameBuffers();

	// pipelines
	gui.createPipeline();
	deferred.createPipelines(renderTargets);
	ssao.createPipelines(renderTargets);
	ssr.createPipeline(renderTargets);
	fxaa.createPipeline(renderTargets);
	bloom.createPipelines(renderTargets);
	motionBlur.createPipeline();
	shadows.createPipeline(Mesh::getDescriptorSetLayout(), Model::getDescriptorSetLayout());
	skyBoxDay.createPipeline();
	skyBoxNight.createPipeline();

	computePool.Init(5);

	metrics.resize(20);
	for (auto& metric : metrics)
		metric.initQueryPool();
}

void Context::resizeViewport(uint32_t width, uint32_t height)
{
	vulkan.device.waitIdle();

	//- Free resources ----------------------
	// render targets
	for (auto& RT : renderTargets)
		RT.second.destroy();
	renderTargets.clear();

	// GUI
	if (gui.renderPass) {
		vulkan.device.destroyRenderPass(gui.renderPass);
	}
	for (auto &frameBuffer : gui.frameBuffers) {
		if (frameBuffer) {
			vulkan.device.destroyFramebuffer(frameBuffer);
		}
	}
	gui.pipeline.destroy();

	// deferred
	if (deferred.renderPass) {
		vulkan.device.destroyRenderPass(deferred.renderPass);
	}
	if (deferred.compositionRenderPass) {
		vulkan.device.destroyRenderPass(deferred.compositionRenderPass);
	}
	for (auto &frameBuffer : deferred.frameBuffers) {
		if (frameBuffer) {
			vulkan.device.destroyFramebuffer(frameBuffer);
		}
	}
	for (auto &frameBuffer : deferred.compositionFrameBuffers) {
		if (frameBuffer) {
			vulkan.device.destroyFramebuffer(frameBuffer);
		}
	}
	deferred.pipeline.destroy();
	deferred.pipelineComposition.destroy();

	// SSR
	for (auto &frameBuffer : ssr.frameBuffers) {
		if (frameBuffer) {
			vulkan.device.destroyFramebuffer(frameBuffer);
		}
	}
	if (ssr.renderPass) {
		vulkan.device.destroyRenderPass(ssr.renderPass);
	}
	ssr.pipeline.destroy();

	// FXAA
	for (auto &frameBuffer : fxaa.frameBuffers) {
		if (frameBuffer) {
			vulkan.device.destroyFramebuffer(frameBuffer);
		}
	}
	if (fxaa.renderPass) {
		vulkan.device.destroyRenderPass(fxaa.renderPass);
	}
	fxaa.pipeline.destroy();

	// Bloom
	for (auto &frameBuffer : bloom.frameBuffers) {
		if (frameBuffer) {
			vulkan.device.destroyFramebuffer(frameBuffer);
		}
	}
	if (bloom.renderPassBrightFilter) {
		vulkan.device.destroyRenderPass(bloom.renderPassBrightFilter);
	}
	if (bloom.renderPassGaussianBlur) {
		vulkan.device.destroyRenderPass(bloom.renderPassGaussianBlur);
	}
	if (bloom.renderPassCombine) {
		vulkan.device.destroyRenderPass(bloom.renderPassCombine);
	}
	bloom.pipelineBrightFilter.destroy();
	bloom.pipelineGaussianBlurHorizontal.destroy();
	bloom.pipelineGaussianBlurVertical.destroy();
	bloom.pipelineCombine.destroy();

	// Motion blur
	for (auto &frameBuffer : motionBlur.frameBuffers) {
		if (frameBuffer) {
			vulkan.device.destroyFramebuffer(frameBuffer);
		}
	}
	if (motionBlur.renderPass) {
		vulkan.device.destroyRenderPass(motionBlur.renderPass);
	}
	motionBlur.pipeline.destroy();

	// SSAO
	if (ssao.renderPass) {
		vulkan.device.destroyRenderPass(ssao.renderPass);
	}
	if (ssao.blurRenderPass) {
		vulkan.device.destroyRenderPass(ssao.blurRenderPass);
	}
	for (auto &frameBuffer : ssao.frameBuffers) {
		if (frameBuffer) {
			vulkan.device.destroyFramebuffer(frameBuffer);
		}
	}
	for (auto &frameBuffer : ssao.blurFrameBuffers) {
		if (frameBuffer) {
			vulkan.device.destroyFramebuffer(frameBuffer);
		}
	}
	ssao.pipeline.destroy();
	ssao.pipelineBlur.destroy();

	// skyboxes
	if (skyBoxDay.renderPass) {
		vulkan.device.destroyRenderPass(skyBoxDay.renderPass);
	}
	for (auto &frameBuffer : skyBoxDay.frameBuffers) {
		if (frameBuffer) {
			vulkan.device.destroyFramebuffer(frameBuffer);
		}
	}
	skyBoxDay.pipeline.destroy();

	if (skyBoxNight.renderPass) {
		vulkan.device.destroyRenderPass(skyBoxNight.renderPass);
	}
	for (auto &frameBuffer : skyBoxNight.frameBuffers) {
		if (frameBuffer) {
			vulkan.device.destroyFramebuffer(frameBuffer);
		}
	}
	skyBoxNight.pipeline.destroy();

	vulkan.depth->destroy();
	vulkan.swapchain->destroy();
	//- Free resources end ------------------

	//- Recreate resources ------------------
	vulkan.surface->actualExtent = { width, height };
	*vulkan.swapchain = createSwapchain();
	*vulkan.depth = createDepthResources();

	addRenderTarget("depth", vk::Format::eR32Sfloat);
	addRenderTarget("normal", vk::Format::eR32G32B32A32Sfloat);
	addRenderTarget("albedo", vk::Format::eR8G8B8A8Unorm);
	addRenderTarget("srm", vk::Format::eR8G8B8A8Unorm); // Specular Roughness Metallic
	addRenderTarget("ssao", vk::Format::eR16Unorm);
	addRenderTarget("ssaoBlur", vk::Format::eR8Unorm);
	addRenderTarget("ssr", vk::Format::eR8G8B8A8Unorm);
	addRenderTarget("composition", vk::Format::eR8G8B8A8Unorm);
	addRenderTarget("composition2", vk::Format::eR8G8B8A8Unorm);
	addRenderTarget("velocity", vk::Format::eR16G16B16A16Sfloat);
	addRenderTarget("brightFilter", vk::Format::eR8G8B8A8Unorm);
	addRenderTarget("gaussianBlurHorizontal", vk::Format::eR8G8B8A8Unorm);
	addRenderTarget("gaussianBlurVertical", vk::Format::eR8G8B8A8Unorm);
	addRenderTarget("emissive", vk::Format::eR8G8B8A8Unorm);

	deferred.createRenderPasses(renderTargets);
	deferred.createFrameBuffers(renderTargets);
	deferred.createPipelines(renderTargets);
	deferred.updateDescriptorSets(renderTargets, lightUniforms);

	ssr.createRenderPass(renderTargets);
	ssr.createFrameBuffers(renderTargets);
	ssr.createPipeline(renderTargets);
	ssr.updateDescriptorSets(renderTargets);

	fxaa.createRenderPass(renderTargets);
	fxaa.createFrameBuffers(renderTargets);
	fxaa.createPipeline(renderTargets);
	fxaa.updateDescriptorSets(renderTargets);

	bloom.createRenderPasses(renderTargets);
	bloom.createFrameBuffers(renderTargets);
	bloom.createPipelines(renderTargets);
	bloom.updateDescriptorSets(renderTargets);

	motionBlur.createRenderPass();
	motionBlur.createFrameBuffers();
	motionBlur.createPipeline();
	motionBlur.updateDescriptorSets(renderTargets);

	ssao.createRenderPasses(renderTargets);
	ssao.createFrameBuffers(renderTargets);
	ssao.createPipelines(renderTargets);
	ssao.updateDescriptorSets(renderTargets);

	gui.createRenderPass();
	gui.createFrameBuffers();
	gui.createPipeline();

	skyBoxDay.createRenderPass();
	skyBoxDay.createFrameBuffers();
	skyBoxDay.createPipeline();
	skyBoxNight.createRenderPass();
	skyBoxNight.createFrameBuffers();
	skyBoxNight.createPipeline();

	//compute.pipeline = createComputePipeline();
	//compute.updateDescriptorSets();
	//- Recreate resources end --------------
}

// Callbacks for scripts -------------------
static void LoadModel(MonoString* folderPath, MonoString* modelName, uint32_t instances)
{
	std::string curPath = std::filesystem::current_path().string() + "\\";
	std::string path(mono_string_to_utf8(folderPath));
	std::string name(mono_string_to_utf8(modelName));
	for (; instances > 0; instances--)
		Queue::loadModel.push_back({ curPath + path, name });
}

static bool KeyDown(uint32_t key)
{
	return ImGui::GetIO().KeysDown[key];
}

static bool MouseButtonDown(uint32_t button)
{
	return ImGui::GetIO().MouseDown[button];
}

static ImVec2 GetMousePos()
{
	return ImGui::GetIO().MousePos;
}

static void SetMousePos(float x, float y)
{
	SDL_WarpMouseInWindow(GUI::g_Window, static_cast<int>(x), static_cast<int>(y));
}

static float GetMouseWheel()
{
	return ImGui::GetIO().MouseWheel;
}

static void SetTimeScale(float timeScale)
{
	GUI::timeScale = timeScale;
}
// ----------------------------------------

void Context::loadResources()
{
	// SKYBOXES LOAD
	std::array<std::string, 6> skyTextures = {
		"objects/sky/right.png",
		"objects/sky/left.png",
		"objects/sky/top.png",
		"objects/sky/bottom.png",
		"objects/sky/back.png",
		"objects/sky/front.png" };
	skyBoxDay.loadSkyBox(skyTextures, 1024);
	skyTextures = { 
		"objects/lmcity/lmcity_rt.png",
		"objects/lmcity/lmcity_lf.png",
		"objects/lmcity/lmcity_up.png",
		"objects/lmcity/lmcity_dn.png",
		"objects/lmcity/lmcity_bk.png",
		"objects/lmcity/lmcity_ft.png" };
	skyBoxNight.loadSkyBox(skyTextures, 512);

	// GUI LOAD
	gui.loadGUI();

	// SCRIPTS
#ifdef USE_SCRIPTS
	Script::Init();
	Script::addCallback("Global::LoadModel", LoadModel);
	Script::addCallback("Global::KeyDown", KeyDown);
	Script::addCallback("Global::SetTimeScale", SetTimeScale);
	Script::addCallback("Global::MouseButtonDown", MouseButtonDown);
	Script::addCallback("Global::GetMousePos", GetMousePos);
	Script::addCallback("Global::SetMousePos", SetMousePos);
	Script::addCallback("Global::GetMouseWheel", GetMouseWheel);
	scripts.push_back(new Script("Load"));
#endif
}

void Context::createUniforms()
{
	// DESCRIPTOR SETS FOR GUI
	gui.createDescriptorSet(GUI::getDescriptorSetLayout(vulkan.device));
	// DESCRIPTOR SETS FOR SKYBOX
	skyBoxDay.createUniformBuffer(2 * sizeof(mat4));
	skyBoxDay.createDescriptorSet(SkyBox::getDescriptorSetLayout(vulkan.device));
	skyBoxNight.createUniformBuffer(2 * sizeof(mat4));
	skyBoxNight.createDescriptorSet(SkyBox::getDescriptorSetLayout(vulkan.device));
	// DESCRIPTOR SETS FOR SHADOWS
	shadows.createUniformBuffers();
	shadows.createDescriptorSets();
	// DESCRIPTOR SETS FOR LIGHTS
	lightUniforms.createLightUniforms();
	// DESCRIPTOR SETS FOR SSAO
	ssao.createUniforms(renderTargets);
	// DESCRIPTOR SETS FOR COMPOSITION PIPELINE
	deferred.createDeferredUniforms(renderTargets, lightUniforms);
	// DESCRIPTOR SET FOR REFLECTION PIPELINE
	ssr.createSSRUniforms(renderTargets);
	// DESCRIPTOR SET FOR FXAA PIPELINE
	fxaa.createUniforms(renderTargets);
	// DESCRIPTOR SET FOR BLOOM PIPELINEs
	bloom.createUniforms(renderTargets);
	// DESCRIPTOR SET FOR MOTIONBLUR PIPELINE
	motionBlur.createMotionBlurUniforms(renderTargets);
	// DESCRIPTOR SET FOR COMPUTE PIPELINE
	//compute.createComputeUniforms(sizeof(SBOIn));
}

vk::Instance Context::createInstance()
{
	unsigned extCount;
	if (!SDL_Vulkan_GetInstanceExtensions(vulkan.window, &extCount, nullptr))
		throw std::runtime_error(SDL_GetError());

	std::vector<const char*> instanceExtensions(extCount);
	if (!SDL_Vulkan_GetInstanceExtensions(vulkan.window, &extCount, instanceExtensions.data()))
		throw std::runtime_error(SDL_GetError());

	std::vector<const char*> instanceLayers{};
#ifdef _DEBUG
	instanceLayers.push_back("VK_LAYER_KHRONOS_validation");
	instanceLayers.push_back("VK_LAYER_LUNARG_assistant_layer");
#endif

	vk::ApplicationInfo appInfo;
	appInfo.pApplicationName = "VulkanMonkey3D";
	appInfo.pEngineName = "VulkanMonkey3D";
	appInfo.apiVersion = VK_MAKE_VERSION(1, 1, 0);

	vk::InstanceCreateInfo instInfo;
	instInfo.pApplicationInfo = &appInfo;
	instInfo.enabledLayerCount = static_cast<uint32_t>(instanceLayers.size());
	instInfo.ppEnabledLayerNames = instanceLayers.data();
	instInfo.enabledExtensionCount = (uint32_t)(instanceExtensions.size());
	instInfo.ppEnabledExtensionNames = instanceExtensions.data();

	return vk::createInstance(instInfo);
}

Surface Context::createSurface()
{
	VkSurfaceKHR _vkSurface;
	if (!SDL_Vulkan_CreateSurface(vulkan.window, VkInstance(vulkan.instance), &_vkSurface))
		throw std::runtime_error(SDL_GetError());

	Surface _surface;
	int width, height;
	SDL_GL_GetDrawableSize(vulkan.window, &width, &height);
	_surface.actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
	_surface.surface = vk::SurfaceKHR(_vkSurface);

	return _surface;
}

int Context::getGraphicsFamilyId()
{
	std::vector<vk::PhysicalDevice> gpuList = vulkan.instance.enumeratePhysicalDevices();

	for (const auto& gpu : gpuList) {

		std::vector<vk::QueueFamilyProperties> properties = gpu.getQueueFamilyProperties();

		for (int i = 0; i < properties.size(); i++) {
			//find graphics queue family index
			if (properties[i].queueFlags & vk::QueueFlagBits::eGraphics)
				return i;
		}
	}
	return -1;
}

int Context::getPresentFamilyId()
{
	std::vector<vk::PhysicalDevice> gpuList = vulkan.instance.enumeratePhysicalDevices();

	for (const auto& gpu : gpuList) {

		std::vector<vk::QueueFamilyProperties> properties = gpu.getQueueFamilyProperties();

		for (uint32_t i = 0; i < properties.size(); ++i) {
			// find present queue family index
			if (properties[i].queueCount > 0 && gpu.getSurfaceSupportKHR(i, vulkan.surface->surface))
				return i;
		}
	}
	return -1;
}

int Context::getComputeFamilyId()
{
	std::vector<vk::PhysicalDevice> gpuList = vulkan.instance.enumeratePhysicalDevices();

	for (const auto& gpu : gpuList) {
		std::vector<vk::QueueFamilyProperties> properties = gpu.getQueueFamilyProperties();

		for (uint32_t i = 0; i < properties.size(); ++i) {
			//find compute queue family index
			if (properties[i].queueFlags & vk::QueueFlagBits::eCompute)
				return i;
		}
	}
	return -1;
}

vk::PhysicalDevice Context::findGpu()
{
	if (vulkan.graphicsFamilyId < 0 || vulkan.presentFamilyId < 0 || vulkan.computeFamilyId < 0)
		return nullptr;
	std::vector<vk::PhysicalDevice> gpuList = vulkan.instance.enumeratePhysicalDevices();

	for (const auto& gpu : gpuList) {
		std::vector<vk::QueueFamilyProperties> properties = gpu.getQueueFamilyProperties();

		if (properties[vulkan.graphicsFamilyId].queueFlags & vk::QueueFlagBits::eGraphics &&
			gpu.getSurfaceSupportKHR(vulkan.presentFamilyId, vulkan.surface->surface) &&
			properties[vulkan.computeFamilyId].queueFlags & vk::QueueFlagBits::eCompute)
			return gpu;
	}
	return nullptr;
}

vk::SurfaceCapabilitiesKHR Context::getSurfaceCapabilities()
{
	return vulkan.gpu.getSurfaceCapabilitiesKHR(vulkan.surface->surface);
}

vk::SurfaceFormatKHR Context::getSurfaceFormat()
{
	std::vector<vk::SurfaceFormatKHR> formats = vulkan.gpu.getSurfaceFormatsKHR(vulkan.surface->surface);

	for (const auto& i : formats) {
		if (i.format == vk::Format::eB8G8R8A8Unorm && i.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
			return i;
	}

	return formats[0];
}

vk::PresentModeKHR Context::getPresentationMode()
{
	std::vector<vk::PresentModeKHR> presentModes = vulkan.gpu.getSurfacePresentModesKHR(vulkan.surface->surface);

	for (const auto& i : presentModes)
		if (i == vk::PresentModeKHR::eImmediate || i == vk::PresentModeKHR::eMailbox)
			return i;

	return vk::PresentModeKHR::eFifo;
}

vk::PhysicalDeviceProperties Context::getGPUProperties()
{
	return vulkan.gpu.getProperties();
}

vk::PhysicalDeviceFeatures Context::getGPUFeatures()
{
	return vulkan.gpu.getFeatures();
}

vk::Device Context::createDevice()
{
	std::vector<vk::ExtensionProperties> extensionProperties = vulkan.gpu.enumerateDeviceExtensionProperties();

	std::vector<const char*> deviceExtensions{};
	for (auto& i : extensionProperties) {
		if (std::string(i.extensionName) == VK_KHR_SWAPCHAIN_EXTENSION_NAME)
			deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	}
	float priorities[]{ 1.0f }; // range : [0.0, 1.0]

	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{};

	// graphics queue
	queueCreateInfos.push_back({});
	queueCreateInfos.back().queueFamilyIndex = vulkan.graphicsFamilyId;
	queueCreateInfos.back().queueCount = 1;
	queueCreateInfos.back().pQueuePriorities = priorities;

	// compute queue
	if (vulkan.computeFamilyId != vulkan.graphicsFamilyId) {
		queueCreateInfos.push_back({});
		queueCreateInfos.back().queueFamilyIndex = vulkan.computeFamilyId;
		queueCreateInfos.back().queueCount = 1;
		queueCreateInfos.back().pQueuePriorities = priorities;
	}

	vk::DeviceCreateInfo deviceCreateInfo;
	deviceCreateInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
	deviceCreateInfo.pEnabledFeatures = &vulkan.gpuFeatures;

	return vulkan.gpu.createDevice(deviceCreateInfo);
}

vk::Queue Context::getGraphicsQueue()
{
	return vulkan.device.getQueue(vulkan.graphicsFamilyId, 0);
}

vk::Queue Context::getPresentQueue()
{
	return vulkan.device.getQueue(vulkan.presentFamilyId, 0);
}

vk::Queue Context::getComputeQueue()
{
	return vulkan.device.getQueue(vulkan.computeFamilyId, 0);
}

Swapchain Context::createSwapchain()
{
	VkExtent2D extent = vulkan.surface->actualExtent;
	vulkan.surface->capabilities = getSurfaceCapabilities();
	Swapchain _swapchain;

	uint32_t swapchainImageCount = vulkan.surface->capabilities.minImageCount + 1;
	if (vulkan.surface->capabilities.maxImageCount > 0 &&
		swapchainImageCount > vulkan.surface->capabilities.maxImageCount) {
		swapchainImageCount = vulkan.surface->capabilities.maxImageCount;
	}

	vk::SwapchainKHR oldSwapchain = _swapchain.swapchain;
	vk::SwapchainCreateInfoKHR swapchainCreateInfo;
	swapchainCreateInfo.surface = vulkan.surface->surface;
	swapchainCreateInfo.minImageCount = swapchainImageCount;
	swapchainCreateInfo.imageFormat = vulkan.surface->formatKHR.format;
	swapchainCreateInfo.imageColorSpace = vulkan.surface->formatKHR.colorSpace;
	swapchainCreateInfo.imageExtent = extent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
	swapchainCreateInfo.preTransform = vulkan.surface->capabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	swapchainCreateInfo.presentMode = vulkan.surface->presentModeKHR;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.oldSwapchain = oldSwapchain;

	// new swapchain with old create info
	vk::SwapchainKHR newSwapchain = vulkan.device.createSwapchainKHR(swapchainCreateInfo);

	if (_swapchain.swapchain)
		vulkan.device.destroySwapchainKHR(_swapchain.swapchain);
	_swapchain.swapchain = newSwapchain;

	// get the swapchain image handlers
	std::vector<vk::Image> images = vulkan.device.getSwapchainImagesKHR(_swapchain.swapchain);

	_swapchain.images.resize(images.size());
	for (unsigned i = 0; i < images.size(); i++) {
		_swapchain.images[i].image = images[i]; // hold the image handlers
		_swapchain.images[i].blentAttachment.blendEnable = VK_TRUE;
		_swapchain.images[i].blentAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
		_swapchain.images[i].blentAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		_swapchain.images[i].blentAttachment.colorBlendOp = vk::BlendOp::eAdd;
		_swapchain.images[i].blentAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
		_swapchain.images[i].blentAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
		_swapchain.images[i].blentAttachment.alphaBlendOp = vk::BlendOp::eAdd;
		_swapchain.images[i].blentAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	}

	// create image views for each swapchain image
	for (auto &image : _swapchain.images) {
		vk::ImageViewCreateInfo imageViewCreateInfo;
		imageViewCreateInfo.image = image.image;
		imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
		imageViewCreateInfo.format = vulkan.surface->formatKHR.format;
		imageViewCreateInfo.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
		image.view = vulkan.device.createImageView(imageViewCreateInfo);
	}

	return _swapchain;
}

vk::CommandPool Context::createCommandPool()
{
	vk::CommandPoolCreateInfo cpci;
	cpci.queueFamilyIndex = vulkan.graphicsFamilyId;
	cpci.flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

	return vulkan.device.createCommandPool(cpci);
}

void Context::addRenderTarget(const std::string& name, vk::Format format)
{
	if (renderTargets.find(name) != renderTargets.end()) {
		std::cout << "Render Target already exists\n";
		return;
	}
	renderTargets[name] = Image();
	renderTargets[name].format = format;
	renderTargets[name].initialLayout = vk::ImageLayout::eUndefined;
	renderTargets[name].createImage(WIDTH, HEIGHT, vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal);
	renderTargets[name].transitionImageLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);
	renderTargets[name].createImageView(vk::ImageAspectFlagBits::eColor);
	renderTargets[name].createSampler();

	//std::string str = to_string(format); str.find("A8") != std::string::npos
	renderTargets[name].blentAttachment.blendEnable = name == "albedo" ? VK_TRUE : VK_FALSE;
	renderTargets[name].blentAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	renderTargets[name].blentAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	renderTargets[name].blentAttachment.colorBlendOp = vk::BlendOp::eAdd;
	renderTargets[name].blentAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
	renderTargets[name].blentAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
	renderTargets[name].blentAttachment.alphaBlendOp = vk::BlendOp::eAdd;
	renderTargets[name].blentAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
}

Image Context::createDepthResources()
{
	Image _image = Image();
	_image.format = vk::Format::eUndefined;
	std::vector<vk::Format> candidates = { vk::Format::eD32SfloatS8Uint, vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint };
	for (auto &format : candidates) {
		vk::FormatProperties props = vulkan.gpu.getFormatProperties(format);
		if ((props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) == vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
			_image.format = format;
			break;
		}
	}
	if (_image.format == vk::Format::eUndefined)
		throw std::runtime_error("Depth format is undefined");

	_image.createImage(WIDTH, HEIGHT, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal);
	_image.createImageView(vk::ImageAspectFlagBits::eDepth);

	_image.addressMode = vk::SamplerAddressMode::eClampToEdge;
	_image.maxAnisotropy = 1.f;
	_image.borderColor = vk::BorderColor::eFloatOpaqueWhite;
	_image.samplerCompareEnable = VK_TRUE;
	_image.createSampler();

	_image.transitionImageLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);

	return _image;
}

std::vector<vk::CommandBuffer> Context::createCmdBuffers(const uint32_t bufferCount)
{
	vk::CommandBufferAllocateInfo cbai;
	cbai.commandPool = vulkan.commandPool;
	cbai.level = vk::CommandBufferLevel::ePrimary;
	cbai.commandBufferCount = bufferCount;

	return vulkan.device.allocateCommandBuffers(cbai);
}

vk::DescriptorPool Context::createDescriptorPool(uint32_t maxDescriptorSets)
{
	std::vector<vk::DescriptorPoolSize> descPoolsize(4);
	descPoolsize[0].type = vk::DescriptorType::eUniformBuffer;
	descPoolsize[0].descriptorCount =  maxDescriptorSets;
	descPoolsize[1].type = vk::DescriptorType::eStorageBuffer;
	descPoolsize[1].descriptorCount = maxDescriptorSets;
	descPoolsize[2].type = vk::DescriptorType::eInputAttachment;
	descPoolsize[2].descriptorCount = maxDescriptorSets;
	descPoolsize[3].type = vk::DescriptorType::eCombinedImageSampler;
	descPoolsize[3].descriptorCount = maxDescriptorSets;

	vk::DescriptorPoolCreateInfo createInfo;
	createInfo.poolSizeCount = (uint32_t)descPoolsize.size();
	createInfo.pPoolSizes = descPoolsize.data();
	createInfo.maxSets = maxDescriptorSets;

	return vulkan.device.createDescriptorPool(createInfo);
}

std::vector<vk::Fence> Context::createFences(const uint32_t fenceCount)
{
	std::vector<vk::Fence> _fences(fenceCount);
	vk::FenceCreateInfo fi;

	for (uint32_t i = 0; i < fenceCount; i++) {
		_fences[i] = vulkan.device.createFence(fi);
	}

	return _fences;
}

std::vector<vk::Semaphore> Context::createSemaphores(const uint32_t semaphoresCount)
{
	std::vector<vk::Semaphore> _semaphores(semaphoresCount);
	vk::SemaphoreCreateInfo si;

	for (uint32_t i = 0; i < semaphoresCount; i++) {
		_semaphores[i] = vulkan.device.createSemaphore(si);
	}

	return _semaphores;
}

void Context::destroyVkContext()
{
	for (auto& fence : vulkan.fences) {
		if (fence) {
			vulkan.device.destroyFence(fence);
			fence = nullptr;
		}
	}
	for (auto &semaphore : vulkan.semaphores) {
		if (semaphore) {
			vulkan.device.destroySemaphore(semaphore);
			semaphore = nullptr;
		}
	}
	for (auto& rt : renderTargets)
		rt.second.destroy();

	vulkan.depth->destroy();
	delete vulkan.depth;

	if (vulkan.descriptorPool) {
		vulkan.device.destroyDescriptorPool(vulkan.descriptorPool);
	}
	if (vulkan.commandPool) {
		vulkan.device.destroyCommandPool(vulkan.commandPool);
	}
	if (vulkan.commandPool2) {
		vulkan.device.destroyCommandPool(vulkan.commandPool2);
	}

	vulkan.swapchain->destroy();
	delete vulkan.swapchain;

	if (vulkan.device) {
		vulkan.device.destroy();
	}

	if (vulkan.surface->surface) {
		vulkan.instance.destroySurfaceKHR(vulkan.surface->surface);
	}delete vulkan.surface;

	if (vulkan.instance) {
		vulkan.instance.destroy();
	}
}