#include "figure/graph_line.h"

namespace g3d {
void LineGraphMeshBuilder::regeneratePositions() {
	_genericBuilder.clearPositions();
	_genericBuilder.generatePositions<1, 2>(_func);
}

void LineGraphMeshBuilder::regenerateColors() {
	_genericBuilder.clearColors();
	_genericBuilder.generateColors<1>(_colors);
}

void LineGraphMeshBuilder::regenerateIndices() {
	_genericBuilder.clearIndices();
	_genericBuilder.generate1dLineIndices();
}

void LineDisplay::populateEntity(Renderer& renderer) {
	populateDynamicLineEntity(
		renderer,
		_line,
		Transform(),
		_positions,
		_colors,
		_indices
	);
}

void LineDisplay::uploadPositions(const GraphMeshBuilder& builder) {
	_positions.copyData(builder.positions());
}

void LineDisplay::uploadColors(const GraphMeshBuilder& builder) {
	_colors.copyData(builder.colors());
}

void LineDisplay::uploadIndices(const GraphMeshBuilder& builder) {
	_indices.copyData(builder.lineIndices());
}

void LineDisplay::draw() {
	_line.draw();
}
}
