#include "vk/helper.h"

#include "vk/constants.h"

#include <fstream>
#include <iostream>

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
vk::raii::ShaderModule loadShader(vk::raii::Device& device, const std::string& filename) {
	std::vector<char> code = readFile(filename);
	vk::ShaderModuleCreateInfo createInfo {
		.codeSize = code.size(),
		.pCode = reinterpret_cast<const uint32_t*>(code.data())
	};
	return vk::raii::ShaderModule(device, createInfo);
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
	return vk::raii::DescriptorSetLayout(*_device, createInfo);
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

	return vk::raii::DescriptorPool(*_device, info);
}

vk::raii::DescriptorSet DescriptorSetFactory::makeDescriptorSet(
	vk::ArrayProxy<vk::DescriptorBufferInfo> bufferInfo
) {
	vk::DescriptorSetAllocateInfo allocInfo {
		.descriptorPool = *_descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &(*_descriptorSetLayout)
	};

	auto descriptorSet = std::move(_device->allocateDescriptorSets(allocInfo)[0]);

	std::vector<vk::WriteDescriptorSet> writeInfo;
	for (size_t i = 0; i < _layoutBindings.size(); ++i) {
		const auto& binding = _layoutBindings[i];
		writeInfo.push_back({
			.dstSet = *descriptorSet,
			.dstBinding = binding.binding,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = binding.descriptorType,
			.pBufferInfo = &bufferInfo.data()[i]
		});
	}

	_device->updateDescriptorSets(writeInfo, nullptr);

	return descriptorSet;
}

vk::raii::Pipeline PipelineBuilder::build() {
	if (_vertexShader.size() == 0) {
		throw std::runtime_error("Vertex shader must be specified");
	}

	if (_fragmentShader.size() == 0) {
		throw std::runtime_error("Fragment shader must be specified");
	}

	if (_inputBindings.size() == 0) {
		throw std::runtime_error("At least one input type must be specified");
	}

	// SHADERS
	vk::raii::ShaderModule vertModule = loadShader(*_device, _vertexShader);
	vk::raii::ShaderModule fragModule = loadShader(*_device, _fragmentShader);

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
	vk::PipelineVertexInputStateCreateInfo vertexInputInfo {
		.vertexBindingDescriptionCount = static_cast<uint32_t>(_inputBindings.size()),
		.pVertexBindingDescriptions = _inputBindings.data(),
		.vertexAttributeDescriptionCount = static_cast<uint32_t>(_inputAttributes.size()),
		.pVertexAttributeDescriptions = _inputAttributes.data()
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
		.rasterizationSamples = MSAA_SAMPLES,
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

	return vk::raii::Pipeline(*_device, nullptr, pipelineInfo);
}
}
