#include "../include/SSR.h"
#include "../include/Errors.h"

using namespace vm;

vm::SSR::SSR(VulkanContext * vulkan) : vulkan(vulkan)
{ }

void SSR::createSSRUniforms(std::map<std::string, Image>& renderTargets)
{
	UBReflection.createBuffer(256, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostCoherent);
	VkCheck(vulkan->device.mapMemory(UBReflection.memory, 0, UBReflection.size, vk::MemoryMapFlags(), &UBReflection.data));

	auto const allocateInfo2 = vk::DescriptorSetAllocateInfo()
		.setDescriptorPool(vulkan->descriptorPool)
		.setDescriptorSetCount(1)
		.setPSetLayouts(&DSLayoutReflection);
	VkCheck(vulkan->device.allocateDescriptorSets(&allocateInfo2, &DSReflection));

	vk::WriteDescriptorSet textureWriteSets[5];
	// Albedo
	textureWriteSets[0] = vk::WriteDescriptorSet()
		.setDstSet(DSReflection)									// DescriptorSet dstSet;
		.setDstBinding(0)												// uint32_t dstBinding;
		.setDstArrayElement(0)											// uint32_t dstArrayElement;
		.setDescriptorCount(1)											// uint32_t descriptorCount;
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)	// DescriptorType descriptorType;
		.setPImageInfo(&vk::DescriptorImageInfo()						// const DescriptorImageInfo* pImageInfo;
			.setSampler(renderTargets["albedo"].sampler)					// Sampler sampler;
			.setImageView(renderTargets["albedo"].view)					// ImageView imageView;
			.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal));		// ImageLayout imageLayout;
	// Positions
	textureWriteSets[1] = vk::WriteDescriptorSet()
		.setDstSet(DSReflection)									// DescriptorSet dstSet;
		.setDstBinding(1)												// uint32_t dstBinding;
		.setDstArrayElement(0)											// uint32_t dstArrayElement;
		.setDescriptorCount(1)											// uint32_t descriptorCount;
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)	// DescriptorType descriptorType;
		.setPImageInfo(&vk::DescriptorImageInfo()						// const DescriptorImageInfo* pImageInfo;
			.setSampler(renderTargets["position"].sampler)				// Sampler sampler;
			.setImageView(renderTargets["position"].view)				// ImageView imageView;
			.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal));		// ImageLayout imageLayout;
	// Normals
	textureWriteSets[2] = vk::WriteDescriptorSet()
		.setDstSet(DSReflection)									// DescriptorSet dstSet;
		.setDstBinding(2)												// uint32_t dstBinding;
		.setDstArrayElement(0)											// uint32_t dstArrayElement;
		.setDescriptorCount(1)											// uint32_t descriptorCount;
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)	// DescriptorType descriptorType;
		.setPImageInfo(&vk::DescriptorImageInfo()						// const DescriptorImageInfo* pImageInfo;
			.setSampler(renderTargets["normal"].sampler)					// Sampler sampler;
			.setImageView(renderTargets["normal"].view)					// ImageView imageView;
			.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal));		// ImageLayout imageLayout;
	// Specular
	textureWriteSets[3] = vk::WriteDescriptorSet()
		.setDstSet(DSReflection)									// DescriptorSet dstSet;
		.setDstBinding(3)												// uint32_t dstBinding;
		.setDstArrayElement(0)											// uint32_t dstArrayElement;
		.setDescriptorCount(1)											// uint32_t descriptorCount;
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)	// DescriptorType descriptorType;
		.setPImageInfo(&vk::DescriptorImageInfo()						// const DescriptorImageInfo* pImageInfo;
			.setSampler(renderTargets["specular"].sampler)				// Sampler sampler;
			.setImageView(renderTargets["specular"].view)				// ImageView imageView;
			.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal));		// ImageLayout imageLayout;
	// Uniform variables
	textureWriteSets[4] = vk::WriteDescriptorSet()
		.setDstSet(DSReflection)									// DescriptorSet dstSet;
		.setDstBinding(4)												// uint32_t dstBinding;
		.setDstArrayElement(0)											// uint32_t dstArrayElement;
		.setDescriptorCount(1)											// uint32_t descriptorCount;
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)			// DescriptorType descriptorType;
		.setPBufferInfo(&vk::DescriptorBufferInfo()						// const DescriptorImageInfo* pImageInfo;
			.setBuffer(UBReflection.buffer)								// Buffer buffer;
			.setOffset(0)													// DeviceSize offset;
			.setRange(3 * 64));									// DeviceSize range;

	vulkan->device.updateDescriptorSets(5, textureWriteSets, 0, nullptr);
	std::cout << "DescriptorSet allocated and updated\n";
}

void vm::SSR::updateDescriptorSets(std::map<std::string, Image>& renderTargets)
{

	vk::WriteDescriptorSet textureWriteSets[5];
	// Albedo
	textureWriteSets[0] = vk::WriteDescriptorSet()
		.setDstSet(DSReflection)									// DescriptorSet dstSet;
		.setDstBinding(0)												// uint32_t dstBinding;
		.setDstArrayElement(0)											// uint32_t dstArrayElement;
		.setDescriptorCount(1)											// uint32_t descriptorCount;
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)	// DescriptorType descriptorType;
		.setPImageInfo(&vk::DescriptorImageInfo()						// const DescriptorImageInfo* pImageInfo;
			.setSampler(renderTargets["albedo"].sampler)					// Sampler sampler;
			.setImageView(renderTargets["albedo"].view)					// ImageView imageView;
			.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal));		// ImageLayout imageLayout;
	// Positions
	textureWriteSets[1] = vk::WriteDescriptorSet()
		.setDstSet(DSReflection)									// DescriptorSet dstSet;
		.setDstBinding(1)												// uint32_t dstBinding;
		.setDstArrayElement(0)											// uint32_t dstArrayElement;
		.setDescriptorCount(1)											// uint32_t descriptorCount;
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)	// DescriptorType descriptorType;
		.setPImageInfo(&vk::DescriptorImageInfo()						// const DescriptorImageInfo* pImageInfo;
			.setSampler(renderTargets["position"].sampler)				// Sampler sampler;
			.setImageView(renderTargets["position"].view)				// ImageView imageView;
			.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal));		// ImageLayout imageLayout;
	// Normals
	textureWriteSets[2] = vk::WriteDescriptorSet()
		.setDstSet(DSReflection)									// DescriptorSet dstSet;
		.setDstBinding(2)												// uint32_t dstBinding;
		.setDstArrayElement(0)											// uint32_t dstArrayElement;
		.setDescriptorCount(1)											// uint32_t descriptorCount;
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)	// DescriptorType descriptorType;
		.setPImageInfo(&vk::DescriptorImageInfo()						// const DescriptorImageInfo* pImageInfo;
			.setSampler(renderTargets["normal"].sampler)					// Sampler sampler;
			.setImageView(renderTargets["normal"].view)					// ImageView imageView;
			.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal));		// ImageLayout imageLayout;
	// Specular
	textureWriteSets[3] = vk::WriteDescriptorSet()
		.setDstSet(DSReflection)									// DescriptorSet dstSet;
		.setDstBinding(3)												// uint32_t dstBinding;
		.setDstArrayElement(0)											// uint32_t dstArrayElement;
		.setDescriptorCount(1)											// uint32_t descriptorCount;
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)	// DescriptorType descriptorType;
		.setPImageInfo(&vk::DescriptorImageInfo()						// const DescriptorImageInfo* pImageInfo;
			.setSampler(renderTargets["specular"].sampler)				// Sampler sampler;
			.setImageView(renderTargets["specular"].view)				// ImageView imageView;
			.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal));		// ImageLayout imageLayout;
	// Uniform variables
	textureWriteSets[4] = vk::WriteDescriptorSet()
		.setDstSet(DSReflection)									// DescriptorSet dstSet;
		.setDstBinding(4)												// uint32_t dstBinding;
		.setDstArrayElement(0)											// uint32_t dstArrayElement;
		.setDescriptorCount(1)											// uint32_t descriptorCount;
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)			// DescriptorType descriptorType;
		.setPBufferInfo(&vk::DescriptorBufferInfo()						// const DescriptorImageInfo* pImageInfo;
			.setBuffer(UBReflection.buffer)								// Buffer buffer;
			.setOffset(0)													// DeviceSize offset;
			.setRange(3 * 64));									// DeviceSize range;

	vulkan->device.updateDescriptorSets(5, textureWriteSets, 0, nullptr);
	std::cout << "DescriptorSet updated\n";
}

void vm::SSR::destroy()
{
	for (auto &frameBuffer : frameBuffers) {
		if (frameBuffer) {
			vulkan->device.destroyFramebuffer(frameBuffer);
			std::cout << "Frame Buffer destroyed\n";
		}
	}
	if (renderPass) {
		vulkan->device.destroyRenderPass(renderPass);
		renderPass = nullptr;
		std::cout << "RenderPass destroyed\n";
	}
	if (DSLayoutReflection) {
		vulkan->device.destroyDescriptorSetLayout(DSLayoutReflection);
		DSLayoutReflection = nullptr;
		std::cout << "Descriptor Set Layout destroyed\n";
	}
	UBReflection.destroy();
	pipeline.destroy();
}
