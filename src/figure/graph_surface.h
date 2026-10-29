#pragma once

#include "figure/graph_common.h"

namespace g3d {
// Surface-specific regeneration.
class SurfaceGraphMeshBuilder {
private:
	static constexpr std::array<char, 2> inputVars { 'x', 'y' };

	Function<2, 1> _func;
	GraphColors<2> _colors;
	GraphMeshBuilder _genericBuilder;
public:
	SurfaceGraphMeshBuilder() = delete;
	SurfaceGraphMeshBuilder(
		VariableStore& variableStore,
		unsigned int cells,
		Range& range,
		TimerCollection& perfTimers,
		DiscriminatedStringMap& perfIds
	)
	: _func { variableStore, inputVars },
	  _colors { default2dColors() },
	  _genericBuilder { cells, range, perfTimers, perfIds }
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

	auto& colorsUpdated() { return _colors.colorChanged; }
	auto& funcUpdated() { return _func.updated(); }

	auto& func() { return _func; }
	const auto& func() const { return _func; }

	auto& colors() { return _colors; }
	const auto& colors() const { return _colors ; }
};

enum class SurfaceDisplayMode : int {
	surface = 0,
	wireframe,
	none
};

constexpr unsigned int SurfaceDisplayModeCount = 3;

struct SurfaceDisplaySettings {
	bool renderGrid = true;
	bool renderNormals = false;
	SurfaceDisplayMode renderMode = SurfaceDisplayMode::surface;
	WithInitial<Material> material = defaultMaterial();
};

class SurfaceDisplay {
private:
	static constexpr float gridLoft = 0.002f;

	DynamicVertexAttributes<Position> _surfacePositions;

	DynamicIndexBuffer _surfaceIndices;
	DynamicVertexAttributes<Color> _surfaceColors;
	DynamicVertexAttributes<Normal> _surfaceNormals;

	DynamicIndexBuffer _gridIndices;
	DynamicVertexAttributes<Color> _gridColors;

	DynamicIndexBuffer _normalIndices;
	DynamicVertexAttributes<Color> _normalColors;

	Entity _surface;
	Entity _gridTop;
	Entity _gridBottom;
	Entity _normals;
	Entity _wireframe;

	std::vector<Color> makeGridColors(const GraphMeshBuilder& builder);
	std::vector<Color> makeNormalColors(const GraphMeshBuilder& builder);

	void populateSurfaceEntity(Renderer& renderer);
	void populateGridEntity(Renderer& renderer, Entity& ent, float loftMult);
	void populateWireframeEntity(Renderer& renderer);
	void populateNormalEntity(Renderer& renderer);

public:
	SurfaceDisplaySettings settings;

	SurfaceDisplay(
		Renderer& renderer,
		const GraphMeshBuilder& builder
	)
	: _surfacePositions { renderer, builder.positions() },
	  _surfaceIndices { renderer, builder.triangleIndices() },
	  _surfaceColors { renderer, builder.colors() },
	  _surfaceNormals { renderer, builder.normals() },
	  _gridIndices { renderer, builder.lineIndices() },
	  _gridColors { renderer, makeGridColors(builder) },
	  _normalIndices { renderer, builder.normalIndices() },
	  _normalColors { renderer, makeNormalColors(builder) }
	{
		uploadPositions(builder);
		uploadColors(builder);
		uploadIndices(builder);

		populateSurfaceEntity(renderer);
		populateGridEntity(renderer, _gridTop, 1.0f);
		populateGridEntity(renderer, _gridBottom, -1.0f);
		populateWireframeEntity(renderer);
		populateNormalEntity(renderer);
	}

	void uploadPositions(const GraphMeshBuilder& builder);
	void uploadColors(const GraphMeshBuilder& builder);
	void uploadIndices(const GraphMeshBuilder& builder);

	void draw();
};

using SurfaceGraphFigure = GraphFigure<SurfaceGraphMeshBuilder, SurfaceDisplay>;
}
