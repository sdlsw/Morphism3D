#pragma once

#include "bitmask.h"
#include "container.h"
#include "figure/iface.h"
#include "function.h"
#include "iteration.h"
#include "primitive.h"
#include "range.h"
#include "settings.h"
#include "statistics.h"
#include "vk/renderer.h"

#include <concepts>
#include <vector>

namespace g3d {
template<size_t DIMS>
struct GraphColors {
	// Colors for extreme points of graph.
	// For example, for DIMS = 2,
	// left.left - (-range, -range)
	// left.right - (range, -range)
	// right.left - (-range, range)
	// right.right - (range, range)
	using EndpointType = FixedSizeBinaryTree<glm::vec3, DIMS>;
	EndpointType gradientEndpoints;

	Flag colorChanged;

	GraphColors(const EndpointType& gradientEndpoints)
	: gradientEndpoints { gradientEndpoints } {}

	GraphColors(
		const std::span<glm::vec3, EndpointType::spanSize>& gradientEndpoints
	)
	: GraphColors(EndpointType::fromSpan(gradientEndpoints)) {}

	GraphColors(const glm::vec3& color)
	: GraphColors(EndpointType::fromValue(color)) {}

	GraphColors() : GraphColors({0.0f, 0.0f, 0.0f}) {}

	void setSingleColor(const glm::vec3& color) {
		gradientEndpoints.fill(color);
	}
};

GraphColors<2> default2dColors();

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

	bool clampOut = false;

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

				if (clampOut) {
					auto& p = _positions.back().vec;
					p.z = glm::clamp(p.z, -1.0f, 1.0f);
				}
			} else if constexpr (OUT_DIMS == 2) {
				auto outputs = func.eval(inputs);
				_positions.push_back(_range->toModelSpace({
					inputs[0], outputs[0], outputs[1]
				}));

				if (clampOut) {
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
	void generateColors(const GraphColors<IN_DIMS>& colors) {
		_perfTimers->start((*_perfIds)["regenColors"]);
		float inc = 1.0f / static_cast<float>(cells);

		MultidimensionalLerpCursor<glm::vec3, IN_DIMS> lerpCursor {
			colors.gradientEndpoints
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

struct GraphState {
	Flag shouldRegenPositions;
	Flag cellsChanged;

	Flag inRegen;
	GraphUpdateMode regenMode = GraphUpdateMode::none;

	// When the cells value changes, the buffers need to be updated for
	// multiple frames since everything is double buffered.
	unsigned int uploadFrames = 0;
	GraphUpdateMode uploadMode = GraphUpdateMode::none;

	void setRegenMode(GraphUpdateMode mode);
	void setUploadMode(GraphUpdateMode mode);
	void setUpdateMode(GraphUpdateMode mode);

	void decRegen();
	void decUpload();

	void update(Flag& functionChanged, Flag& colorsChanged);
	void updateEnd();
};

template<typename BUILDER, typename DISPLAY>
class GraphFigure : public MathFigure {
	using BuilderType = BUILDER;
	using DisplayType = DISPLAY;

private:
	unsigned int _id;

	DiscriminatedStringMap _perfIds;
	TimerCollection* _perfTimers;
	DebugSettings* _debugSettings;

	BUILDER _builder;
	DISPLAY _display;
	GraphState _state;

	void handleRangeChanged(const RangeChangedEvent& e);
	void handleFigureRemoved(const FigureRemovedEvent& e);

	CompoundMethodEventHandler<GraphFigure<BUILDER, DISPLAY>,
		RangeChangedEvent,
		FigureRemovedEvent
	> _eventHandlers { this,
		std::mem_fn(handleRangeChanged),
		std::mem_fn(handleFigureRemoved)
	};

	void regen();
	void upload();
public:
	bool doUpload = true;
	bool doRegen = true;

	GraphFigure(
		unsigned int id,
		EventRouter& eventRouter,
		Renderer& renderer,
		VariableStore& variableStore,
		unsigned int cells,
		Range& range,
		TimerCollection& perfTimers,
		DebugSettings& debugSettings
	)
	: _id { id },
	  _perfIds { std::format("_graph{}", id) },
	  _builder { variableStore, cells, range, perfTimers, _perfIds },
	  _display { renderer, _builder.genericBuilder() },
	  _perfTimers { &perfTimers },
	  _debugSettings { &debugSettings }
	{
		_eventHandlers.addToRouter(eventRouter);
	}

	GraphFigure(GraphFigure<BUILDER, DISPLAY>&& other)
	: _id { other._id },
	  _builder { std::move(other._builder) },
	  _display { std::move(other._display) },
	  _state { std::move(other._state) },
	  _perfTimers { other._perfTimers },
	  _eventHandlers { std::move(other._eventHandlers) },
	  _perfIds { std::move(other._perfIds) },
	  _debugSettings { other._debugSettings }
	{
		_eventHandlers.updateThis(this);
	}

	unsigned int id() const override { return _id; }

	auto& builder() { return _builder; }
	auto& display() { return _display; }
	auto& debugSettings() { return *_debugSettings; }

	void clampOut(bool b);
	bool clampOut() const;

	void cells(unsigned int cells);
	unsigned int cells() const;

	// TODO: Should really flesh out the entity system so I don't have to
	// do this.
	void updateSynchronized() override;
	void draw() override;
};

template<typename B, typename D>
void GraphFigure<B, D>::handleRangeChanged(const RangeChangedEvent& e) {
	_state.shouldRegenPositions.set();
}

template<typename B, typename D>
void GraphFigure<B, D>::handleFigureRemoved(const FigureRemovedEvent& e) {
	if (e.id != _id) return;

	for (const auto& [_, perfId] : _perfIds.cache()) {
		_perfTimers->removeTimer(perfId);
	}
}

template<typename B, typename D>
void GraphFigure<B, D>::regen() {
	if (!doRegen) return;

	_perfTimers->start(_perfIds["regen"]);
	// Indices need to be regenerated first, since the
	// generateNormals() is dependent on them.
	if (flagsEnabled(_state.regenMode, GraphUpdateMode::indices)) {
		_builder.regenerateIndices();
	}

	if (flagsEnabled(_state.regenMode, GraphUpdateMode::colors)) {
		_builder.regenerateColors();
	}

	if (flagsEnabled(_state.regenMode, GraphUpdateMode::positions)) {
		_builder.regeneratePositions();
	}
	_perfTimers->stop(_perfIds["regen"]);
}

template<typename B, typename D>
void GraphFigure<B, D>::upload() {
	if (!doUpload) return;

	_perfTimers->start(_perfIds["upload"]);
	if (flagsEnabled(_state.uploadMode, GraphUpdateMode::positions)) {
		_display.uploadPositions(_builder.genericBuilder());
	}

	if (flagsEnabled(_state.uploadMode, GraphUpdateMode::colors)) {
		_display.uploadColors(_builder.genericBuilder());
	}

	if (flagsEnabled(_state.uploadMode, GraphUpdateMode::indices)) {
		_display.uploadIndices(_builder.genericBuilder());
	}
	_perfTimers->stop(_perfIds["upload"]);
}

template<typename B, typename D>
void GraphFigure<B, D>::clampOut(bool b) {
	_builder.genericBuilder().clampOut = b;
	_state.shouldRegenPositions.set();
}

template<typename B, typename D>
bool GraphFigure<B, D>::clampOut() const {
	return _builder.genericBuilder().clampOut;
}

template<typename B, typename D>
void GraphFigure<B, D>::cells(unsigned int cells) {
	_builder.genericBuilder().cells = cells;
	_state.cellsChanged.set();
}

template<typename B, typename D>
unsigned int GraphFigure<B, D>::cells() const {
	return _builder.genericBuilder().cells;
}

template<typename B, typename D>
void GraphFigure<B, D>::updateSynchronized() {
	_perfTimers->start(_perfIds["updateSynchronized"]);
	_state.update(_builder.funcUpdated(), _builder.colorsUpdated());
	regen();
	upload();
	_state.updateEnd();
	_perfTimers->stop(_perfIds["updateSynchronized"]);
}

template<typename B, typename D>
void GraphFigure<B, D>::draw() {
	_display.draw();
}
}
