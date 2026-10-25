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

	TimerCollection* _perfTimers;
	DiscriminatedStringMap* _perfIds;
	Range* _range;

	std::vector<Position> _positions;
	std::vector<Color> _colors;
	std::vector<Normal> _normals;

	std::vector<uint16_t> _triangleIndices;
	std::vector<uint16_t> _lineIndices;
	std::vector<uint16_t> _normalIndices;

	uint16_t idx(unsigned int x, unsigned int y);
	const Position& getPosition(unsigned int x, unsigned int y);
public:
	// The number of discrete steps to walk along each input variable.
	// Higher values yield higher accuracy at the cost of additional
	// vertices.
	unsigned int cells;

	bool clampZ = false;

	GraphMeshBuilder() = delete;
	GraphMeshBuilder(
		unsigned int cells,
		Range& range,
		TimerCollection& perfTimers,
		DiscriminatedStringMap& perfIds
	)
	: cells { cells },
	  _range { &range },
	  _perfTimers { &perfTimers },
	  _perfIds { &perfIds }
	{}

	const auto& positions() const { return _positions; }
	const auto& colors() const { return _colors; }
	const auto& normals() const { return _normals; }
	const auto& lineIndices() const { return _lineIndices; }
	const auto& triangleIndices() const { return _triangleIndices; }
	const auto& normalIndices() const { return _normalIndices; }
	auto pointCount() const { return (cells + 1) * (cells + 1); }

	template<size_t IN_DIMS, size_t OUT_DIMS>
	void generatePositions(Function<IN_DIMS, OUT_DIMS>& func) {
		// XXX gross, should work on better generalizing this
		static_assert(
			IN_DIMS + OUT_DIMS == 3 &&
			IN_DIMS > 0 &&
			OUT_DIMS > 0,
			"generatePositions<I, O>() requires I = 1,2; O = 1,2; I+O=3"
		);

		_perfTimers->start((*_perfIds)["regenPositions"]);

		// NOTE: Explicitly giving the dimension like this is temporary.
		float inc = 1.0f / static_cast<float>(cells);
		std::array<float, IN_DIMS> inputs { 0.0f, 0.0f };
		MultidimensionalCounter<IN_DIMS> counter { cells, [this, &inputs, inc](size_t i, size_t cnt) {
			inputs[i] = glm::mix(_range->low()[i], _range->high()[i], inc*cnt);
		}};

		while (!counter.atEnd()) {
			// XXX also gross
			if constexpr (IN_DIMS == 2) {
				_positions.push_back(_range->toModelSpace({
					inputs[0], inputs[1], func.eval(inputs)[0]
				}));

				if (clampZ) {
					auto& p = _positions.back().vec;
					p.z = glm::clamp(p.z, -1.0f, 1.0f);
				}
			} else if constexpr (OUT_DIMS == 2) {
				auto outputs = func.eval(inputs);
				_positions.push_back(_range->toModelSpace({
					inputs[0], outputs[0], outputs[1]
				}));

				// TODO: Rename "clampOut".
				if (clampZ) {
					auto& p = _positions.back().vec;
					p.y = glm::clamp(p.y, -1.0f, 1.0f);
					p.z = glm::clamp(p.z, -1.0f, 1.0f);
				}
			}

			counter.inc();
		}

		_perfTimers->stop((*_perfIds)["regenPositions"]);
	}

	void generateNormalPositions();
	void clearPositions();

	template<size_t IN_DIMS>
	void generateColors(const GraphAppearance<IN_DIMS>& appearance) {
		_perfTimers->start((*_perfIds)["regenColors"]);
		float inc = 1.0f / static_cast<float>(cells);

		MultidimensionalLerpCursor<glm::vec3, IN_DIMS> lerpCursor {
			appearance.gradientEndpoints
		};

		int lerpDim = -1;
		size_t lerpCnt = 0;

		// Bit of a workaround here. Say we have a MultidimensionalCounter<3>
		// where cells=10, _array={10, 10, 2} called "counter". On the next
		// counter.update(), _array[0] will be updated first, then [1], then
		// [2], and the given lamba here will run in that order.
		//
		// Problem is, in MultidimensionalLerpCursor, the order of dependencies
		// is reversed. [2] must be updated first, *then* [1], in order for [1]
		// to reset to the correct value. Similarly, [0] is dependent on [1].
		//
		// To work around this, we use some tracking variables to only run
		// lerpCursor.update() on the highest dimension that was updated on
		// this count, since lerpCursor.update() takes care of the proper lower
		// dimension resets on its own.
		MultidimensionalCounter<IN_DIMS> counter { cells,
			[this, &lerpDim, &lerpCnt](size_t i, size_t cnt) {
				int i_int = static_cast<int>(i);
				if (i_int > lerpDim) {
					lerpDim = i_int;
					lerpCnt = cnt;
				}
			}
		};

		while (!counter.atEnd()) {
			_colors.push_back(lerpCursor.value());
			counter.inc();
			lerpCursor.update(lerpDim, lerpCnt*inc);
			lerpDim = -1;
		}
		_perfTimers->stop((*_perfIds)["regenColors"]);
	}

	void clearColors();

	void generateNormals();
	void clearNormals();

	void generateLineIndices();
	void generateTriangleIndices();
	void generateNormalIndices();
	void clearIndices();
};

enum class GraphRenderMode : int {
	surface = 0,
	wireframe,
	none
};

constexpr unsigned int GraphRenderModeCount = 3;

// Surface-specific regeneration.
class SurfaceGraphMeshBuilder {
private:
	Function<2, 1>* _func;
	GraphAppearance<2>* _appearance;
	GraphMeshBuilder _genericBuilder;

public:
	SurfaceGraphMeshBuilder() = delete;
	SurfaceGraphMeshBuilder(
		Function<2, 1>& f,
		unsigned int cells,
		Range& range,
		GraphAppearance<2>& appearance,
		TimerCollection& perfTimers,
		DiscriminatedStringMap& perfIds
	)
	: _func { &f },
	  _appearance { &appearance },
	  _genericBuilder {cells, range, perfTimers, perfIds }
	{
		regenerateIndices();
		regeneratePositions();
		regenerateColors();
	}

	void regeneratePositions();
	void regenerateColors();
	void regenerateIndices();

	auto& genericBuilder() { return _genericBuilder; }
	const auto& genericBuilder() const { return _genericBuilder; }
};

struct SurfaceGraphBuffers {
private:
	std::vector<Color> makeGridColors(const GraphMeshBuilder& builder);
	std::vector<Color> makeNormalColors(const GraphMeshBuilder& builder);
public:
	DynamicVertexAttributes<Position> surfacePositions;

	DynamicIndexBuffer surfaceIndices;
	DynamicVertexAttributes<Color> surfaceColors;
	DynamicVertexAttributes<Normal> surfaceNormals;

	DynamicIndexBuffer gridIndices;
	DynamicVertexAttributes<Color> gridColors;

	DynamicIndexBuffer normalIndices;
	DynamicVertexAttributes<Color> normalColors;

	SurfaceGraphBuffers(Renderer& renderer, const GraphMeshBuilder& builder)
	: surfacePositions { renderer, builder.positions() },
	  surfaceIndices { renderer, builder.triangleIndices() },
	  surfaceColors { renderer, builder.colors() },
	  surfaceNormals { renderer, builder.normals() },
	  gridIndices { renderer, builder.lineIndices() },
	  gridColors { renderer, makeGridColors(builder) },
	  normalIndices { renderer, builder.normalIndices() },
	  normalColors { renderer, makeNormalColors(builder) }
	{
		uploadPositions(builder);
		uploadColors(builder);
		uploadIndices(builder);
	}

	void uploadPositions(const GraphMeshBuilder& builder);
	void uploadColors(const GraphMeshBuilder& builder);
	void uploadIndices(const GraphMeshBuilder& builder);
};

class SurfaceGraphFigure : public MathFigure {
private:
	static constexpr float gridLoft = 0.002f;
	static constexpr std::array<char, 2> inputVars { 'x', 'y' };

	unsigned int _id;

	Function<2, 1> _function;
	DiscriminatedStringMap _perfIds;
	GraphAppearance<2> _appearance;
	SurfaceGraphMeshBuilder _builder;
	TimerCollection* _perfTimers;

	Flag shouldUpdate;
	Flag cellsChanged;

	Flag inRegen;
	GraphUpdateMode _regenMode = GraphUpdateMode::none;

	// When the cells value changes, the buffers need to be updated for
	// multiple frames since everything is double buffered.
	unsigned int uploadFrames = 0;
	GraphUpdateMode _uploadMode = GraphUpdateMode::none;

	SurfaceGraphBuffers _buffers;
	Entity _surface;
	Entity _gridTop;
	Entity _gridBottom;
	Entity _normals;
	Entity _wireframe;

	void handleRangeChanged(const RangeChangedEvent& e);
	void handleFigureRemoved(const FigureRemovedEvent& e);

	CompoundMethodEventHandler<SurfaceGraphFigure,
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

	void setRegenMode(GraphUpdateMode mode);
	void setUploadMode(GraphUpdateMode mode);
	void setUpdateMode(GraphUpdateMode mode);

	void regen();
	void upload();
public:
	bool doUpload = true;
	bool doRegen = true;
	bool renderGrid = true;
	bool renderNormals = false;
	GraphRenderMode renderMode = GraphRenderMode::surface;

	SurfaceGraphFigure(
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
	  _buffers { renderer, _builder.genericBuilder() },
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

	SurfaceGraphFigure(SurfaceGraphFigure&& other)
	: _id { other._id },
	  _function { std::move(other._function) },
	  _builder { std::move(other._builder) },
	  _appearance { other._appearance },
	  _buffers { std::move(other._buffers) },
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
