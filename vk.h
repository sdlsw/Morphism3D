#pragma once
#include "window.h"

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

namespace g3d {
constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

template<typename T>
concept MemoryBindable = requires(T obj, vk::raii::DeviceMemory m, int n) {
	{ obj.getMemoryRequirements() } -> std::convertible_to<vk::MemoryRequirements>;
	{ obj.bindMemory(m, n) };
};

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() const;
};

struct SwapChainSupportDetails {
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> presentModes;
};

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;

	static vk::VertexInputBindingDescription getBindingDescription() {
		vk::VertexInputBindingDescription d {
			.binding = 0,
			.stride = sizeof(Vertex),
			.inputRate = vk::VertexInputRate::eVertex
		};
		return d;
	}

	static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions() {
		std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions {};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}
};

struct CamMatrices {
	glm::mat4 view;
	glm::mat4 proj;
};

// Class wrapping the top level Vulkan objects to be used by the application.
class VkTop {
private:
	vk::ApplicationInfo _appInfo;
	vk::raii::Context _context;
	vk::raii::Instance _instance;
	vk::raii::PhysicalDevices _physicalDevices;

	vk::raii::Instance createInstance();
public:
	VkTop() = delete;
	VkTop(VkTop&&) = default;
	VkTop(const vk::ApplicationInfo& appInfo);
	const vk::ApplicationInfo& appInfo() const { return _appInfo; }
	vk::raii::Context& context() { return _context; }
	vk::raii::Instance& instance() { return _instance; }
	vk::raii::PhysicalDevices& physicalDevices() { return _physicalDevices; }
};

// One time use command buffer that automatically submits to a queue when it
// goes out of scope. Movable, but not copyable.
class OneTimeCommandBuffer {
private:
	vk::raii::Device* _device;
	vk::raii::Queue* _submitQueue;
	vk::raii::CommandBuffer _buffer;
	bool _shouldSubmit = false;

	vk::raii::CommandBuffer createCommandBuffer(vk::raii::CommandPool& pool);
public:
	OneTimeCommandBuffer()
	: _device {}, _submitQueue {}, _buffer { nullptr }, _shouldSubmit { false } {}

	OneTimeCommandBuffer(OneTimeCommandBuffer&& other)
	: _device { other._device },
	  _submitQueue { other._submitQueue },
	  _buffer { std::move(other._buffer) }
	{
		_shouldSubmit = false;
	}

	OneTimeCommandBuffer(
		vk::raii::Device& device,
		vk::raii::CommandPool& pool,
		vk::raii::Queue& submitQueue
	)
	: _device { &device },
	  _submitQueue { &submitQueue },
	  _buffer { createCommandBuffer(pool) },
	  _shouldSubmit { true }
	{
		_buffer.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
	}
	~OneTimeCommandBuffer();

	auto& buffer() { return _buffer; }
};

// Class wrapping a Vulkan logical device and the various top level objects
// required to construct it. Also holds references to the queues.
class GraphDevice {
private:
	VkTop* _vkTop;
	Window* _window;
	vk::raii::SurfaceKHR _surface; // NOTE: Allocated by GLFW, but freed by vulkan-hpp
	vk::raii::PhysicalDevice* _physicalDevice;
	QueueFamilyIndices _queueFamilyIndices;
	vk::raii::Device _device;
	vk::raii::Queue _graphicsQueue;
	vk::raii::Queue _presentQueue;
	vk::raii::CommandPool _commandPool;

	// Private init functions for constructor
	vk::raii::Device createLogicalDevice();
	vk::raii::CommandPool createCommandPool();

	// Query functions for determining physical device to use.
	QueueFamilyIndices queryQueueFamilies(const vk::raii::PhysicalDevice& device) const;
	SwapChainSupportDetails _querySwapChainSupport(const vk::raii::PhysicalDevice& device) const;
	bool pDevSupported(const vk::raii::PhysicalDevice& device) const;
	vk::raii::PhysicalDevice& pickPhysicalDevice(vk::raii::PhysicalDevices& devices);
public:
	GraphDevice() = delete;
	GraphDevice(GraphDevice&&) = default;

	GraphDevice(VkTop& vkTop, Window& window)
	: _vkTop { &vkTop },
	  _window { &window },
	  _surface { window.createSurface(vkTop.instance()) },
	  _physicalDevice { &pickPhysicalDevice(vkTop.physicalDevices()) },
	  _queueFamilyIndices { queryQueueFamilies(*_physicalDevice) },
	  _device { createLogicalDevice() },
	  _graphicsQueue { _device.getQueue(_queueFamilyIndices.graphicsFamily.value(), 0) },
	  _presentQueue { _device.getQueue(_queueFamilyIndices.presentFamily.value(), 0) },
	  _commandPool { createCommandPool() }
	{}

	auto& vkTop() { return *_vkTop; }
	auto& window() { return *_window; }
	auto& surface() { return _surface; }
	auto& logicalDevice() { return _device; }
	auto& physicalDevice() { return *_physicalDevice; }
	auto& graphicsQueue() { return _graphicsQueue; }
	auto& presentQueue() { return _presentQueue; }
	auto& commandPool() { return _commandPool; }

	auto querySwapChainSupport() const { return _querySwapChainSupport(*_physicalDevice); }

	bool queueFamiliesAreSame() const {
		return _queueFamilyIndices.graphicsFamily == _queueFamilyIndices.presentFamily;
	}
	const QueueFamilyIndices& queueFamilies() const { return _queueFamilyIndices; }
	std::array<uint32_t, 2> queueFamiliesAsArray() const {
		std::array<uint32_t, 2> a {
			_queueFamilyIndices.graphicsFamily.value(),
			_queueFamilyIndices.presentFamily.value()
		};
		return a;
	}

	vk::Format findSupportedFormat(
		const std::vector<vk::Format>& candidates,
		vk::ImageTiling tiling,
		vk::FormatFeatureFlags features
	);
	uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
	vk::raii::DeviceMemory allocateRequiredMemory(
		const vk::MemoryRequirements& requirements,
		const vk::MemoryPropertyFlags& properties
	);

	// Create and bind a new memory allocation for an individual object.
	// This is very much not the recommended way to do this in Vulkan (the
	// preferred way is to allocate a large chunk of device memory and then
	// manage it yourself), but this program isn't going to have many
	// resources to allocate so it SHOULD be okay. TODO come back to this
	template<MemoryBindable T>
	vk::raii::DeviceMemory bindNewMemory(
		T& obj,
		vk::MemoryPropertyFlags properties = vk::MemoryPropertyFlagBits::eDeviceLocal
	);

	vk::raii::ShaderModule loadShader(const std::string& filename);
	OneTimeCommandBuffer beginOneTimeCommands();
};

class BoundBuffer {
protected:
	GraphDevice* _graphDevice;
	vk::BufferCreateInfo _info;
	vk::raii::Buffer _buffer;
	vk::raii::DeviceMemory _memory;
public:
	BoundBuffer() = delete;
	BoundBuffer(BoundBuffer&&) = default;
	BoundBuffer(
		GraphDevice& graphDevice,
		const vk::BufferCreateInfo& info,
		vk::MemoryPropertyFlags properties = vk::MemoryPropertyFlagBits::eDeviceLocal
	)
	: _graphDevice { &graphDevice },
	  _info { info },
	  _buffer { _graphDevice->logicalDevice(), info },
	  _memory { _graphDevice->bindNewMemory(_buffer, properties) }
	{}

	operator vk::raii::Buffer&() { return _buffer; }
	auto& buffer() { return _buffer; }
	const auto& buffer() const { return _buffer; }
	const auto& info() { return _info; }
	vk::DescriptorBufferInfo descriptorInfo() const {
		vk::DescriptorBufferInfo info { .buffer = *_buffer, .offset = 0, .range = _info.size };
		return info;
	}

	void vkCopyTo(BoundBuffer& other);
};

class UnsafeMappedBuffer : public BoundBuffer {
private:
	static inline const vk::MemoryPropertyFlags MEMORY_PROPERTIES = (
		vk::MemoryPropertyFlagBits::eHostVisible |
		vk::MemoryPropertyFlagBits::eHostCoherent
	);

protected:
	void* _ptr;

public:
	UnsafeMappedBuffer() = delete;
	UnsafeMappedBuffer(UnsafeMappedBuffer&&) = default;
	UnsafeMappedBuffer(
		GraphDevice& graphDevice,
		vk::DeviceSize size,
		vk::BufferUsageFlags usage,
		vk::SharingMode sharingMode = vk::SharingMode::eExclusive
	)
	: BoundBuffer(
		graphDevice,
		{ .size = size, .usage = usage, .sharingMode = sharingMode },
		MEMORY_PROPERTIES
	  ),
	  _ptr { _memory.mapMemory(0, size, {}) }
	{}

	auto unsafePtr() { return _ptr; }
	void unsafeCopyIn(const void* data, size_t size);
};

// Same as UnsafeMappedBuffer, but tied to a specific datatype. Unfortunately
// can't be used for dynamically sized things.
template<typename T>
class MappedBuffer : public UnsafeMappedBuffer {
public:
	MappedBuffer() = delete;
	MappedBuffer(MappedBuffer&&) = default;
	MappedBuffer(
		GraphDevice& graphDevice,
		vk::BufferUsageFlags usage,
		vk::SharingMode sharingMode = vk::SharingMode::eExclusive
	)
	: UnsafeMappedBuffer(graphDevice, sizeof(T), usage, sharingMode)
	{}

	operator T*() { return reinterpret_cast<T*>(_ptr); }
	auto ptr() { return reinterpret_cast<T*>(_ptr); }
	void copyIn(const T& obj);
};

class DescriptorSetFactory {
private:
	const uint32_t MAX_DESCRIPTOR_SETS = 100;

	GraphDevice* _graphDevice;
	std::vector<vk::DescriptorSetLayoutBinding> _layoutBindings;
	vk::raii::DescriptorSetLayout _descriptorSetLayout;
	vk::raii::DescriptorPool _descriptorPool;
	uint32_t _allocatedDescriptors = 0;

	vk::raii::DescriptorSetLayout createDescriptorSetLayout();
	vk::raii::DescriptorPool createDescriptorPool();
public:
	DescriptorSetFactory() = delete;
	DescriptorSetFactory(DescriptorSetFactory&&) = default;
	DescriptorSetFactory(
		GraphDevice& graphDevice,
		const std::vector<vk::DescriptorSetLayoutBinding>& layoutBindings
	)
	: _graphDevice { &graphDevice },
	  _layoutBindings { layoutBindings },
	  _descriptorSetLayout { createDescriptorSetLayout() },
	  _descriptorPool { createDescriptorPool() }
	{}
	DescriptorSetFactory(
		GraphDevice& graphDevice,
		std::vector<vk::DescriptorSetLayoutBinding>&& layoutBindings
	)
	: _graphDevice { &graphDevice },
	  _layoutBindings { layoutBindings},
	  _descriptorSetLayout { createDescriptorSetLayout() },
	  _descriptorPool { createDescriptorPool() }
	{}

	auto& descriptorSetLayout() { return _descriptorSetLayout; }
	vk::raii::DescriptorSet makeDescriptorSet(vk::ArrayProxy<BoundBuffer> buffers);
};

// Class encapsulating an Image and its view. The view covers the entire image,
// and some accompanying memory will be allocated and bound to the image.
class ImageResource {
private:
	GraphDevice* _graphDevice;
	vk::ImageCreateInfo _info;
	vk::ImageAspectFlags _aspectFlags;
	vk::raii::Image _image;
	vk::raii::DeviceMemory _memory;
	vk::raii::ImageView _view;

	vk::raii::ImageView createView() const;
public:
	ImageResource() = delete;
	ImageResource(ImageResource&&) = default;

	ImageResource(GraphDevice& device, const vk::ImageCreateInfo& info, vk::ImageAspectFlags aspectFlags)
	: _graphDevice { &device },
	  _aspectFlags { aspectFlags },
	  _info { info },
	  _image { _graphDevice->logicalDevice(), _info },
	  _memory { _graphDevice->bindNewMemory(_image) },
	  _view { createView() }
	{}

	const vk::ImageCreateInfo& info() const { return _info; }
	const vk::raii::ImageView& view() const { return _view; }
};

// Class encapsulating resources that must be recreated on every window resize.
class WindowResources {
private:
	GraphDevice* _graphDevice;
	const vk::raii::RenderPass* _renderPass;

	// All supported abilities of surface this swapchain is attached to
	SwapChainSupportDetails _swapchainSupport;

	// Chosen formats and settings for swapchain
	vk::SurfaceFormatKHR _swapchainImageFormat;
	vk::PresentModeKHR _swapchainPresentMode;
	vk::Extent2D _swapchainExtent;

	vk::raii::SwapchainKHR _swapchain;
	std::vector<vk::Image> _swapchainImages; // owned by swapchain, no need to free these
	std::vector<vk::raii::ImageView> _swapchainImageViews;

	ImageResource _depthResource;

	// Swap init functions
	static vk::SurfaceFormatKHR chooseSwapFormat(const std::vector<vk::SurfaceFormatKHR>& availFormats);
	static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availPresentModes);
	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const;
	vk::raii::SwapchainKHR createSwapchain() const;
	vk::raii::ImageView createSwapImageView(const vk::Image& image) const;
	std::vector<vk::raii::ImageView> createSwapImageViews() const {
		std::vector<vk::raii::ImageView> views;
		for (const auto& image : _swapchainImages) {
			views.push_back(createSwapImageView(image));
		}
		return views;
	}

	// Depth buffer is same size as window and must be recreated on resize.
	ImageResource createDepthResource() const;

public:
	WindowResources() = delete;
	WindowResources(WindowResources&&) = default;
	WindowResources(GraphDevice& graphDevice)
	: _graphDevice { &graphDevice },
	  _swapchainSupport { _graphDevice->querySwapChainSupport() },
	  _swapchainImageFormat { chooseSwapFormat(_swapchainSupport.formats) },
	  _swapchainPresentMode { chooseSwapPresentMode(_swapchainSupport.presentModes) },
	  _swapchainExtent { chooseSwapExtent(_swapchainSupport.capabilities) },
	  _swapchain { createSwapchain() },
	  _swapchainImages { _swapchain.getImages() },
	  _swapchainImageViews { createSwapImageViews() },
	  _depthResource { createDepthResource() }
	{}

	vk::Format swapchainFormat() { return _swapchainImageFormat.format; }
	const auto& swapchainExtent() const { return _swapchainExtent; }
	auto& swapchain() { return _swapchain; }
	const ImageResource& depthResource() { return _depthResource; }

	// Create framebuffer objects encompassing all of the resources for the
	// window.
	std::vector<vk::raii::Framebuffer> createFramebuffers(const vk::raii::RenderPass& renderPass);
};

enum class CameraMode : int {
	forward = 0,
	fixedLook
};

inline constexpr unsigned int CameraModeCount = 2;

class Camera {
private:
	glm::vec3 _position {0.0f, 0.0f, 0.0f};

	// In "forward" mode, this is a vector corresponding to the view
	// direction. In "fixedLook" mode, this vector is a fixed position the
	// camera is looking at. TODO maybe this is gross? 
	glm::vec3 _look {0.0f, 1.0f, 0.0f};

	// For this application, +Z is considered "up" due to 3D graphing
	// conventions.
	glm::vec3 _up;
	glm::vec3 _right;

	CameraMode _mode = CameraMode::forward;

	void recalcDirections();
public:
	Camera() { recalcDirections(); }
	Camera(
		CameraMode mode,
		const glm::vec3& position,
		const glm::vec3& look
	) : _mode{mode}, _position{position}, _look{look} {
		recalcDirections();
	}

	const glm::vec3& up() const { return _up; }
	const glm::vec3& right() const { return _right; }

	glm::mat4 viewMatrix() const;
	glm::mat4 perspectiveMatrix(unsigned int width, unsigned int height) const;

	glm::vec3 lookAt() const;
	void lookAt(const glm::vec3& lookVec);

	glm::vec3 forward() const;
	void forward(const glm::vec3& lookVec);

	CameraMode mode() const { return _mode; }
	void mode(CameraMode newMode);

	const glm::vec3& position() const { return _position; }
	void position(const glm::vec3& newPosition);

	void rotateForward(float horizontal, float vertical);
	void rotateAround(const glm::vec3& center, float horizontal, float vertical);
};

// Set of resources that must be duplicated per frame in flight.
class PerFrameResources {
private:
	GraphDevice* _graphDevice;
	MappedBuffer<CamMatrices> _camMats;
	vk::raii::DescriptorSet _descriptorSet;
	vk::raii::CommandBuffer _commandBuffer;

	// Per frame sync primitives.
	vk::raii::Semaphore _imageAvailableSemaphore;
	vk::raii::Semaphore _renderFinishedSemaphore;
	vk::raii::Fence _inFlightFence;

	vk::raii::CommandBuffer createCommandBuffer();
public:
	PerFrameResources() = delete;
	PerFrameResources(PerFrameResources&&) = default;
	PerFrameResources(
		GraphDevice& graphDevice,
		DescriptorSetFactory& descriptorSetFactory
	)
	: _graphDevice { &graphDevice },
	  _camMats { *_graphDevice, vk::BufferUsageFlagBits::eUniformBuffer },
	  _descriptorSet { descriptorSetFactory.makeDescriptorSet(_camMats) },
	  _commandBuffer { createCommandBuffer() },
	  _imageAvailableSemaphore { _graphDevice->logicalDevice().createSemaphore({}) },
	  _renderFinishedSemaphore { _graphDevice->logicalDevice().createSemaphore({}) },
	  _inFlightFence { _graphDevice->logicalDevice().createFence({ .flags = vk::FenceCreateFlagBits::eSignaled }) }
	{}

	auto& commandBuffer() { return _commandBuffer; }
	const auto& descriptorSet() { return _descriptorSet; }
	auto& imageAvailableSemaphore() { return _imageAvailableSemaphore; }
	auto& renderFinishedSemaphore() { return _renderFinishedSemaphore; }
	auto& inFlightFence() { return _inFlightFence; }
	void updateCamMatrices(const Camera& camera, unsigned int width, unsigned int height);
};

// Lightweight context object pointing to objects and information required for
// rendering. Reconstructed each frame, and only valid between
// Renderer::beginFrame() and Renderer::endFrame()
class RenderContext {
private:
	GraphDevice* _graphDevice;
	PerFrameResources* _frameResources;
	vk::raii::PipelineLayout* _pipelineLayout;
	unsigned int _currentFrame;

public:
	RenderContext() = default;
	RenderContext(
		GraphDevice& graphDevice,
		PerFrameResources& frameResources,
		vk::raii::PipelineLayout& pipelineLayout,
		unsigned int currentFrame
	)
	: _graphDevice { &graphDevice },
	  _frameResources { &frameResources },
	  _pipelineLayout { &pipelineLayout },
	  _currentFrame { currentFrame }
	{}

	auto& graphDevice() { return *_graphDevice; }
	auto& frameResources() { return *_frameResources; }
	auto& pipelineLayout() { return *_pipelineLayout; }
	auto currentFrame() { return _currentFrame; }
};

template<typename T>
concept Renderable = requires(T obj, PerFrameResources& resources) {
	{ obj.record(resources) };
};

class Renderer {
private:
	unsigned int _currentFrame = 0;
	unsigned int _currentSwapchainImage = 0;
	bool _inFrame = false;

	GraphDevice* _graphDevice;
	std::unique_ptr<WindowResources> _windowResources;
	vk::raii::RenderPass _renderPass;
	DescriptorSetFactory _descriptorSetFactory;
	vk::raii::PipelineLayout _pipelineLayout;
	vk::raii::Pipeline _graphicsPipeline;
	RenderContext _renderContext;

	// Annoying: Framebuffer objects depend upon all the attachments
	// to be used during rendering, and must be recreated on window
	// resize. So, ideal place to put them would be in PerFrameResources.
	// However, Framebuffers *also* depend on the RenderPass, which depends
	// on information in WindowResources AND must *not* be recreated on
	// window resize. So, put it here for now.
	// TODO: separate out format discovery and specification to eliminate
	// dependency
	std::vector<vk::raii::Framebuffer> _framebuffers;

	std::vector<PerFrameResources> _perFrameResources;

	vk::raii::RenderPass createRenderPass();
	vk::raii::DescriptorSetLayout createDescriptorSetLayout();
	vk::raii::PipelineLayout createPipelineLayout();
	vk::raii::Pipeline createGraphicsPipeline();
	DescriptorSetFactory createDescriptorSetFactory();
	std::vector<PerFrameResources> createPerFrameResources();

	void updateRenderContext();
	void recreateWindowResources();
public:
	Renderer() = delete;
	Renderer(Renderer&&) = default;
	Renderer(GraphDevice& graphDevice)
	: _graphDevice { &graphDevice },
	  _windowResources { std::make_unique<WindowResources>(*_graphDevice) },
	  _renderPass { createRenderPass() },
	  _descriptorSetFactory { createDescriptorSetFactory() },
	  _pipelineLayout { createPipelineLayout() },
	  _graphicsPipeline { createGraphicsPipeline() },
	  _framebuffers { _windowResources->createFramebuffers(_renderPass) },
	  _perFrameResources { createPerFrameResources() }
	{}

	auto& currentFrameResources() { return _perFrameResources[_currentFrame]; }
	auto& currentFramebuffer() { return _framebuffers[_currentFrame]; }
	auto& context() { return _renderContext; }
	auto& descriptorSetFactory() { return _descriptorSetFactory; }
	auto& renderPass() { return _renderPass; }
	uint32_t swapchainImageCount() {
		return static_cast<uint32_t>(_windowResources->swapchain().getImages().size());
	}

	RenderContext& beginFrame(const Camera& camera);
	bool inFrame() const { return _inFrame; }
	void endFrame();

	template<Renderable T>
	void record(const T& obj);
};

class Model {
private:
	GraphDevice* _graphDevice;
	BoundBuffer _vertexBuffer;
	BoundBuffer _indexBuffer;
	
	size_t _indexCount;

	BoundBuffer createVertexBuffer(const std::vector<Vertex>& vertices);
	BoundBuffer createIndexBuffer(const std::vector<uint16_t>& indices);
public:
	Model() = delete;
	Model(Model&&) = default;
	Model(
		GraphDevice& graphDevice,
		const std::vector<Vertex>& vertices,
		const std::vector<uint16_t>& indices
	)
	: _graphDevice { &graphDevice },
	  _vertexBuffer { createVertexBuffer(vertices) },
	  _indexBuffer { createIndexBuffer(indices) },
	  _indexCount { indices.size() }
	{}

	void record(RenderContext& ctx) const;
};

struct Transform {
	// For use in constructors
	static inline const glm::vec3 default_scale { 1.0f, 1.0f, 1.0f };

	glm::vec3 translation { 0.0f, 0.0f, 0.0f};
	glm::vec3 scale { 1.0f, 1.0f, 1.0f };
	glm::mat4 rotation { 1.0f };

	Transform() = default;
	Transform(const glm::vec3& t) : translation {t} {}
	Transform(const glm::vec3& t, const glm::vec3& s) : translation {t}, scale {s} {}
	Transform(const glm::vec3& t, const glm::vec3& s, const glm::mat4 r) : translation {t}, scale {s}, rotation {r} {}

	glm::mat4 matrix() const;
};

class RenderObject {
private:
	GraphDevice* _graphDevice;
	Renderer* _renderer;
	Model _model;
public:
	Transform transform;

	RenderObject() = delete;
	RenderObject(RenderObject&&) = default;
	RenderObject(
		GraphDevice& graphDevice,
		Renderer& renderer,
		Model&& model,
		const Transform& transform
	)
	: _graphDevice { &graphDevice },
	  _renderer { &renderer },
	  _model { std::move(model) },
	  transform { transform }
	{}

	void record(RenderContext& ctx);
};
}
