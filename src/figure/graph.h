#pragma once

#include "bitmask.h"
#include "container.h"
#include "figure/iface.h"
#include "function.h"
#include "iteration.h"
#include "primitive.h"
#include "range.h"
#include "statistics.h"
#include "vk/renderer.h"

#include <concepts>
#include <vector>

namespace g3d {
template<size_t DIMS>
struct GraphAppearance {
	// Colors for extreme points of graph.
	// For example, for DIMS = 2,
	// left.left - (-range, -range)
	// left.right - (range, -range)
	// right.left - (-range, range)
	// right.right - (range, range)
	using EndpointType = FixedSizeBinaryTree<glm::vec3, DIMS>;
	EndpointType gradientEndpoints;

	WithInitial<Material> material;

	Flag colorChanged;

	GraphAppearance(
		const EndpointType& gradientEndpoints,
		const Material& material
	)
	: gradientEndpoints { gradientEndpoints },
	  material {{ material }}
	{}

	GraphAppearance(
		const std::span<glm::vec3, EndpointType::spanSize>& gradientEndpoints,
		const Material& material
	)
	: GraphAppearance(EndpointType::fromSpan(gradientEndpoints), material) {}

	GraphAppearance(
		const glm::vec3& color,
		const Material& material
	)
	: GraphAppearance(EndpointType::fromValue(color), material) {}

	GraphAppearance(const glm::vec3& color)
	: GraphAppearance(color, defaultMaterial()) {}

	GraphAppearance() : GraphAppearance({0.0f, 0.0f, 0.0f}) {}

	void setSingleColor(const glm::vec3& color) {
		gradientEndpoints.fill(color);
	}
};

GraphAppearance<2> default2dAppearance();

enum class GraphUpdateMode : uint8_t {
	none = 0,
	positions = 1 << 1,
	colors = 1 << 2,
	indices = 1 << 3
};

template<>
struct enableBitmaskOps<GraphUpdateMode> {
	static constexpr bool value = true;
};

class GraphMeshBuilder {
private:
	static constexpr float normLength = 0.1f;

	Function<2, 1>* _func;
	TimerCollection* _perfTimers;
	DiscriminatedStringMap* _perfIds;
	Range* _range;
	GraphAppearance<2>* _appearance;

	std::vector<Position> _positions;
	std::vector<Color> _colors;
	std::vector<Normal> _normals;

	std::vector<uint16_t> _triangleIndices;
	std::vector<uint16_t> _lineIndices;
	std::vector<uint16_t> _normalIndices;

	uint16_t idx(unsigned int x, unsigned int y);
	const Position& getPosition(unsigned int x, unsigned int y);

	void generatePositions();
	void generateColors();
	void generateNormals();
	void generateNormalPositions();
	void generateLineIndices();
	void generateTriangleIndices();
	void generateNormalIndices();
	void regeneratePositions();
	void regenerateColors();
	void regenerateIndices();
public:
	// The number of discrete steps to walk along each input variable.
	// Higher values yield higher accuracy at the cost of additional
	// vertices.
	unsigned int cells;

	bool clampZ = false;

	GraphMeshBuilder() = delete;
	GraphMeshBuilder(
		Function<2, 1>& f,
		unsigned int cells,
		Range& range,
		GraphAppearance<2>& appearance,
		TimerCollection& perfTimers,
		DiscriminatedStringMap& perfIds
	)
	: _func { &f },
	  cells { cells },
	  _range { &range },
	  _appearance { &appearance },
	  _perfTimers { &perfTimers },
	  _perfIds { &perfIds }
	{
		regenerate(allEnabled<GraphUpdateMode>());
	}

	const auto& positions() const { return _positions; }
	const auto& colors() const { return _colors; }
	const auto& normals() const { return _normals; }
	const auto& lineIndices() const { return _lineIndices; }
	const auto& triangleIndices() const { return _triangleIndices; }
	const auto& normalIndices() const { return _normalIndices; }
	auto pointCount() const { return (cells + 1) * (cells + 1); }

	void regenerate(const GraphUpdateMode& mode);
};

enum class GraphRenderMode : int {
	surface = 0,
	wireframe,
	none
};

constexpr unsigned int GraphRenderModeCount = 3;

class GraphFigure : public MathFigure {
private:
	static constexpr float gridLoft = 0.002f;
	static constexpr std::array<char, 2> inputVars { 'x', 'y' };

	unsigned int _id;

	Function<2, 1> _function;
	DiscriminatedStringMap _perfIds;
	GraphAppearance<2> _appearance;
	GraphMeshBuilder _builder;
	TimerCollection* _perfTimers;

	Flag shouldUpdate;
	Flag cellsChanged;

	Flag inRegen;
	GraphUpdateMode _regenMode = GraphUpdateMode::none;

	// When the cells value changes, the buffers need to be updated for
	// multiple frames since everything is double buffered.
	unsigned int uploadFrames = 0;
	GraphUpdateMode _uploadMode = GraphUpdateMode::none;

	DynamicVertexAttributes<Position> _surfacePositions;

	DynamicIndexBuffer _surfaceIndices;
	DynamicVertexAttributes<Color> _surfaceColors;
	DynamicVertexAttributes<Normal> _surfaceNormals;
	Entity _surface;

	DynamicIndexBuffer _gridIndices;
	DynamicVertexAttributes<Color> _gridColors;
	Entity _gridTop;
	Entity _gridBottom;

	DynamicIndexBuffer _normalIndices;
	DynamicVertexAttributes<Color> _normalColors;
	Entity _normals;

	Entity _wireframe;

	void handleRangeChanged(const RangeChangedEvent& e);
	void handleFigureRemoved(const FigureRemovedEvent& e);

	CompoundMethodEventHandler<GraphFigure,
		RangeChangedEvent,
		FigureRemovedEvent
	> _eventHandlers { this,
		std::mem_fn(handleRangeChanged),
		std::mem_fn(handleFigureRemoved)
	};

	void populateSurfaceEntity(Renderer& renderer);
	void populateGridEntity(Renderer& renderer, Entity& ent, float loftMult);
	void populateWireframeEntity(Renderer& renderer);
	void populateNormalEntity(Renderer& renderer);

	std::vector<Color> makeGridColors();
	std::vector<Color> makeNormalColors();

	void setRegenMode(GraphUpdateMode mode);
	void setUploadMode(GraphUpdateMode mode);
	void setUpdateMode(GraphUpdateMode mode);

	void regen();
	void uploadPositions();
	void uploadColors();
	void uploadIndices();
	void upload();
public:
	bool doUpload = true;
	bool doRegen = true;
	bool renderGrid = true;
	bool renderNormals = false;
	GraphRenderMode renderMode = GraphRenderMode::surface;

	GraphFigure(
		unsigned int id,
		EventRouter& eventRouter,
		Renderer& renderer,
		VariableStore& variableStore,
		unsigned int cells,
		Range& range,
		TimerCollection& perfTimers
	)
	: _id { id },
	  _function { variableStore, inputVars },
	  _perfIds { std::format("_graph{}", id) },
	  _appearance { default2dAppearance() },
	  _builder { _function, cells, range, _appearance, perfTimers, _perfIds },
	  _surfacePositions { renderer, _builder.positions() },
	  _surfaceColors { renderer, _builder.colors() },
	  _surfaceNormals { renderer, _builder.normals() },
	  _surfaceIndices { renderer, _builder.triangleIndices() },
	  _gridIndices { renderer, _builder.lineIndices() },
	  _gridColors { renderer, makeGridColors() },
	  _normalIndices { renderer, _builder.normalIndices() },
	  _normalColors { renderer, makeNormalColors() },
	  _regenMode { GraphUpdateMode::none },
	  _uploadMode { GraphUpdateMode::none },
	  _perfTimers { &perfTimers }
	{
		populateSurfaceEntity(renderer);
		populateGridEntity(renderer, _gridTop, 1.0f);
		populateGridEntity(renderer, _gridBottom, -1.0f);
		populateWireframeEntity(renderer);
		populateNormalEntity(renderer);

		_eventHandlers.addToRouter(eventRouter);
	}

	GraphFigure(GraphFigure&& other)
	: _id { other._id },
	  _function { std::move(other._function) },
	  _builder { std::move(other._builder) },
	  _appearance { other._appearance },
	  _surfacePositions { std::move(other._surfacePositions) } ,
	  _surfaceColors { std::move(other._surfaceColors) },
	  _surfaceNormals { std::move(other._surfaceNormals) },
	  _surfaceIndices { std::move(other._surfaceIndices) },
	  _gridIndices { std::move(other._gridIndices) },
	  _gridColors { std::move(other._gridColors) },
	  _normalIndices { std::move(other._normalIndices) },
	  _normalColors { std::move(other._normalColors) },
	  _regenMode { other._regenMode },
	  _uploadMode { other._uploadMode },
	  _perfTimers { other._perfTimers },
	  _eventHandlers { std::move(other._eventHandlers) },
	  _perfIds { std::move(other._perfIds) }
	{
		_eventHandlers.updateThis(this);
	}

	unsigned int id() const override { return _id; }

	auto& func() { return _function; }

	auto& appearance() { return _appearance; }

	auto& surface() { return _surface; }
	auto& gridTop() { return _gridTop; }
	auto& gridBottom() { return _gridBottom; }
	auto& normals() { return _normals; }
	auto& wireframe() { return _wireframe; }

	void clampZ(bool b);
	bool clampZ() const;

	void cells(unsigned int cells);
	unsigned int cells() const;

	// TODO: Should really flesh out the entity system so I don't have to
	// do this.
	void updateSynchronized() override;
	void draw() override;
};
}
