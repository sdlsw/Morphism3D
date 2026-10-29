#pragma once

#include "figure/graph_common.h"

namespace g3d {
class LineGraphMeshBuilder {
private:
	static constexpr std::array<char, 1> inputVars { 'x' };

	Function<1, 2> _func;
	GraphColors<1> _colors;
	GraphMeshBuilder _genericBuilder;
public:
	LineGraphMeshBuilder() = delete;
	LineGraphMeshBuilder(
		VariableStore& variableStore,
		unsigned int cells,
		Range& range,
		TimerCollection& perfTimers,
		DiscriminatedStringMap& perfIds
	)
	: _func { variableStore, inputVars },
	  _colors { default1dColors() },
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

class LineDisplay {
private:
	DynamicVertexAttributes<Position> _positions;
	DynamicVertexAttributes<Color> _colors;
	DynamicIndexBuffer _indices;

	Entity _line;

	void populateEntity(Renderer& renderer);

public:
	LineDisplay(
		Renderer& renderer,
		const GraphMeshBuilder& builder
	)
	: _positions { renderer, builder.positions() },
	  _colors { renderer, builder.colors() },
	  _indices { renderer, builder.lineIndices() }
	{
		populateEntity(renderer);
	}

	void uploadPositions(const GraphMeshBuilder& builder);
	void uploadColors(const GraphMeshBuilder& builder);
	void uploadIndices(const GraphMeshBuilder& builder);

	void draw();
};

using LineGraphFigure = GraphFigure<LineGraphMeshBuilder, LineDisplay>;
}
