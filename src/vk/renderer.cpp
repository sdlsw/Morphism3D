#include "vk/renderer.h"

#include <algorithm>
#include <iostream>
#include <limits>

namespace g3d {
std::ostream& operator<<(std::ostream& o, const glm::vec3& v) {
	o << v.x << ", " << v.y << ", " << v.z;
	return o;
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

ImageResource WindowResources::createColorResource() const {
	vk::ImageTiling tiling = vk::ImageTiling::eOptimal;

	vk::ImageCreateInfo info {
		.imageType = vk::ImageType::e2D,
		.format = _swapchainImageFormat.format,
		.extent = {
			.width = _swapchainExtent.width,
			.height = _swapchainExtent.height,
			.depth = 1
		},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = MSAA_SAMPLES,
		.tiling = vk::ImageTiling::eOptimal,
		.usage = (
			vk::ImageUsageFlagBits::eColorAttachment |
			vk::ImageUsageFlagBits::eTransientAttachment
		),
	};

	return ImageResource(*_graphDevice, info, vk::ImageAspectFlagBits::eColor);
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
		.samples = MSAA_SAMPLES,
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
#ifdef MSAA_ENABLE
		std::array<vk::ImageView, 3> attachments = {
			*_colorResource.view(),
			*_depthResource.view(),
			*_swapchainImageViews[i]
		};
#else
		std::array<vk::ImageView, 2> attachments = {
			*_swapchainImageViews[i],
			*_depthResource.view()
		};
#endif
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

vk::raii::DescriptorSet PerFrameResources::createDescriptorSet(
	DescriptorSetFactory& descriptorSetFactory
) {
	std::array<vk::DescriptorBufferInfo, 2> bufferInfo {
		_camData.descriptorInfo(),
		_lightData.descriptorInfo()
	};
	return descriptorSetFactory.makeDescriptorSet(bufferInfo);
}

void PerFrameResources::updateCamData(const Camera& camera, unsigned int width, unsigned int height) {
	CamData mats {};
	mats.view = camera.viewMatrix();
	mats.proj = camera.projectionMatrix(width, height);
	mats.pos = camera.position;
	_camData.copyIn(mats);
}

void PerFrameResources::updateLightData(const Light& light) {
	_lightData.copyIn(light);
}

vk::raii::RenderPass Renderer::createRenderPass() {
	vk::AttachmentDescription colorAttachment {
		.format = _windowResources->swapchainFormat(),
		.samples = MSAA_SAMPLES,
		.loadOp = vk::AttachmentLoadOp::eClear,
		.storeOp = vk::AttachmentStoreOp::eStore,
		.stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
		.stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
		.initialLayout = vk::ImageLayout::eUndefined,
#ifdef MSAA_ENABLE
		.finalLayout = vk::ImageLayout::eColorAttachmentOptimal
#else
		.finalLayout = vk::ImageLayout::ePresentSrcKHR
#endif
	};
	vk::AttachmentReference colorAttachmentRef {
		.attachment = 0,
		.layout = vk::ImageLayout::eColorAttachmentOptimal
	};

	vk::AttachmentDescription depthAttachment {
		.format = _windowResources->depthResource().info().format,
		.samples = MSAA_SAMPLES,
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

#ifdef MSAA_ENABLE
	vk::AttachmentDescription resolveAttachment {
		.format = _windowResources->swapchainFormat(),
		.samples = vk::SampleCountFlagBits::e1,
		.loadOp = vk::AttachmentLoadOp::eDontCare,
		.storeOp = vk::AttachmentStoreOp::eStore,
		.stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
		.stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
		.initialLayout = vk::ImageLayout::eUndefined,
		.finalLayout = vk::ImageLayout::ePresentSrcKHR
	};
	vk::AttachmentReference resolveAttachmentReference {
		.attachment = 2,
		.layout = vk::ImageLayout::eColorAttachmentOptimal
	};
#endif

	vk::SubpassDescription subpass {
		.pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
#ifdef MSAA_ENABLE
		.pResolveAttachments = &resolveAttachmentReference,
#endif
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

#ifdef MSAA_ENABLE
	std::array<vk::AttachmentDescription, 3> attachments = {
		colorAttachment,
		depthAttachment,
		resolveAttachment
	};
#else
	std::array<vk::AttachmentDescription, 2> attachments = {
		colorAttachment,
		depthAttachment
	};
#endif

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
			// Camera
			.binding = 0,
			.descriptorType = vk::DescriptorType::eUniformBuffer,
			.descriptorCount = 1,
			.stageFlags = (
				vk::ShaderStageFlagBits::eVertex |
				vk::ShaderStageFlagBits::eFragment
			)
		},
		{
			// Light info, we only ever have one light in the scene
			.binding = 1,
			.descriptorType = vk::DescriptorType::eUniformBuffer,
			.descriptorCount = 1,
			.stageFlags = vk::ShaderStageFlagBits::eFragment
		}
	};

	return DescriptorSetFactory(_graphDevice->logicalDevice(), std::move(layoutBindings));
}

vk::raii::PipelineLayout Renderer::createPipelineLayout() {
	std::array<vk::DescriptorSetLayout, 1> descriptorSetLayouts {
		*_descriptorSetFactory.descriptorSetLayout()
	};

	std::vector<vk::PushConstantRange> pcRanges {
		{
			// Model transform
			.stageFlags = vk::ShaderStageFlagBits::eVertex,
			.offset = 0,
			.size = sizeof(glm::mat4)
		},
		{
			// Material
			.stageFlags = vk::ShaderStageFlagBits::eFragment,
			.offset = sizeof(glm::mat4),
			.size = sizeof(Material)
		}
	};

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
		.setLayoutCount = 1,
		.pSetLayouts = descriptorSetLayouts.data(),
		.pushConstantRangeCount = static_cast<uint32_t>(pcRanges.size()),
		.pPushConstantRanges = pcRanges.data()
	};
	return vk::raii::PipelineLayout(_graphDevice->logicalDevice(), pipelineLayoutInfo);
}

vk::raii::Pipeline Renderer::createLinePipeline() {
	return PipelineBuilder(_graphDevice->logicalDevice(), _pipelineLayout, _renderPass)
		.withInputType<Position>()
		.withInputType<Color>()
		.withVertexShader("unlit_vert.spv")
		.withFragmentShader("unlit_frag.spv")
		.withTopology(vk::PrimitiveTopology::eLineList)
		.build();
}

vk::raii::Pipeline Renderer::createTrianglePipeline() {
	return PipelineBuilder(_graphDevice->logicalDevice(), _pipelineLayout, _renderPass)
		.withInputType<Position>()
		.withInputType<Color>()
		.withVertexShader("unlit_vert.spv")
		.withFragmentShader("unlit_frag.spv")
		.withTopology(vk::PrimitiveTopology::eTriangleList)
		.build();
}

vk::raii::Pipeline Renderer::createLitTrianglePipeline() {
	return PipelineBuilder(_graphDevice->logicalDevice(), _pipelineLayout, _renderPass)
		.withInputType<Position>()
		.withInputType<Color>()
		.withInputType<Normal>()
		.withVertexShader("lit_vert.spv")
		.withFragmentShader("lit_frag.spv")
		.withTopology(vk::PrimitiveTopology::eTriangleList)
		.build();
}

vk::raii::Pipeline Renderer::createLitTriangleCulledPipeline() {
	return PipelineBuilder(_graphDevice->logicalDevice(), _pipelineLayout, _renderPass)
		.withInputType<Position>()
		.withInputType<Color>()
		.withInputType<Normal>()
		.withVertexShader("lit_vert.spv")
		.withFragmentShader("lit_frag.spv")
		.withTopology(vk::PrimitiveTopology::eTriangleList)
		.withCulling(vk::CullModeFlagBits::eBack)
		.build();
}

std::unordered_map<RenderMode, vk::raii::Pipeline> Renderer::createPipelines() {
	std::unordered_map<RenderMode, vk::raii::Pipeline> out;

	out.emplace(RenderMode::line, createLinePipeline());
	out.emplace(RenderMode::triangle, createTrianglePipeline());
	out.emplace(RenderMode::litTriangle, createLitTrianglePipeline());
	out.emplace(RenderMode::litTriangleCulled, createLitTriangleCulledPipeline());

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

void Renderer::beginFrame(const Camera& camera, const Light& light) {
	auto& device = _graphDevice->logicalDevice();
	auto& frameResources = _perFrameResources[_currentFrame];

	auto waitResult = device.waitForFences(*frameResources.inFlightFence(), vk::True, UINT64_MAX);
	if (waitResult != vk::Result::eSuccess) {
		std::cerr << "[RENDERER] beginFrame: Failed while waiting for inFlightFence" << std::endl;
		return;
	}

	auto [result, imageIndex] = _windowResources->swapchain().acquireNextImage(
		UINT64_MAX, frameResources.imageAvailableSemaphore()
	);

	if (result == vk::Result::eErrorOutOfDateKHR) {
		std::cerr << "[RENDERER] beginFrame: Got eErrorOutOfDateKHR, resizing..." << std::endl;
		recreateWindowResources();
		return;
	}

	const auto& extent = _windowResources->swapchainExtent();
	frameResources.updateCamData(camera, extent.width, extent.height);
	frameResources.updateLightData(light);

	device.resetFences(*frameResources.inFlightFence());

	auto& cmdBuffer = frameResources.commandBuffer();
	cmdBuffer.reset();
	cmdBuffer.begin({});

	std::array<vk::ClearValue, 2> clearValues {};
	clearValues[0].color.setFloat32({0.0f, 0.0f, 0.0f, 1.0f});
	clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

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

	// per frame descriptor set referring to camera and light info
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

	_inFrame = true;
}

void Renderer::setMode(RenderMode mode) {
	auto& cmd = currentCommandBuffer();
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipelines.at(mode));
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

BoundBuffer StaticIndexBuffer::createBuffer(const std::vector<uint16_t>& indices) {
	return makeStaticGPUBuffer(
		renderer().graphDevice(),
		indices,
		vk::BufferUsageFlagBits::eIndexBuffer
	);
}

void StaticIndexBuffer::record() const {
	auto& cmd = _renderer->currentCommandBuffer();
	cmd.bindIndexBuffer(_buffer.buffer(), 0, vk::IndexType::eUint16);
	cmd.drawIndexed(static_cast<uint32_t>(_count), 1, 0, 0, 0);
}

void StaticMesh::record() const {
	_positions.record();
	_indices.record();
}

void TransformComponent::render() {
	auto& cmd = _renderer->currentCommandBuffer();
	glm::mat4 modelMat = transform.matrix();
	cmd.pushConstants<glm::mat4>(
		*_renderer->pipelineLayout(),
		vk::ShaderStageFlagBits::eVertex,
		0,
		modelMat
	);
}

void MaterialComponent::render() {
	auto& cmd = _renderer->currentCommandBuffer();
	cmd.pushConstants<Material>(
		*_renderer->pipelineLayout(),
		vk::ShaderStageFlagBits::eFragment,
		sizeof(glm::mat4),
		*_material
	);
}

void populateStaticEntity(
	Renderer& renderer,
	Entity& entity,
	const Transform& transform,
	StaticMesh& mesh,
	StaticVertexAttributes<Color>& colors
) {
	entity.addComponent<TransformComponent>(renderer, transform);
	entity.addComponent<StaticMeshComponent>(mesh);
	entity.addComponent<StaticVertexAttributeComponent<Color>>(colors);

	entity.setLastRender<StaticMeshComponent>();
}

void populateStaticEntity(
	Renderer& renderer,
	Entity& entity,
	const Transform& transform,
	StaticMesh& mesh,
	StaticVertexAttributes<Color>& colors,
	StaticVertexAttributes<Normal>& normals
) {
	populateStaticEntity(renderer, entity, transform, mesh, colors);
	entity.addComponent<StaticVertexAttributeComponent<Normal>>(normals);
}

Transform& getTransform(Entity& entity) {
	return entity.requireComponent<TransformComponent>().transform;
}
}
