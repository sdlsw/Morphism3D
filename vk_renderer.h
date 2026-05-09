#pragma once
#include "global_defines.h"

#include "vk_camera.h"
#include "vk_constants.h"
#include "vk_datatypes.h"
#include "vk_device.h"
#include "vk_helper.h"

#include <glm/common.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace g3d {
// Creates a buffer accessible only to the GPU out of some vector of things.
template<typename T>
BoundBuffer makeStaticGPUBuffer(
	GraphDevice& graphDevice,
	const std::vector<T>& items,
	vk::BufferUsageFlags usage
) {
	vk::DeviceSize bufferSize = sizeof(items[0]) * items.size();

	UnsafeMappedBuffer stagingBuffer {
		graphDevice,
		bufferSize,
		vk::BufferUsageFlagBits::eTransferSrc
	};

	BoundBuffer destBuffer {
		graphDevice,
		{
			.size = bufferSize,
			.usage = (
				vk::BufferUsageFlagBits::eTransferDst |
				usage
			),
			.sharingMode = vk::SharingMode::eExclusive
		}
	};

	stagingBuffer.unsafeCopyIn(items.data(), bufferSize);
	stagingBuffer.vkCopyTo(destBuffer);

	return destBuffer;
}

struct CamMatrices {
	glm::mat4 view;
	glm::mat4 proj;
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
	  _descriptorSet { descriptorSetFactory.makeDescriptorSet(_camMats.descriptorInfo()) },
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

enum class RenderMode {
	line,
	triangle
};

class Renderer {
private:
	unsigned int _currentFrame = 0;
	unsigned int _currentSwapchainImage = 0;
	bool _inFrame = false;
	RenderMode _mode = RenderMode::line;

	GraphDevice* _graphDevice;
	std::unique_ptr<WindowResources> _windowResources;
	vk::raii::RenderPass _renderPass;
	DescriptorSetFactory _descriptorSetFactory;
	vk::raii::PipelineLayout _pipelineLayout;
	std::unordered_map<RenderMode, vk::raii::Pipeline> _pipelines;
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
	vk::raii::Pipeline createLinePipeline();
	vk::raii::Pipeline createTrianglePipeline();
	std::unordered_map<RenderMode, vk::raii::Pipeline> createPipelines();
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
	  _pipelines { createPipelines() },
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
	void setMode(RenderMode mode);
	void endFrame();
};

template<typename T>
class StaticVertexAttributes {
private:
	GraphDevice* _graphDevice;
	BoundBuffer _buffer;

	BoundBuffer createBuffer(const std::vector<T>& attrs) {
		return makeStaticGPUBuffer(
			*_graphDevice,
			attrs,
			vk::BufferUsageFlagBits::eVertexBuffer
		);
	}

public:
	StaticVertexAttributes() = delete;
	StaticVertexAttributes(StaticVertexAttributes&&) = default;
	StaticVertexAttributes(
		GraphDevice& graphDevice,
		const std::vector<T>& attrs
	) : _graphDevice { &graphDevice }, _buffer { createBuffer(attrs) } {}

	void record(RenderContext& ctx) const {
		auto& commandBuffer = ctx.frameResources().commandBuffer();

		vk::Buffer vertexBuffers[] = {*_buffer.buffer()};
		vk::DeviceSize offsets[] = { 0 };
		commandBuffer.bindVertexBuffers(T::binding, vertexBuffers, offsets);
	}
};

class StaticMesh {
private:
	GraphDevice* _graphDevice;
	StaticVertexAttributes<Position> _positions;
	BoundBuffer _indexBuffer;

	size_t _indexCount;

	BoundBuffer createIndexBuffer(const std::vector<uint16_t>& indices);
public:
	StaticMesh() = delete;
	StaticMesh(StaticMesh&&) = default;
	StaticMesh(
		GraphDevice& graphDevice,
		const std::vector<Position>& positions,
		const std::vector<uint16_t>& indices
	)
	: _graphDevice { &graphDevice },
	  _positions { graphDevice, positions },
	  _indexBuffer { makeStaticGPUBuffer(
		graphDevice,
		indices,
		vk::BufferUsageFlagBits::eIndexBuffer
	  ) },
	  _indexCount { indices.size() }
	{}

	void record(RenderContext& ctx) const;
};

class RenderObject {
private:
	GraphDevice* _graphDevice;
	Renderer* _renderer;
	StaticMesh _mesh;
	StaticVertexAttributes<Color> _colors;
public:
	Transform transform;

	RenderObject() = delete;
	RenderObject(RenderObject&&) = default;
	RenderObject(
		GraphDevice& graphDevice,
		Renderer& renderer,
		const std::vector<Position>& positions,
		const std::vector<Color>& colors,
		const std::vector<uint16_t>& indices,
		const Transform& transform
	)
	: _graphDevice { &graphDevice },
	  _renderer { &renderer },
	  _mesh { graphDevice, positions, indices },
	  _colors { graphDevice, colors },
	  transform { transform }
	{}

	void record(RenderContext& ctx);
};
}
