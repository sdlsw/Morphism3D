#pragma once
#include "global_defines.h"

#include "entity.h"
#include "vk/camera.h"
#include "vk/constants.h"
#include "vk/datatypes.h"
#include "vk/device.h"
#include "vk/helper.h"

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

struct CamData {
	glm::mat4 view;
	glm::mat4 proj;
	alignas(16) glm::vec3 pos;
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

	ImageResource _colorResource;
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

	// Depth and color buffers are same size as window and must be recreated on resize.
	ImageResource createColorResource() const;
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
	  _colorResource { createColorResource() },
	  _depthResource { createDepthResource() }
	{}

	vk::Format swapchainFormat() { return _swapchainImageFormat.format; }
	const auto& swapchainExtent() const { return _swapchainExtent; }
	auto& swapchain() { return _swapchain; }
	const ImageResource& colorResource() { return _colorResource; }
	const ImageResource& depthResource() { return _depthResource; }

	// Create framebuffer objects encompassing all of the resources for the
	// window.
	std::vector<vk::raii::Framebuffer> createFramebuffers(const vk::raii::RenderPass& renderPass);
};

// Set of resources that must be duplicated per frame in flight.
class PerFrameResources {
private:
	GraphDevice* _graphDevice;
	MappedBuffer<CamData> _camData;
	MappedBuffer<Light> _lightData;
	vk::raii::DescriptorSet _descriptorSet;
	vk::raii::CommandBuffer _commandBuffer;

	// Per frame sync primitives.
	vk::raii::Semaphore _imageAvailableSemaphore;
	vk::raii::Semaphore _renderFinishedSemaphore;
	vk::raii::Fence _inFlightFence;

	vk::raii::CommandBuffer createCommandBuffer();
	vk::raii::DescriptorSet createDescriptorSet(
		DescriptorSetFactory& descriptorSetFactory
	);
public:
	PerFrameResources() = delete;
	PerFrameResources(PerFrameResources&&) = default;
	PerFrameResources(
		GraphDevice& graphDevice,
		DescriptorSetFactory& descriptorSetFactory
	)
	: _graphDevice { &graphDevice },
	  _camData { *_graphDevice, vk::BufferUsageFlagBits::eUniformBuffer },
	  _lightData { *_graphDevice, vk::BufferUsageFlagBits::eUniformBuffer },
	  _descriptorSet { createDescriptorSet(descriptorSetFactory) },
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
	void updateCamData(const Camera& camera, unsigned int width, unsigned int height);
	void updateLightData(const Light& light);
};

enum class RenderMode {
	line,
	triangle,
	litTriangle,
	litTriangleCulled
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
	vk::raii::Pipeline createLitTrianglePipeline();
	vk::raii::Pipeline createLitTriangleCulledPipeline();
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

	auto& graphDevice() { return *_graphDevice; }
	const auto& currentFrame() { return _currentFrame; }
	auto& currentFrameResources() { return _perFrameResources[_currentFrame]; }
	auto& currentFramebuffer() { return _framebuffers[_currentFrame]; }
	auto& currentCommandBuffer() { return currentFrameResources().commandBuffer(); }
	auto& descriptorSetFactory() { return _descriptorSetFactory; }
	auto& renderPass() { return _renderPass; }
	auto& pipelineLayout() { return _pipelineLayout; }
	uint32_t swapchainImageCount() {
		return static_cast<uint32_t>(_windowResources->swapchain().getImages().size());
	}

	void beginFrame(const Camera& camera, const Light& light);
	bool inFrame() const { return _inFrame; }
	void setMode(RenderMode mode);
	void endFrame();
};

template<typename T>
class StaticVertexAttributes {
private:
	Renderer* _renderer;
	BoundBuffer _buffer;
	size_t _count;

	BoundBuffer createBuffer(const std::vector<T>& attrs) {
		return makeStaticGPUBuffer(
			_renderer->graphDevice(),
			attrs,
			vk::BufferUsageFlagBits::eVertexBuffer
		);
	}

public:
	StaticVertexAttributes() = delete;
	StaticVertexAttributes(StaticVertexAttributes&&) = default;
	StaticVertexAttributes(Renderer& renderer, const std::vector<T>& attrs)
	: _renderer { &renderer },
	  _buffer { createBuffer(attrs) },
	  _count { attrs.size() }
	{}

	auto& renderer() const { return *_renderer; }
	auto count() const { return _count; }

	void record() const {
		auto& cmd = _renderer->currentCommandBuffer();
		vk::Buffer vertexBuffers[] = {*_buffer.buffer()};
		vk::DeviceSize offsets[] = { 0 };
		cmd.bindVertexBuffers(T::binding, vertexBuffers, offsets);
	}
};

template<typename T, vk::BufferUsageFlagBits U>
class DynamicDoubleBuffer {
private:
	Renderer* _renderer;
	std::vector<size_t> _counts;
	std::vector<std::unique_ptr<UnsafeMappedBuffer>> _buffers;

	void copyDataTo(unsigned int n, const std::vector<T>& attrs) {
		auto& curBuffer = _buffers[n];
		auto& curCount = _counts[n];

		vk::DeviceSize bufferSize = sizeof(T) * attrs.size();
		if (attrs.size() > curCount) {
			curBuffer.reset(new UnsafeMappedBuffer(
				_renderer->graphDevice(),
				bufferSize,
				U
			));
		}

		curCount = attrs.size();
		curBuffer.get()->unsafeCopyIn(attrs.data(), bufferSize);
	}
public:
	DynamicDoubleBuffer() = delete;
	DynamicDoubleBuffer(DynamicDoubleBuffer&&) = default;
	DynamicDoubleBuffer(Renderer& renderer)
	: _renderer { &renderer }
	{
		for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			_counts.push_back(0);
			_buffers.push_back({});
		}
	}
	DynamicDoubleBuffer(Renderer& renderer, const std::vector<T>& attrs)
	: DynamicDoubleBuffer(renderer)
	{
		for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			copyDataTo(i, attrs);
		}
	}

	auto& renderer() const { return *_renderer; }

	auto count() const { return _counts[_renderer->currentFrame()]; }

	// Automatically copies data to the buffer corresponding to the current
	// frame.
	void copyData(const std::vector<T>& attrs) {
		copyDataTo(_renderer->currentFrame(), attrs);
	}

	void record() {
		auto& curBuffer = _buffers[_renderer->currentFrame()];
		if (!curBuffer) {
			throw std::runtime_error("Cannot record an unallocated dynamic attribute");
		}

		auto& cmd = _renderer->currentCommandBuffer();
		vk::Buffer rawBuffer = *(curBuffer.get()->buffer());
		if constexpr (U == vk::BufferUsageFlagBits::eVertexBuffer) {
			vk::Buffer vertexBuffers[] = { rawBuffer };
			vk::DeviceSize offsets[] = { 0 };
			cmd.bindVertexBuffers(T::binding, vertexBuffers, offsets);
		} else {
			cmd.bindIndexBuffer(rawBuffer, 0, vk::IndexType::eUint16);
			cmd.drawIndexed(static_cast<uint32_t>(count()), 1, 0, 0, 0);
		}
	}
};

template<typename T>
using DynamicVertexAttributes = DynamicDoubleBuffer<T, vk::BufferUsageFlagBits::eVertexBuffer>;

using DynamicIndexBuffer = DynamicDoubleBuffer<uint16_t, vk::BufferUsageFlagBits::eIndexBuffer>;

class StaticIndexBuffer {
private:
	Renderer* _renderer;
	BoundBuffer _buffer;
	size_t _count;

	BoundBuffer createBuffer(const std::vector<uint16_t>& indices);

public:
	StaticIndexBuffer() = delete;
	StaticIndexBuffer(StaticIndexBuffer&&) = default;
	StaticIndexBuffer(Renderer& renderer, const std::vector<uint16_t>& indices)
	: _renderer { &renderer },
	  _buffer { createBuffer(indices) },
	  _count { indices.size() }
	{}

	auto& renderer() const { return *_renderer; }
	auto count() const { return _count; }
	void record() const;
};

// Combined object owning both a position and index buffer. Suitable for most
// uses and a little more convenient than having them be separate.
class StaticMesh {
private:
	StaticVertexAttributes<Position> _positions;
	StaticIndexBuffer _indices;
public:
	StaticMesh() = delete;
	StaticMesh(StaticMesh&&) = default;
	StaticMesh(
		Renderer& renderer,
		const std::vector<Position>& positions,
		const std::vector<uint16_t>& indices
	)
	: _positions { renderer, positions },
	  _indices { renderer, indices }
	{}

	auto& renderer() const { return _positions.renderer(); }
	auto indexCount() const { return _indices.count(); }
	auto positionCount() const { return _positions.count(); }

	void record() const;
};

class TransformComponent : public Component {
private:
	Renderer* _renderer;

public:
	Transform transform;

	TransformComponent(Renderer& renderer) : _renderer { &renderer } {}
	TransformComponent(
		Renderer& renderer,
		const Transform& initialTransform
	)
	: _renderer { &renderer },
	  transform { initialTransform }
	{}

	void render() override;
};

template<typename T>
class StaticVertexAttributeComponent : public Component {
private:
	StaticVertexAttributes<T>* _attributes;

public:
	StaticVertexAttributeComponent(
		StaticVertexAttributes<T>& attributes
	) : _attributes { &attributes } {}

	void render() override { _attributes->record(); }
};

template<typename T>
class DynamicVertexAttributeComponent : public Component {
private:
	DynamicVertexAttributes<T>* _attributes;

public:
	DynamicVertexAttributeComponent(
		DynamicVertexAttributes<T>& attributes
	) : _attributes { &attributes } {}

	void render() override { _attributes->record(); }
};

class StaticIndexBufferComponent : public Component {
private:
	StaticIndexBuffer* _indices;

public:
	StaticIndexBufferComponent(StaticIndexBuffer& indices)
	: _indices { &indices } {}

	void render() override { _indices->record(); }
};

class DynamicIndexBufferComponent : public Component {
private:
	DynamicIndexBuffer* _indices;

public:
	DynamicIndexBufferComponent(DynamicIndexBuffer& indices)
	: _indices { &indices } {}

	void render() override { _indices->record(); }
};

class StaticMeshComponent : public Component {
private:
	StaticMesh* _mesh;

public:
	StaticMeshComponent(StaticMesh& mesh) : _mesh { &mesh } {}
	void render() override { _mesh->record(); }
};

class MaterialComponent : public Component {
private:
	Renderer* _renderer;
	Material* _material;

public:
	MaterialComponent(Renderer& renderer, Material& material)
	: _renderer { &renderer }, _material { &material } {}

	void render() override;
};

// Convenience function for setting up a render entity that never changes
// except for transform.
void populateStaticEntity(
	Renderer& renderer,
	Entity& entity,
	const Transform& transform,
	StaticMesh& mesh,
	StaticVertexAttributes<Color>& colors
);

void populateStaticEntity(
	Renderer& renderer,
	Entity& entity,
	const Transform& transform,
	StaticMesh& mesh,
	StaticVertexAttributes<Color>& colors,
	StaticVertexAttributes<Normal>& normals
);

Transform& getTransform(Entity& entity);
}
