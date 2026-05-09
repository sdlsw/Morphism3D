#pragma once
#include "global_defines.h"

// Helper classes and functions that depend minimally on non-standard types.

#include "vk_concept.h"
#include "vk_datatypes.h"

#include <vulkan/vulkan_raii.hpp>

#include <string>

namespace g3d {
// Loads a shader from a file.
vk::raii::ShaderModule loadShader(vk::raii::Device& device, const std::string& filename);

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

class DescriptorSetFactory {
private:
	const uint32_t MAX_DESCRIPTOR_SETS = 100;

	vk::raii::Device* _device;
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
		vk::raii::Device& device,
		const std::vector<vk::DescriptorSetLayoutBinding>& layoutBindings
	)
	: _device { &device },
	  _layoutBindings { layoutBindings },
	  _descriptorSetLayout { createDescriptorSetLayout() },
	  _descriptorPool { createDescriptorPool() }
	{}
	DescriptorSetFactory(
		vk::raii::Device& device,
		std::vector<vk::DescriptorSetLayoutBinding>&& layoutBindings
	)
	: _device { &device },
	  _layoutBindings { layoutBindings},
	  _descriptorSetLayout { createDescriptorSetLayout() },
	  _descriptorPool { createDescriptorPool() }
	{}

	auto& descriptorSetLayout() { return _descriptorSetLayout; }
	vk::raii::DescriptorSet makeDescriptorSet(
		vk::ArrayProxy<vk::DescriptorBufferInfo> buffers
	);
};

class PipelineBuilder {
private:
	vk::raii::Device* _device;

	// TODO: For now, just use externally specified pipeline layout and
	// render pass. Should look into putting them together in the builder.
	vk::raii::PipelineLayout* _pipelineLayout;
	vk::raii::RenderPass* _renderPass;

	std::vector<vk::VertexInputBindingDescription> _inputBindings;
	std::vector<vk::VertexInputAttributeDescription> _inputAttributes;

	// Builder variables
	std::string _vertexShader {};
	std::string _fragmentShader {};
	vk::PrimitiveTopology _topology = vk::PrimitiveTopology::eLineList;
public:
	PipelineBuilder() = delete;
	PipelineBuilder(PipelineBuilder&&) = default;

	PipelineBuilder(
		vk::raii::Device& device,
		vk::raii::PipelineLayout& pipelineLayout,
		vk::raii::RenderPass& renderPass
	)
	: _device { &device },
	  _pipelineLayout { &pipelineLayout },
	  _renderPass { &renderPass }
	{}

	PipelineBuilder& withVertexShader(const std::string& s) { _vertexShader = s; return *this; }
	PipelineBuilder& withFragmentShader(const std::string& s) { _fragmentShader = s; return *this; }
	PipelineBuilder& withTopology(vk::PrimitiveTopology top) { _topology = top; return *this; }

	template<typename T>
	PipelineBuilder& withInputType() {
		_inputBindings.push_back(getVertexBindingDescription<T>());
		_inputAttributes.push_back(getVertexAttributeDescription<T>());
		return *this;
	}

	vk::raii::Pipeline build();
};
}
