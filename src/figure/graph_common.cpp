#include "figure/graph_common.h"

namespace g3d {
uint16_t GraphMeshBuilder::idx(unsigned int x, unsigned int y) {
	// Note: cells+1 here because there's one more point than the
	// number of cells, for instance:
	//
	//  _ _
	// |_|_|
	// |_|_|
	//
	// 2 cell grid, but 3 points.
	return y*(cells+1) + x;
}

const Position& GraphMeshBuilder::getPosition(unsigned int x, unsigned int y) {
	return _positions[idx(x, y)];
}

void GraphMeshBuilder::generateNormals() {
	_perfTimers->start((*_perfIds)["regenNormals"]);
	autoGenerateNormals(_normals, _positions, _triangleIndices);
	_perfTimers->stop((*_perfIds)["regenNormals"]);
}

void GraphMeshBuilder::generateNormalPositions() {
	_perfTimers->start((*_perfIds)["regenNormalPositions"]);
	auto ptCount = pointCount();
	for (unsigned int i = 0; i < ptCount; i++) {
		// Note: The positions for visualizing the normals are
		// tacked onto the end of the surface positions, to
		// save some bandwidth when copying the positions to
		// the GPU.
		_positions.push_back(_positions[i].vec + normLength*_normals[i].vec);
	}
	_perfTimers->stop((*_perfIds)["regenNormalPositions"]);
}

void GraphMeshBuilder::generate1dLineIndices() {
	for (unsigned int xpt = 0; xpt < cells; xpt++) {
		_lineIndices.push_back(xpt);
		_lineIndices.push_back(xpt+1);
	}
}

void GraphMeshBuilder::generate2dLineIndices() {
	// generate horizontal lines
	for (unsigned int ypt = 0; ypt <= cells; ypt++) {
		for (unsigned int xpt = 0; xpt < cells; xpt++) {
			_lineIndices.push_back(idx(xpt, ypt));
			_lineIndices.push_back(idx(xpt+1, ypt));
		}
	}

	// generate vertical lines
	for (unsigned int xpt = 0; xpt <= cells; xpt++) {
		for (unsigned int ypt = 0; ypt < cells; ypt++) {
			_lineIndices.push_back(idx(xpt, ypt));
			_lineIndices.push_back(idx(xpt, ypt+1));
		}
	}
}

void GraphMeshBuilder::generateTriangleIndices() {
	// X and Y correspond to the top left vertex of the quad being
	// generated.
	for (unsigned int ypt = 0; ypt < cells; ypt++) {
		for (unsigned int xpt = 0; xpt < cells; xpt++) {
			// first tri
			_triangleIndices.push_back(idx(xpt, ypt));
			_triangleIndices.push_back(idx(xpt+1, ypt));
			_triangleIndices.push_back(idx(xpt, ypt+1));

			// second tri
			_triangleIndices.push_back(idx(xpt+1, ypt));
			_triangleIndices.push_back(idx(xpt+1, ypt+1));
			_triangleIndices.push_back(idx(xpt, ypt+1));
		}
	}
}

void GraphMeshBuilder::generateNormalIndices() {
	auto ptCount = pointCount();

	for (unsigned int i = 0; i < ptCount; i++) {
		_normalIndices.push_back(i);
		_normalIndices.push_back(i + ptCount);
	}
}

void GraphMeshBuilder::clearPositions() {
	_positions.clear();
}

void GraphMeshBuilder::clearColors() {
	_colors.clear();
}

void GraphMeshBuilder::clearNormals() {
	_normals.clear();
}

void GraphMeshBuilder::clearIndices() {
	_lineIndices.clear();
	_triangleIndices.clear();
	_normalIndices.clear();
}

GraphColors<1> default1dColors() {
	glm::vec3 colors[] {
		{0.141f, 0.706f, 0.322f}, // green
		{1.000f, 0.000f, 0.000f}  // red
	};

	return { colors };
}

GraphColors<2> default2dColors() {
	glm::vec3 colors[] {
		{0.141f, 0.706f, 0.322f}, // green
		{0.988f, 0.804f, 0.000f}, // yellow orange
		{0.400f, 0.255f, 0.953f}, // blue violet
		{1.000f, 0.000f, 0.000f}  // red
	};

	return { colors };
}

void GraphState::setRegenMode(GraphUpdateMode mode) {
	inRegen.set();
	regenMode |= mode;
}

void GraphState::setUploadMode(GraphUpdateMode mode) {
	uploadFrames = MAX_FRAMES_IN_FLIGHT;
	uploadMode |= mode;
}

void GraphState::setUpdateMode(GraphUpdateMode mode) {
	setRegenMode(mode);
	setUploadMode(mode);
}

void GraphState::decRegen() {
	if (inRegen.consume()) {
		regenMode = GraphUpdateMode::none;
	}
}

void GraphState::decUpload() {
	if (uploadFrames > 0) {
		uploadFrames--;
		if (uploadFrames == 0) uploadMode = GraphUpdateMode::none;
	}
}

void GraphState::update(Flag& functionChanged, Flag& colorsChanged) {
	shouldRegenPositions.setByConsuming(functionChanged);

	if (shouldRegenPositions.consume()) {
		setUpdateMode(GraphUpdateMode::positions);
	}

	if (colorsChanged.consume()) {
		setUpdateMode(GraphUpdateMode::colors);
	}

	if (cellsChanged.consume()) {
		setUpdateMode(allEnabled<GraphUpdateMode>());
	}
}

void GraphState::updateEnd() {
	decRegen();
	decUpload();
}
}
