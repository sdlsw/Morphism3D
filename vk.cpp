#include "vk.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <set>
#include <fstream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>

#ifndef NDEBUG
static const bool ENABLE_VALIDATION_LAYERS = false;
#else
static const bool ENABLE_VALIDATION_LAYERS = true;
#endif

static const std::vector<const char*> VALIDATION_LAYERS = {
	"VK_LAYER_KHRONOS_validation"
};

static const std::vector<const char*> REQUIRED_DEVICE_EXTENSIONS = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Does this physical device support all the extensions we need?
static bool pdevHasRequiredExtensions(const vk::raii::PhysicalDevice& device) {
	std::set<std::string> extSet(REQUIRED_DEVICE_EXTENSIONS.begin(), REQUIRED_DEVICE_EXTENSIONS.end());
	for (const auto& ext : device.enumerateDeviceExtensionProperties()) {
		extSet.erase(ext.extensionName);
	}

	bool hasRequired = extSet.empty();

	if (!hasRequired) {
		for (const auto& extName : extSet) {
			std::cerr << "missing required extension \"" << extName << "\"" << std::endl;
		}
	}

	return hasRequired;
}

static std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t file_size = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(file_size);

	file.seekg(0);
	file.read(buffer.data(), file_size);
	file.close();

	std::cerr << "read file \"" << filename << "\", " << buffer.size() << " bytes" << std::endl;

	return buffer;
}

namespace g3d {
std::ostream& operator<<(std::ostream& o, const glm::vec3& v) {
	o << v.x << ", " << v.y << ", " << v.z;
	return o;
}

bool QueueFamilyIndices::isComplete() const {
	return graphicsFamily.has_value() && presentFamily.has_value();
}

VkTop::VkTop(const vk::ApplicationInfo& appInfo) 
: _appInfo {appInfo},
  _instance { createInstance() },
  _physicalDevices { _instance }
{
	size_t dev_count = _physicalDevices.size();

	if (dev_count != 0) {
		std::cerr << "enumerated " << dev_count << " device(s) with Vulkan support" << std::endl;
	} else {
		throw std::runtime_error("failed to find a device with Vulkan support!");
	}
}

vk::raii::Instance VkTop::createInstance() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	if (glfwExtensionCount != 0) {
		std::cerr << "glfwExtensions:" << std::endl;
		for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
			std::cerr << '\t' << glfwExtensions[i] << std::endl;
		}
	} else {
		std::cerr << "GLFW reported no required extensions" << std::endl;
	}

	vk::InstanceCreateInfo info {
		.pApplicationInfo = &_appInfo,
		.enabledExtensionCount = glfwExtensionCount,
		.ppEnabledExtensionNames = glfwExtensions
	};

	if (ENABLE_VALIDATION_LAYERS) {
		info.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
		info.ppEnabledLayerNames = VALIDATION_LAYERS.data();
	}

	return vk::raii::Instance(_context, info);
}

vk::raii::CommandBuffer OneTimeCommandBuffer::createCommandBuffer(
	vk::raii::CommandPool& pool
) {
	vk::CommandBufferAllocateInfo allocInfo {
		.commandPool = *pool,
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = 1
	};

	return std::move(_device->allocateCommandBuffers(allocInfo)[0]);
}

OneTimeCommandBuffer::~OneTimeCommandBuffer() {
	if(!_shouldSubmit) return;

	_buffer.end();

	vk::SubmitInfo submitInfo {
		.commandBufferCount = 1,
		.pCommandBuffers = &(*_buffer)
	};
	_submitQueue->submit(submitInfo);
	_submitQueue->waitIdle();
}

vk::raii::DescriptorSetLayout DescriptorSetFactory::createDescriptorSetLayout() {
	vk::DescriptorSetLayoutBinding mvpLayoutBinding {
		.binding = 0,
		.descriptorType = vk::DescriptorType::eUniformBuffer,
		.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eVertex
	};

	vk::DescriptorSetLayoutCreateInfo createInfo {
		.bindingCount = static_cast<uint32_t>(_layoutBindings.size()),
		.pBindings = _layoutBindings.data()
	};
	return vk::raii::DescriptorSetLayout(_graphDevice->logicalDevice(), createInfo);
}

vk::raii::DescriptorPool DescriptorSetFactory::createDescriptorPool() {
	std::vector<vk::DescriptorPoolSize> poolSizes;

	for (const auto& binding : _layoutBindings) {
		poolSizes.emplace_back(
			binding.descriptorType,
			binding.descriptorCount * MAX_FRAMES_IN_FLIGHT * MAX_DESCRIPTOR_SETS
		);
	}

	vk::DescriptorPoolCreateInfo info {
		.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		.maxSets = MAX_FRAMES_IN_FLIGHT * MAX_DESCRIPTOR_SETS,
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		.pPoolSizes = poolSizes.data()
	};

	return vk::raii::DescriptorPool(_graphDevice->logicalDevice(), info);
}

vk::raii::DescriptorSet DescriptorSetFactory::makeDescriptorSet(vk::ArrayProxy<BoundBuffer> buffers) {
	vk::DescriptorSetAllocateInfo allocInfo {
		.descriptorPool = *_descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &(*_descriptorSetLayout)
	};

	auto& logicalDevice = _graphDevice->logicalDevice();
	auto descriptorSet = std::move(logicalDevice.allocateDescriptorSets(allocInfo)[0]);

	std::vector<vk::DescriptorBufferInfo> bufferInfo;
	std::vector<vk::WriteDescriptorSet> writeInfo;
	for (size_t i = 0; i < _layoutBindings.size(); ++i) {
		const auto& binding = _layoutBindings[i];
		bufferInfo.push_back(buffers.data()[i].descriptorInfo());
		writeInfo.push_back({
			.dstSet = *descriptorSet,
			.dstBinding = binding.binding,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = binding.descriptorType,
			.pBufferInfo = &bufferInfo[i]
		});
	}

	_graphDevice->logicalDevice().updateDescriptorSets(writeInfo, nullptr);

	return descriptorSet;
}

QueueFamilyIndices GraphDevice::queryQueueFamilies(const vk::raii::PhysicalDevice& device) const {
	QueueFamilyIndices indices;

	const std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();
	for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
		const auto& familyProperties = queueFamilies[i];
		if (familyProperties.queueFlags & vk::QueueFlagBits::eGraphics) {
			indices.graphicsFamily = i;
		}

		vk::Bool32 presentSupport = device.getSurfaceSupportKHR(i, _surface);
		if (presentSupport) {
			indices.presentFamily = i;
		}

		if (indices.isComplete()) break;
	}

	return indices;
}

SwapChainSupportDetails GraphDevice::_querySwapChainSupport(const vk::raii::PhysicalDevice& device) const {
	SwapChainSupportDetails details {
		device.getSurfaceCapabilitiesKHR(_surface),
		device.getSurfaceFormatsKHR(_surface),
		device.getSurfacePresentModesKHR(_surface)
	};
	
	return details;
}

// Is this physical device supported by our application?
// (For now, just pick the first one that meets every need. TODO May come back to
// this.)
bool GraphDevice::pDevSupported(const vk::raii::PhysicalDevice& device) const {
	if (!queryQueueFamilies(device).isComplete()) {
		std::cerr << "device does not support required queue types" << std::endl;
		return false;
	}

	// FIXME: checking for surface support after creating surface...
	if (!pdevHasRequiredExtensions(device)) {
		std::cerr << "device does not support required extensions" << std::endl;
		return false;
	}

	SwapChainSupportDetails scSupportDetails = _querySwapChainSupport(device);
	if (scSupportDetails.formats.empty()) {
		std::cerr << "window surface supports no formats" << std::endl;
		return false;
	}
	if (scSupportDetails.presentModes.empty()) {
		std::cerr << "window surface supports no presentation modes" << std::endl;
		return false;
	}

	return true;
}

vk::raii::PhysicalDevice& GraphDevice::pickPhysicalDevice(vk::raii::PhysicalDevices& devices) {
	if (devices.size() == 0) {
		throw std::invalid_argument("cannot pick physical device from empty list");
	}

	for (auto& device : devices) {
		if (pDevSupported(device)) {
			std::cerr << "found suitable device: " << device.getProperties().deviceName << std::endl;
			return device;
		}
	}

	throw std::runtime_error("no enumerated device supports required features!");
}

vk::raii::Device GraphDevice::createLogicalDevice() {
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies {
		_queueFamilyIndices.graphicsFamily.value(),
		_queueFamilyIndices.presentFamily.value()
	};

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		queueCreateInfos.push_back({
			.queueFamilyIndex = queueFamily,
			.queueCount = 1,
			.pQueuePriorities = &queuePriority
		});
	}

	vk::PhysicalDeviceFeatures deviceFeatures {};

	vk::DeviceCreateInfo createInfo {
		.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
		.pQueueCreateInfos = queueCreateInfos.data(),
		.enabledExtensionCount = static_cast<uint32_t>(REQUIRED_DEVICE_EXTENSIONS.size()),
		.ppEnabledExtensionNames = REQUIRED_DEVICE_EXTENSIONS.data(),
		.pEnabledFeatures = &deviceFeatures
	};

	if (ENABLE_VALIDATION_LAYERS) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
		createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
	}

	return _physicalDevice->createDevice(createInfo, nullptr);
}

vk::raii::CommandPool GraphDevice::createCommandPool() {
	vk::CommandPoolCreateInfo info {
		.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		.queueFamilyIndex = _queueFamilyIndices.graphicsFamily.value()
	};

	return vk::raii::CommandPool(_device, info);
}

OneTimeCommandBuffer GraphDevice::beginOneTimeCommands() {
	return OneTimeCommandBuffer(_device, _commandPool, _graphicsQueue);
}

vk::Format GraphDevice::findSupportedFormat(
	const std::vector<vk::Format>& candidates,
	vk::ImageTiling tiling,
	vk::FormatFeatureFlags features
) {
	for (vk::Format format : candidates) {
		vk::FormatProperties props = _physicalDevice->getFormatProperties(format);
		if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
			return format;
		} else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format");
}

uint32_t GraphDevice::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
	vk::PhysicalDeviceMemoryProperties memProperties = _physicalDevice->getMemoryProperties();
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
		if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type");
}

vk::raii::DeviceMemory GraphDevice::allocateRequiredMemory(
	const vk::MemoryRequirements& requirements,
	const vk::MemoryPropertyFlags& properties
) {
	vk::MemoryAllocateInfo info {
		.allocationSize = requirements.size,
		.memoryTypeIndex = findMemoryType(requirements.memoryTypeBits, properties)
	};
	return vk::raii::DeviceMemory(_device, info);
}

template<MemoryBindable T>
vk::raii::DeviceMemory GraphDevice::bindNewMemory(T& obj, vk::MemoryPropertyFlags properties) {
	auto memory = allocateRequiredMemory(
		obj.getMemoryRequirements(),
		properties
	);
	obj.bindMemory(memory, 0);
	return memory;
}

vk::raii::ShaderModule GraphDevice::loadShader(const std::string& filename) {
	std::vector<char> code = readFile(filename);
	vk::ShaderModuleCreateInfo createInfo {
		.codeSize = code.size(),
		.pCode = reinterpret_cast<const uint32_t*>(code.data())
	};
	return vk::raii::ShaderModule(_device, createInfo);
}

void BoundBuffer::vkCopyTo(BoundBuffer& other) {
	if (other.info().size != _info.size) {
		throw std::invalid_argument("vkCopyTo: mismatched buffer sizes");
	}

	OneTimeCommandBuffer b = _graphDevice->beginOneTimeCommands();

	vk::BufferCopy copyRegion {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = _info.size
	};
	b.buffer().copyBuffer(_buffer, other.buffer(), copyRegion);
}

void UnsafeMappedBuffer::unsafeCopyIn(const void* obj, size_t size) {
	memcpy(_ptr, obj, size);
}

template<typename T>
void MappedBuffer<T>::copyIn(const T& obj) {
	memcpy(_ptr, &obj, sizeof(T));
}

vk::raii::Pipeline PipelineBuilder::build() {
	if (_vertexShader.size() == 0) {
		throw std::runtime_error("Vertex shader must be specified");
	}

	if (_fragmentShader.size() == 0) {
		throw std::runtime_error("Fragment shader must be specified");
	}

	// SHADERS
	vk::raii::ShaderModule vertModule = _graphDevice->loadShader(_vertexShader);
	vk::raii::ShaderModule fragModule = _graphDevice->loadShader(_fragmentShader);

	vk::PipelineShaderStageCreateInfo vertShaderInfo {
		.stage = vk::ShaderStageFlagBits::eVertex,
		.module = vertModule,
		.pName = "main"
	};

	vk::PipelineShaderStageCreateInfo fragShaderInfo {
		.stage = vk::ShaderStageFlagBits::eFragment,
		.module = fragModule,
		.pName = "main"
	};

	vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderInfo, fragShaderInfo};

	// DYNAMIC STATE
	std::array<vk::DynamicState, 2> dynamicStates {
		vk::DynamicState::eViewport, vk::DynamicState::eScissor
	};

	vk::PipelineDynamicStateCreateInfo dynamicStateInfo {
		.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
		.pDynamicStates = dynamicStates.data()
	};

	// VERTEX INPUT
	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();
	vk::PipelineVertexInputStateCreateInfo vertexInputInfo {
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &bindingDescription,
		.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
		.pVertexAttributeDescriptions = attributeDescriptions.data()
	};

	// INPUT ASSEMBLY
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo {
		.topology = _topology,
		.primitiveRestartEnable = vk::False
	};

	// PORT STATE (DYNAMIC)
	vk::PipelineViewportStateCreateInfo viewportInfo {
		.viewportCount = 1,
		.scissorCount = 1
	};

	// RASTERIZER
	vk::PipelineRasterizationStateCreateInfo rasterInfo {
		.depthClampEnable = vk::False,
		.rasterizerDiscardEnable = vk::False,
		.polygonMode = vk::PolygonMode::eFill,
		.cullMode = vk::CullModeFlagBits::eNone,
		.frontFace = vk::FrontFace::eCounterClockwise,
		.depthBiasEnable = vk::False,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp = 0.0f,
		.depthBiasSlopeFactor = 0.0f,
		.lineWidth = 1.0f
	};

	// MULTISAMPLING
	vk::PipelineMultisampleStateCreateInfo multisampleInfo {
		.rasterizationSamples = vk::SampleCountFlagBits::e1,
		.sampleShadingEnable = vk::False,
		.minSampleShading = 1.0f,
		.pSampleMask = nullptr,
		.alphaToCoverageEnable = vk::False,
		.alphaToOneEnable = vk::False
	};

	// COLOR BLENDING
	vk::PipelineColorBlendAttachmentState colorBlendAttachment {
		.blendEnable = vk::False,
		.srcColorBlendFactor = vk::BlendFactor::eOne,
		.dstColorBlendFactor = vk::BlendFactor::eZero,
		.colorBlendOp = vk::BlendOp::eAdd,
		.srcAlphaBlendFactor = vk::BlendFactor::eOne,
		.dstAlphaBlendFactor = vk::BlendFactor::eZero,
		.alphaBlendOp = vk::BlendOp::eAdd,
		.colorWriteMask = (
			vk::ColorComponentFlagBits::eR |
			vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eB |
			vk::ColorComponentFlagBits::eA
		)
	};
	std::array<float, 4> blendConstants { 0.0f, 0.0f, 0.0f, 0.0f };
	vk::PipelineColorBlendStateCreateInfo colorBlendInfo {
		.logicOpEnable = vk::False,
		.logicOp = vk::LogicOp::eCopy,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachment,
		.blendConstants = blendConstants
	};

	// DEPTH / STENCIL
	vk::PipelineDepthStencilStateCreateInfo depthStencilInfo {
		.depthTestEnable = vk::True,
		.depthWriteEnable = vk::True,
		.depthCompareOp = vk::CompareOp::eLess,
		.depthBoundsTestEnable = vk::False,
		.stencilTestEnable = vk::False,
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f
	};

	// PIPELINE
	vk::GraphicsPipelineCreateInfo pipelineInfo {
		.stageCount = 2,
		.pStages = shaderStages,

		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssemblyInfo,
		.pViewportState = &viewportInfo,
		.pRasterizationState = &rasterInfo,
		.pMultisampleState = &multisampleInfo,
		.pDepthStencilState = &depthStencilInfo,
		.pColorBlendState = &colorBlendInfo,
		.pDynamicState = &dynamicStateInfo,

		.layout = *_pipelineLayout,
		.renderPass = *_renderPass,
		.subpass = 0,

		.basePipelineHandle = {},
		.basePipelineIndex = -1
	};

	return vk::raii::Pipeline(_graphDevice->logicalDevice(), nullptr, pipelineInfo);
}

vk::raii::ImageView ImageResource::createView() const {
	vk::ImageViewCreateInfo info {
		.image = _image,
		.viewType = vk::ImageViewType::e2D,
		.format = _info.format,
		.subresourceRange {
			.aspectMask = _aspectFlags,
			.baseMipLevel = 0,
			.levelCount = _info.mipLevels,
			.baseArrayLayer = 0,
			.layerCount = _info.arrayLayers
		}
	};

	return vk::raii::ImageView(_graphDevice->logicalDevice(), info);
}

vk::SurfaceFormatKHR WindowResources::chooseSwapFormat(const std::vector<vk::SurfaceFormatKHR>& availFormats) {
	for (const auto& availFormat : availFormats) {
		if (availFormat.format == vk::Format::eB8G8R8A8Srgb &&
		    availFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			return availFormat;
		}
	}

	std::cerr << "preferred swap surface format unavailable, choosing alternative..." << std::endl;

	return availFormats[0];
}

vk::PresentModeKHR WindowResources::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availPresentModes) {
	for (const auto& availPresentMode : availPresentModes) {
		if (availPresentMode == vk::PresentModeKHR::eMailbox) {
			return availPresentMode;
		}
	}

	std::cerr << "preferred swap present mode unavailable, choosing alternative..." << std::endl;

	return vk::PresentModeKHR::eFifo;
}

vk::Extent2D WindowResources::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}

	// Maxed out dimension means we'll need to determine what to use
	// ourselves.
	std::cerr << "window manager does not specify extent..." << std::endl;
	auto windowSize = _graphDevice->window().size();
	vk::Extent2D actualExtent { windowSize.width, windowSize.height };

	actualExtent.width = std::clamp(
		actualExtent.width,
		capabilities.minImageExtent.width,
		capabilities.maxImageExtent.width
	);
	actualExtent.height = std::clamp(
		actualExtent.height,
		capabilities.minImageExtent.height,
		capabilities.maxImageExtent.height
	);

	return actualExtent;
}

vk::raii::SwapchainKHR WindowResources::createSwapchain() const {
	uint32_t imageCount = _swapchainSupport.capabilities.minImageCount + 1;

	if (_swapchainSupport.capabilities.maxImageCount > 0 && imageCount > _swapchainSupport.capabilities.maxImageCount) {
		imageCount = _swapchainSupport.capabilities.maxImageCount;
	}

	vk::SwapchainCreateInfoKHR createInfo {
		.surface = _graphDevice->surface(),
		.minImageCount = imageCount,
		.imageFormat = _swapchainImageFormat.format,
		.imageColorSpace = _swapchainImageFormat.colorSpace,
		.imageExtent = _swapchainExtent,
		.imageArrayLayers = 1,
		.imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
		.preTransform = _swapchainSupport.capabilities.currentTransform,
		.presentMode = _swapchainPresentMode,
		.clipped = vk::True
	};

	if (_graphDevice->queueFamiliesAreSame()) {
		createInfo.imageSharingMode = vk::SharingMode::eExclusive;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	} else {
		auto families = _graphDevice->queueFamiliesAsArray();
		createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		createInfo.queueFamilyIndexCount = static_cast<uint32_t>(families.size());
		createInfo.pQueueFamilyIndices = families.data();
	}

	return vk::raii::SwapchainKHR(_graphDevice->logicalDevice(), createInfo);
}

vk::raii::ImageView WindowResources::createSwapImageView(const vk::Image& image) const {
	vk::ImageViewCreateInfo info {
		.image = image,
		.viewType = vk::ImageViewType::e2D,
		.format = _swapchainImageFormat.format,
		.subresourceRange = {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};

	return vk::raii::ImageView(_graphDevice->logicalDevice(), info);
}

ImageResource WindowResources::createDepthResource() const {
	vk::ImageTiling tiling = vk::ImageTiling::eOptimal;
	vk::Format format = _graphDevice->findSupportedFormat(
		{vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
		tiling,
		vk::FormatFeatureFlagBits::eDepthStencilAttachment
	);

	vk::ImageCreateInfo info {
		.imageType = vk::ImageType::e2D,
		.format = format,
		.extent = {
			.width = _swapchainExtent.width,
			.height = _swapchainExtent.height,
			.depth = 1
		},
		.mipLevels = 1,
		.arrayLayers = 1,
		.tiling = tiling,
		.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
		.sharingMode = vk::SharingMode::eExclusive,
		.initialLayout = vk::ImageLayout::eUndefined
	};

	return ImageResource(*_graphDevice, info, vk::ImageAspectFlagBits::eDepth);
}

std::vector<vk::raii::Framebuffer> WindowResources::createFramebuffers(const vk::raii::RenderPass& renderPass) {
	std::vector<vk::raii::Framebuffer> framebuffers;

	for (size_t i = 0; i < _swapchainImages.size(); ++i) {
		std::array<vk::ImageView, 2> attachments = {
			*_swapchainImageViews[i],
			*_depthResource.view()
		};
		vk::FramebufferCreateInfo info {
			.renderPass = *renderPass,
			.attachmentCount = static_cast<uint32_t>(attachments.size()),
			.pAttachments = attachments.data(),
			.width = _swapchainExtent.width,
			.height = _swapchainExtent.height,
			.layers = 1
		};
		framebuffers.push_back(vk::raii::Framebuffer(
			_graphDevice->logicalDevice(), info
		));
	}

	return framebuffers;
}

vk::raii::CommandBuffer PerFrameResources::createCommandBuffer() {
	vk::CommandBufferAllocateInfo allocInfo {
		.commandPool = *(_graphDevice->commandPool()),
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = 1
	};

	auto buf = std::move(_graphDevice->logicalDevice().allocateCommandBuffers(allocInfo)[0]);
	return buf;
}

void PerFrameResources::updateCamMatrices(const Camera& camera, unsigned int width, unsigned int height) {
	CamMatrices mats {};
	mats.view = camera.viewMatrix();
	mats.proj = camera.projectionMatrix(width, height);
	_camMats.copyIn(mats);
}

BoundBuffer Model::createVertexBuffer(const std::vector<Vertex>& vertices) {
	vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	UnsafeMappedBuffer stagingBuffer {
		*_graphDevice,
		bufferSize,
		vk::BufferUsageFlagBits::eTransferSrc
	};

	BoundBuffer destBuffer {
		*_graphDevice,
		{
			.size = bufferSize,
			.usage = (
				vk::BufferUsageFlagBits::eTransferDst |
				vk::BufferUsageFlagBits::eVertexBuffer
			),
			.sharingMode = vk::SharingMode::eExclusive
		}
	};

	stagingBuffer.unsafeCopyIn(vertices.data(), bufferSize);
	stagingBuffer.vkCopyTo(destBuffer);

	return destBuffer;
}

BoundBuffer Model::createIndexBuffer(const std::vector<uint16_t>& indices) {
	vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	UnsafeMappedBuffer stagingBuffer {
		*_graphDevice,
		bufferSize,
		vk::BufferUsageFlagBits::eTransferSrc
	};

	BoundBuffer destBuffer {
		*_graphDevice,
		{
			.size = bufferSize,
			.usage = (
				vk::BufferUsageFlagBits::eTransferDst |
				vk::BufferUsageFlagBits::eIndexBuffer
			),
			.sharingMode = vk::SharingMode::eExclusive
		}
	};

	stagingBuffer.unsafeCopyIn(indices.data(), bufferSize);
	stagingBuffer.vkCopyTo(destBuffer);

	return destBuffer;
}

void Model::record(RenderContext& ctx) const {
	auto& commandBuffer = ctx.frameResources().commandBuffer();

	vk::Buffer vertexBuffers[] = {*_vertexBuffer.buffer()};
	vk::DeviceSize offsets[] = { 0 };
	commandBuffer.bindVertexBuffers(0, vertexBuffers, offsets);
	commandBuffer.bindIndexBuffer(_indexBuffer.buffer(), 0, vk::IndexType::eUint16);
	commandBuffer.drawIndexed(static_cast<uint32_t>(_indexCount), 1, 0, 0, 0);
}

vk::raii::RenderPass Renderer::createRenderPass() {
	vk::AttachmentDescription colorAttachment {
		.format = _windowResources->swapchainFormat(),
		.samples = vk::SampleCountFlagBits::e1,
		.loadOp = vk::AttachmentLoadOp::eClear,
		.storeOp = vk::AttachmentStoreOp::eStore,
		.stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
		.stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
		.initialLayout = vk::ImageLayout::eUndefined,
		.finalLayout = vk::ImageLayout::ePresentSrcKHR
	};
	vk::AttachmentReference colorAttachmentRef {
		.attachment = 0,
		.layout = vk::ImageLayout::eColorAttachmentOptimal
	};

	vk::AttachmentDescription depthAttachment {
		.format = _windowResources->depthResource().info().format,
		.samples = vk::SampleCountFlagBits::e1,
		.loadOp = vk::AttachmentLoadOp::eClear,
		.storeOp = vk::AttachmentStoreOp::eDontCare,
		.stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
		.stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
		.initialLayout = vk::ImageLayout::eUndefined,
		.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal
	};
	vk::AttachmentReference depthAttachmentReference {
		.attachment = 1,
		.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal
	};

	vk::SubpassDescription subpass {
		.pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		.pDepthStencilAttachment = &depthAttachmentReference
	};

	vk::SubpassDependency subpassDependency {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = (
			vk::PipelineStageFlagBits::eColorAttachmentOutput |
			vk::PipelineStageFlagBits::eLateFragmentTests
		),
		.dstStageMask = (
			vk::PipelineStageFlagBits::eColorAttachmentOutput |
			vk::PipelineStageFlagBits::eEarlyFragmentTests
		),
		.srcAccessMask = (
			vk::AccessFlagBits::eColorAttachmentWrite |
			vk::AccessFlagBits::eDepthStencilAttachmentWrite
		),
		.dstAccessMask = (
			vk::AccessFlagBits::eColorAttachmentWrite |
			vk::AccessFlagBits::eDepthStencilAttachmentWrite
		)
	};

	std::array<vk::AttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
	vk::RenderPassCreateInfo renderPassInfo {
		.attachmentCount = static_cast<uint32_t>(attachments.size()),
		.pAttachments = attachments.data(),
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &subpassDependency
	};

	return vk::raii::RenderPass(_graphDevice->logicalDevice(), renderPassInfo);
}

DescriptorSetFactory Renderer::createDescriptorSetFactory() {
	std::vector<vk::DescriptorSetLayoutBinding> layoutBindings {
		{
			.binding = 0,
			.descriptorType = vk::DescriptorType::eUniformBuffer,
			.descriptorCount = 1,
			.stageFlags = vk::ShaderStageFlagBits::eVertex
		}
	};

	return DescriptorSetFactory(*_graphDevice, std::move(layoutBindings));
}

vk::raii::PipelineLayout Renderer::createPipelineLayout() {
	std::array<vk::DescriptorSetLayout, 1> descriptorSetLayouts {
		*_descriptorSetFactory.descriptorSetLayout()
	};

	vk::PushConstantRange pcRange {
		.stageFlags = vk::ShaderStageFlagBits::eVertex,
		.offset = 0,
		.size = sizeof(glm::mat4)
	};

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
		.setLayoutCount = 1,
		.pSetLayouts = descriptorSetLayouts.data(),
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &pcRange
	};
	return vk::raii::PipelineLayout(_graphDevice->logicalDevice(), pipelineLayoutInfo);
}

vk::raii::Pipeline Renderer::createLinePipeline() {
	return PipelineBuilder(*_graphDevice, _pipelineLayout, _renderPass)
		.withVertexShader("vert.spv")
		.withFragmentShader("frag.spv")
		.withTopology(vk::PrimitiveTopology::eLineList)
		.build();
}

vk::raii::Pipeline Renderer::createTrianglePipeline() {
	return PipelineBuilder(*_graphDevice, _pipelineLayout, _renderPass)
		.withVertexShader("vert.spv")
		.withFragmentShader("frag.spv")
		.withTopology(vk::PrimitiveTopology::eTriangleList)
		.build();
}

std::unordered_map<RenderMode, vk::raii::Pipeline> Renderer::createPipelines() {
	std::unordered_map<RenderMode, vk::raii::Pipeline> out;

	out.emplace(RenderMode::line, createLinePipeline());
	out.emplace(RenderMode::triangle, createTrianglePipeline());

	return out;
}

std::vector<PerFrameResources> Renderer::createPerFrameResources() {
	std::vector<PerFrameResources> vec;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vec.emplace_back(*_graphDevice, _descriptorSetFactory);
	}

	return std::move(vec);
}

void Renderer::recreateWindowResources() {
	_graphDevice->window().pauseWhileMinimized();
	_graphDevice->logicalDevice().waitIdle();

	_windowResources.reset(new WindowResources(*_graphDevice));
	_framebuffers.clear();
	_framebuffers = _windowResources->createFramebuffers(_renderPass);
}

void Camera::modAngles() {
	auto fullCircle = 2.0f * glm::pi<float>();

	if (angles.x > fullCircle || angles.x < 0) {
		angles.x = glm::mod(angles.x, fullCircle);
	}

	if (angles.y > fullCircle || angles.y < 0) {
		angles.y = glm::mod(angles.y, fullCircle);
	}
}

void Camera::updateVectors() {
	glm::mat4 hrot = glm::rotate(glm::mat4(1.0f), angles.x, {0.0f, 0.0f, 1.0f});
	_right = glm::vec3(hrot * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

	glm::mat4 vrot = glm::rotate(glm::mat4(1.0f), angles.y, _right);
	_forward = glm::vec3(vrot * hrot * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
	_up = glm::vec3(vrot * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
}

void Camera::update() {
	modAngles();
	updateVectors();

	// In fixedLook mode, position is automatically updated based on angles
	if (mode == CameraMode::fixedLook) {
		position = lookPosition - glm::distance(lookPosition, position)*_forward;
	}
}

void Camera::forward(const glm::vec3& fwd) {
	angles.x = glm::acos(fwd.y / glm::length(glm::vec2(fwd.x, fwd.y)));
	if (fwd.x > 0.0f) angles.x = 2.0f*glm::pi<float>() - angles.x;

	glm::mat4 hrot = glm::rotate(glm::mat4(1.0f), angles.x, {0.0f, 0.0f, 1.0f});
	glm::vec3 right = glm::vec3(hrot * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

	angles.y = glm::orientedAngle(
		glm::normalize(glm::vec3(fwd.x, fwd.y, 0.0f)),
		glm::normalize(fwd),
		right
	);
}

void Camera::lookAt(const glm::vec3& pos) {
	forward(pos - position);
}

glm::mat4 Camera::viewMatrix() const {
	if (mode == CameraMode::forward) {
		return glm::lookAt(position, position + _forward, _up);
	} else {
		return glm::lookAt(position, lookPosition, _up);
	}
}

glm::mat4 Camera::projectionMatrix(unsigned int width, unsigned int height) const {
	float fheight = static_cast<float>(height);
	float fwidth = static_cast<float>(width);
	float aspect = fwidth / fheight;

	glm::mat4 perspective = glm::perspective(
		glm::radians(45.0f),
		aspect,
		0.1f,
		100.0f
	);

	// compensate for Vulkan inverted Y clip coord
	perspective[1][1] *= -1;

	// In forward mode, ignore orthographic setting, since it doesn't
	// make sense.
	if (mode == CameraMode::forward) {
		return std::move(perspective);
	}

	// Scale the orthographic prism's horizontal and vertical sizes
	// relative to distance to lookAt point, sort of faking a perspective
	// divide. This allows moving the camera towards and away from the
	// center to zoom in and out, as it does in pure perspective
	// projection, making the transition between perspective and
	// orthographic much smoother.
	float orthoScale = glm::distance(position, lookPosition) / 2.0f;
	glm::mat4 ortho = glm::ortho(
		-aspect * orthoScale, aspect * orthoScale,
		-1.0f * orthoScale, 1.0f * orthoScale,
		-100.0f,
		100.0f
	);
	ortho[1][1] *= -1;

	// glm doesn't support mixing matrices, so have to do it myself here
	glm::mat4 mixed = ortho * (1.0f - projectionMix) + perspective * projectionMix;

	return std::move(mixed);
}

RenderContext& Renderer::beginFrame(const Camera& camera) {
	auto& device = _graphDevice->logicalDevice();
	auto& frameResources = _perFrameResources[_currentFrame];

	device.waitForFences(*frameResources.inFlightFence(), vk::True, UINT64_MAX);

	auto [result, imageIndex] = _windowResources->swapchain().acquireNextImage(
		UINT64_MAX, frameResources.imageAvailableSemaphore()
	);

	if (result == vk::Result::eErrorOutOfDateKHR) {
		std::cerr << "[RENDERER] beginFrame: Got eErrorOutOfDateKHR, resizing..." << std::endl;
		recreateWindowResources();
		return _renderContext;
	}

	const auto& extent = _windowResources->swapchainExtent();
	frameResources.updateCamMatrices(camera, extent.width, extent.height);

	// TODO: handle window resize
	
	device.resetFences(*frameResources.inFlightFence());

	auto& cmdBuffer = frameResources.commandBuffer();
	cmdBuffer.reset();
	cmdBuffer.begin({});

	std::array<vk::ClearValue, 2> clearValues {};
	clearValues[0].color.setFloat32({0.0f, 0.0f, 0.0f, 1.0f});
	clearValues[1].depthStencil = {1.0f, 0};

	vk::RenderPassBeginInfo beginInfo {
		.renderPass = *_renderPass,
		.framebuffer = *currentFramebuffer(),
		.renderArea = {
			.offset = {0, 0},
			.extent = extent
		},
		.clearValueCount = static_cast<uint32_t>(clearValues.size()),
		.pClearValues = clearValues.data()
	};

	cmdBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eInline);

	// per frame descriptor set referring to camera view and proj mats
	cmdBuffer.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics,
		_pipelineLayout,
		0,
		*currentFrameResources().descriptorSet(),
		nullptr
	);

	vk::Viewport viewport {
		.x = 0.0f,
		.y = 0.0f,
		.width = static_cast<float>(extent.width),
		.height = static_cast<float>(extent.height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};
	cmdBuffer.setViewport(0, viewport);

	vk::Rect2D scissor {
		.offset = {0, 0},
		.extent = extent
	};
	cmdBuffer.setScissor(0, scissor);
	_currentSwapchainImage = imageIndex;

	updateRenderContext();

	_inFrame = true;

	return _renderContext;
}

void Renderer::setMode(RenderMode mode) {
	auto& cmdBuffer = _renderContext.frameResources().commandBuffer();
	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipelines.at(mode));
}

void Renderer::endFrame() {
	auto& frameResources = currentFrameResources();
	auto& cmdBuffer = frameResources.commandBuffer();
	cmdBuffer.endRenderPass();
	cmdBuffer.end();

	_inFrame = false;

	// Submit work
	vk::Semaphore waitSemaphores[] { *frameResources.imageAvailableSemaphore() };
	vk::PipelineStageFlags waitStages[] { vk::PipelineStageFlagBits::eColorAttachmentOutput };
	vk::Semaphore signalSemaphores[] { *frameResources.renderFinishedSemaphore() };
	vk::SubmitInfo submitInfo {
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = waitSemaphores,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &(*frameResources.commandBuffer()),
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signalSemaphores
	};
	_graphDevice->graphicsQueue().submit(submitInfo, frameResources.inFlightFence());

	// Present
	vk::SwapchainKHR swapchains[] { *_windowResources->swapchain() };
	vk::PresentInfoKHR presentInfo {
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signalSemaphores,
		.swapchainCount = 1,
		.pSwapchains = swapchains,
		.pImageIndices = &_currentSwapchainImage
	};
	vk::Result result = _graphDevice->presentQueue().presentKHR(presentInfo);

	bool needWindowResize = (
		result == vk::Result::eErrorOutOfDateKHR ||
		result == vk::Result::eSuboptimalKHR ||
		_graphDevice->window().wasResized()
	);

	if (needWindowResize) {
		std::cerr << "[RENDERER] endFrame: window resized" << std::endl;
		_graphDevice->window().clearResized();
		recreateWindowResources();
	}

	_currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

/*
template<Renderable T>
void Renderer::record(const T& obj) {
	obj.record(currentFrameResources());
}
*/

void Renderer::updateRenderContext() {
	_renderContext = RenderContext(
		*_graphDevice,
		currentFrameResources(),
		_pipelineLayout,
		_currentFrame
	);
}

glm::mat4 Transform::matrix() const {
	glm::mat4 m { 1.0f };
	m = glm::translate(m, translation);
	m = glm::scale(m, scale);
	return m * rotation;
}

void RenderObject::record(RenderContext& ctx) {
	auto& cmdBuffer = ctx.frameResources().commandBuffer();

	glm::mat4 modelMat = transform.matrix();
	cmdBuffer.pushConstants<glm::mat4>(
		*ctx.pipelineLayout(),
		vk::ShaderStageFlagBits::eVertex,
		0,
		modelMat
	);
	_model.record(ctx);
}
}
