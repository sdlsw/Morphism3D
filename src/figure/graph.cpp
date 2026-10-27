#include "figure/graph.h"

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

void GraphMeshBuilder::generateLineIndices() {
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

GraphColors<2> default2dColors() {
	glm::vec3 colors[] {
		{0.141f, 0.706f, 0.322f}, // green
		{0.988f, 0.804f, 0.000f}, // yellow orange
		{0.400f, 0.255f, 0.953f}, // blue violet
		{1.000f, 0.000f, 0.000f}  // red
	};

	return { colors };
}

void SurfaceGraphMeshBuilder::regeneratePositions() {
	_genericBuilder.clearPositions();
	_genericBuilder.clearNormals();
	_genericBuilder.generatePositions<2, 1>(_func);
	_genericBuilder.generateNormals();
	_genericBuilder.generateNormalPositions();
}

void SurfaceGraphMeshBuilder::regenerateColors() {
	_genericBuilder.clearColors();
	_genericBuilder.generateColors<2>(_colors);
}

void SurfaceGraphMeshBuilder::regenerateIndices() {
	_genericBuilder.clearIndices();
	_genericBuilder.generateLineIndices();
	_genericBuilder.generateTriangleIndices();
	_genericBuilder.generateNormalIndices();
}

std::vector<Color> SurfaceDisplayBuffers::makeGridColors(const GraphMeshBuilder& builder) {
	return { builder.pointCount(), {0.1f, 0.1f, 0.1f} };
}

std::vector<Color> SurfaceDisplayBuffers::makeNormalColors(const GraphMeshBuilder& builder) {
	return { 2*builder.pointCount(), {1.0f, 1.0f, 1.0f} };
}

void SurfaceDisplayBuffers::uploadPositions(const GraphMeshBuilder& builder) {
	surfacePositions.copyData(builder.positions());
	surfaceNormals.copyData(builder.normals());
}

void SurfaceDisplayBuffers::uploadColors(const GraphMeshBuilder& builder) {
	surfaceColors.copyData(builder.colors());
	gridColors.copyData(makeGridColors(builder));
	normalColors.copyData(makeNormalColors(builder));
}

void SurfaceDisplayBuffers::uploadIndices(const GraphMeshBuilder& builder) {
	surfaceIndices.copyData(builder.triangleIndices());
	gridIndices.copyData(builder.lineIndices());
	normalIndices.copyData(builder.normalIndices());
}

void SurfaceDisplayEntities::populateSurfaceEntity(
	Renderer& renderer,
	SurfaceDisplayBuffers& buffers,
	Material& material
) {
	_surface.addComponent<TransformComponent>(renderer, Transform());

	_surface.addComponent<RenderModeComponent>(RenderMode::litTriangle);
	_surface.addComponent<DynamicVertexAttributeComponent<Position>>(buffers.surfacePositions);
	_surface.addComponent<DynamicVertexAttributeComponent<Color>>(buffers.surfaceColors);
	_surface.addComponent<DynamicVertexAttributeComponent<Normal>>(buffers.surfaceNormals);
	_surface.addComponent<DynamicIndexBufferComponent>(buffers.surfaceIndices);
	_surface.addComponent<MaterialComponent>(renderer, material);

	_surface.setLastRender<DynamicIndexBufferComponent>();
}

void SurfaceDisplayEntities::populateGridEntity(
	Renderer& renderer,
	SurfaceDisplayBuffers& buffers,
	Entity& ent,
	float loftMult
) {
	populateDynamicLineEntity(
		renderer,
		ent,
		Transform({0.0f, 0.0f, loftMult*gridLoft}),
		buffers.surfacePositions,
		buffers.gridColors,
		buffers.gridIndices
	);
}

void SurfaceDisplayEntities::populateWireframeEntity(Renderer& renderer, SurfaceDisplayBuffers& buffers) {
	populateDynamicLineEntity(
		renderer,
		_wireframe,
		Transform(),
		buffers.surfacePositions,
		buffers.surfaceColors,
		buffers.gridIndices
	);
}

void SurfaceDisplayEntities::populateNormalEntity(Renderer& renderer, SurfaceDisplayBuffers& buffers) {
	populateDynamicLineEntity(
		renderer,
		_normals,
		Transform(),
		buffers.surfacePositions,
		buffers.normalColors,
		buffers.normalIndices
	);
}

void SurfaceDisplayEntities::draw() {
	// TODO This is really dumb but it doesn't look too terrible...
	// Look into using textures for the grid, maybe.
	if (settings.renderGrid && settings.renderMode == SurfaceDisplayMode::surface) {
		_gridTop.draw();
		_gridBottom.draw();
	}

	if (settings.renderNormals) {
		_normals.draw();
	}

	if (settings.renderMode == SurfaceDisplayMode::wireframe) {
		_wireframe.draw();
	}

	if (settings.renderMode == SurfaceDisplayMode::surface) {
		_surface.draw();
	}
}

void SurfaceGraphState::setRegenMode(GraphUpdateMode mode) {
	inRegen.set();
	regenMode |= mode;
}

void SurfaceGraphState::setUploadMode(GraphUpdateMode mode) {
	uploadFrames = MAX_FRAMES_IN_FLIGHT;
	uploadMode |= mode;
}

void SurfaceGraphState::setUpdateMode(GraphUpdateMode mode) {
	setRegenMode(mode);
	setUploadMode(mode);
}

void SurfaceGraphState::decRegen() {
	if (inRegen.consume()) {
		regenMode = GraphUpdateMode::none;
	}
}

void SurfaceGraphState::decUpload() {
	if (uploadFrames > 0) {
		uploadFrames--;
		if (uploadFrames == 0) uploadMode = GraphUpdateMode::none;
	}
}

void SurfaceGraphState::updateEnd() {
	decRegen();
	decUpload();
}

void SurfaceGraphFigure::handleRangeChanged(const RangeChangedEvent& e) {
	_state.shouldRegenPositions.set();
}

void SurfaceGraphFigure::handleFigureRemoved(const FigureRemovedEvent& e) {
	if (e.id != _id) return;

	for (const auto& [_, perfId] : _perfIds.cache()) {
		_perfTimers->removeTimer(perfId);
	}
}

void SurfaceGraphFigure::regen() {
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

void SurfaceGraphFigure::upload() {
	if (!doUpload) return;

	_perfTimers->start(_perfIds["upload"]);
	if (flagsEnabled(_state.uploadMode, GraphUpdateMode::positions)) {
		_buffers.uploadPositions(_builder.genericBuilder());
	}

	if (flagsEnabled(_state.uploadMode, GraphUpdateMode::colors)) {
		_buffers.uploadColors(_builder.genericBuilder());
	}

	if (flagsEnabled(_state.uploadMode, GraphUpdateMode::indices)) {
		_buffers.uploadIndices(_builder.genericBuilder());
	}
	_perfTimers->stop(_perfIds["upload"]);
}

void SurfaceGraphFigure::clampOut(bool b) {
	_builder.genericBuilder().clampOut = b;
	_state.shouldRegenPositions.set();
}

bool SurfaceGraphFigure::clampOut() const {
	return _builder.genericBuilder().clampOut;
}

void SurfaceGraphFigure::cells(unsigned int cells) {
	_builder.genericBuilder().cells = cells;
	_state.cellsChanged.set();
}

unsigned int SurfaceGraphFigure::cells() const {
	return _builder.genericBuilder().cells;
}

void SurfaceGraphFigure::updateSynchronized() {
	_perfTimers->start(_perfIds["updateSynchronized"]);
	_state.update(_builder.func(), _builder.colors());
	regen();
	upload();
	_state.updateEnd();
	_perfTimers->stop(_perfIds["updateSynchronized"]);
}

void SurfaceGraphFigure::draw() {
	_entities.draw();
}
}
