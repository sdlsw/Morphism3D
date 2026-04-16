#include "vk.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <set>
#include <fstream>

#include <glm/gtc/matrix_transform.hpp>

static const uint32_t INITIAL_WIDTH = 800;
static const uint32_t INITIAL_HEIGHT = 600;

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
bool QueueFamilyIndices::isComplete() const {
	return graphicsFamily.has_value() && presentFamily.has_value();
}

void Window::initWindowingSystem() {
	std::cerr << "initializing GLFW..." << std::endl;
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	_glfwInitialized = true;
}

Window::Window(std::string title, uint32_t initialWidth, uint32_t initialHeight) {
	if (!_glfwInitialized) {
		initWindowingSystem();
	}

	_glfwWindow = glfwCreateWindow(
		initialWidth,
		initialHeight,
		title.c_str(),
		nullptr, nullptr
	);

	if (_glfwWindow == nullptr) {
		throw std::runtime_error("could not create GLFW window!");
	}

	glfwSetWindowUserPointer(_glfwWindow, this);
	glfwSetFramebufferSizeCallback(_glfwWindow, frameBufferResizeCallback);
	_glfwWindowCount++;

	std::cerr << "successfully created GLFW window at 0x" << this << std::endl;
}

Window::~Window() {
	if (_glfwWindow != nullptr) {
		glfwDestroyWindow(_glfwWindow);
		_glfwWindowCount--;

		if (_glfwWindowCount == 0) {
			std::cerr << "last window destroyed, terminating GLFW" << std::endl;
			glfwTerminate();
			_glfwInitialized = false;
		}
	}
}

Window* Window::getWrapperPointer(GLFWwindow* window) {
	return reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
}

void Window::frameBufferResizeCallback(GLFWwindow* window, int width, int height) {
	Window::getWrapperPointer(window)->_windowResized = true;
}

void Window::pollEvents() {
	glfwPollEvents();
}

bool Window::shouldClose() {
	return glfwWindowShouldClose(_glfwWindow);
}

WindowSize Window::size() {
	int w, h;
	glfwGetFramebufferSize(_glfwWindow, &w, &h);
	WindowSize s { static_cast<uint32_t>(w), static_cast<uint32_t>(h) };
	return s;
}

vk::raii::SurfaceKHR Window::createSurface(vk::raii::Instance& instance) {
	VkSurfaceKHR surface;
	VkResult result = glfwCreateWindowSurface(*instance, _glfwWindow, nullptr, &surface);

	if (result != VK_SUCCESS) {
		std::cerr << "glfwCreateWindowSurface returned status code " << result << std::endl;
		throw std::runtime_error("failed to create window surface!");
	}

	vk::raii::SurfaceKHR raiiSurface { instance, surface };
	return std::move(raiiSurface);
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
	mats.proj = camera.perspectiveMatrix(width, height);
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
	// FIXME FIXME Reusing descriptor set layout multiple times
	// For now OK since both descriptor sets only consist of one uniform
	// buffer each, but really need to fix this.
	std::array<vk::DescriptorSetLayout, 2> descriptorSetLayouts {
		*_descriptorSetFactory.descriptorSetLayout(),
		*_descriptorSetFactory.descriptorSetLayout()
	};

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
		.setLayoutCount = 2,
		.pSetLayouts = descriptorSetLayouts.data(),
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr
	};
	return vk::raii::PipelineLayout(_graphDevice->logicalDevice(), pipelineLayoutInfo);
}

vk::raii::Pipeline Renderer::createGraphicsPipeline() {
	// SHADERS
	vk::raii::ShaderModule vertModule = _graphDevice->loadShader("vert.spv");
	vk::raii::ShaderModule fragModule = _graphDevice->loadShader("frag.spv");

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
		.topology = vk::PrimitiveTopology::eLineList,
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

		.layout = _pipelineLayout,
		.renderPass = _renderPass,
		.subpass = 0,

		.basePipelineHandle = {},
		.basePipelineIndex = -1
	};

	return vk::raii::Pipeline(_graphDevice->logicalDevice(), nullptr, pipelineInfo);
}

std::vector<PerFrameResources> Renderer::createPerFrameResources() {
	std::vector<PerFrameResources> vec;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vec.emplace_back(*_graphDevice, _descriptorSetFactory);
	}

	return std::move(vec);
}

glm::mat4 Camera::viewMatrix() const {
	return glm::lookAt(_position, _lookAt, _up);
}

glm::mat4 Camera::perspectiveMatrix(unsigned int width, unsigned int height) const {
	glm::mat4 p = glm::perspective(
		glm::radians(45.0f),
		width / static_cast<float>(height),
		0.1f,
		100.0f
	);

	// compensate for Vulkan inverted Y clip coord
	p[1][1] *= -1;

	return std::move(p);
}

RenderContext& Renderer::beginFrame(const Camera& camera) {
	auto& device = _graphDevice->logicalDevice();
	auto& frameResources = _perFrameResources[_currentFrame];

	device.waitForFences(*frameResources.inFlightFence(), vk::True, UINT64_MAX);

	auto [result, imageIndex] = _windowResources->swapchain().acquireNextImage(
		UINT64_MAX, frameResources.imageAvailableSemaphore()
	);

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
	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _graphicsPipeline);
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
	_graphDevice->presentQueue().presentKHR(presentInfo);

	// TODO resize stuff

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

std::vector<MappedBuffer<glm::mat4>> RenderObject::createTransformBuffers() {
	std::vector<MappedBuffer<glm::mat4>> vec;
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vec.emplace_back(*_graphDevice, vk::BufferUsageFlagBits::eUniformBuffer);
	}
	return vec;
}

std::vector<vk::raii::DescriptorSet> RenderObject::createDescriptorSets() {
	auto& factory = _renderer->descriptorSetFactory();

	std::vector<vk::raii::DescriptorSet> vec;
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		// FIXME Reusing descriptor set layout
		vec.push_back(factory.makeDescriptorSet(_transformBuffers[i]));
	}
	return vec;
}

void RenderObject::update(RenderContext& ctx) {
	// very gross to have to do this for every object, probably won't
	// scale well, but is OK for now due to low object count
	_transformBuffers[ctx.currentFrame()].copyIn(_transform);
}

void RenderObject::record(RenderContext& ctx) {
	auto& cmdBuffer = ctx.frameResources().commandBuffer();

	cmdBuffer.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics,
		ctx.pipelineLayout(),
		1,
		*_descriptorSets[ctx.currentFrame()],
		nullptr
	);

	_model.record(ctx);
}
}
