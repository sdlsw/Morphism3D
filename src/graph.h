#pragma once

#include "container.h"
#include "function.h"
#include "primitive.h"
#include "range.h"
#include "statistics.h"
#include "vk/renderer.h"

#include <concepts>
#include <vector>

namespace g3d {
class GraphMeshBuilder {
private:
	static constexpr float normLength = 0.1f;

	Function* _func;
	TimerCollection* _perfTimers;
	Range* _range;

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
	void regenerateVertices();
	void regenerateIndices();
public:
	// The number of discrete steps to walk along each input variable.
	// Higher values yield higher accuracy at the cost of additional
	// vertices.
	unsigned int cells;

	bool clampZ = false;

	GraphMeshBuilder() = delete;
	GraphMeshBuilder(Function& f, unsigned int cells, Range& range, TimerCollection& perfTimers)
	: _func { &f },
	  cells { cells },
	  _range { &range },
	  _perfTimers { &perfTimers }
	{
		regenerateEverything();
	}

	const auto& positions() const { return _positions; }
	const auto& colors() const { return _colors; }
	const auto& normals() const { return _normals; }
	const auto& lineIndices() const { return _lineIndices; }
	const auto& triangleIndices() const { return _triangleIndices; }
	const auto& normalIndices() const { return _normalIndices; }
	auto pointCount() const { return (cells + 1) * (cells + 1); }

	// Must be called whenever `_func` changes, and whenever `_range` changes
	void regeneratePositions();

	// Must be called whenever `cells` changes.
	void regenerateEverything();
};

enum class GraphRegenMode {
	none,
	partial,
	all
};

enum class GraphUploadMode {
	none,
	partial,
	all
};

enum class GraphRenderMode : int {
	surface = 0,
	wireframe,
	none
};

constexpr unsigned int GraphRenderModeCount = 3;

class Graph {
private:
	static constexpr float gridLoft = 0.002f;

	Function _function;
	GraphMeshBuilder _builder;
	TimerCollection* _perfTimers;

	bool cellsChanged = false;
	bool temporaryRegen = false;
	bool shouldUpdate = false;

	// When the cells value changes, the buffers need to be updated for
	// multiple frames since everything is double buffered.
	unsigned int temporaryUploadFrames = 0;
	GraphRegenMode _regenMode = GraphRegenMode::none;
	GraphUploadMode _uploadMode = GraphUploadMode::none;

	DynamicVertexAttributes<Position> _surfacePositions;

	DynamicIndexBuffer _surfaceIndices;
	DynamicVertexAttributes<Color> _surfaceColors;
	DynamicVertexAttributes<Normal> _surfaceNormals;
	WithInitial<Material> _surfaceMaterial { defaultMaterial() };
	Entity _surface;

	DynamicIndexBuffer _gridIndices;
	DynamicVertexAttributes<Color> _gridColors;
	Entity _gridTop;
	Entity _gridBottom;

	DynamicIndexBuffer _normalIndices;
	DynamicVertexAttributes<Color> _normalColors;
	Entity _normals;

	Entity _wireframe;

	class _RangeChangedHandler : public EventHandler<RangeChangedEvent> {
	public:
		Graph* _this;
		void handle(const RangeChangedEvent& e) override;
		_RangeChangedHandler(Graph* _this) : _this { _this } {}
	};

	_RangeChangedHandler _rangeChangedHandler { this };

	void populateSurfaceEntity(Renderer& renderer);
	void populateGridEntity(Renderer& renderer, Entity& ent, float loftMult);
	void populateWireframeEntity(Renderer& renderer);
	void populateNormalEntity(Renderer& renderer);

	std::vector<Color> makeGridColors();
	std::vector<Color> makeNormalColors();

	void setRegenMode(GraphRegenMode mode);
	void setUploadMode(GraphUploadMode mode);

	GraphRegenMode defaultRegenMode();
	GraphUploadMode defaultUploadMode();

	void setTemporaryRegenMode(GraphRegenMode mode);
	void setTemporaryUploadMode(GraphUploadMode mode);

	void regen();
	void uploadPartial();
	void uploadAll();
	void upload();
public:
	bool doUpload = true;
	bool doRegen = true;
	bool renderGrid = true;
	bool renderNormals = false;
	GraphRenderMode renderMode = GraphRenderMode::surface;

	Graph(
		EventRouter& eventRouter,
		Renderer& renderer,
		VariableStore& variableStore,
		unsigned int cells,
		Range& range,
		TimerCollection& perfTimers
	)
	: _function { variableStore },
	  _builder { _function, cells, range, perfTimers },
	  _surfacePositions { renderer, _builder.positions() },
	  _surfaceColors { renderer, _builder.colors() },
	  _surfaceNormals { renderer, _builder.normals() },
	  _surfaceIndices { renderer, _builder.triangleIndices() },
	  _gridIndices { renderer, _builder.lineIndices() },
	  _gridColors { renderer, makeGridColors() },
	  _normalIndices { renderer, _builder.normalIndices() },
	  _normalColors { renderer, makeNormalColors() },
	  _regenMode { defaultRegenMode() },
	  _uploadMode { defaultUploadMode() },
	  _perfTimers { &perfTimers }
	{
		populateSurfaceEntity(renderer);
		populateGridEntity(renderer, _gridTop, 1.0f);
		populateGridEntity(renderer, _gridBottom, -1.0f);
		populateWireframeEntity(renderer);
		populateNormalEntity(renderer);

		eventRouter.addHandler(_rangeChangedHandler);
	}

	Graph(Graph&& other)
	: _function { std::move(other._function) },
	  _builder { std::move(other._builder) },
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
	  _rangeChangedHandler { std::move(other._rangeChangedHandler) }
	{
		_rangeChangedHandler._this = this;
	}

	auto& func() { return _function; }

	// Allow outside access to the surface material so the UI can
	// manipulate it.
	auto& surfaceMaterial() { return _surfaceMaterial; }

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
	void update();
	void updateSynchronized();
	void draw();
};
}
