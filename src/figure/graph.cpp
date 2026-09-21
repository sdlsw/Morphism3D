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

void GraphMeshBuilder::generatePositions() {
	_perfTimers->start((*_perfIds)["regenPositions"]);
	float inc = 1.0f / static_cast<float>(cells);
	for (unsigned int ypt = 0; ypt <= cells; ypt++) {
		float y = glm::mix(_range->low().y, _range->high().y, inc*ypt);

		for (unsigned int xpt = 0; xpt <= cells; xpt++) {
			float x = glm::mix(_range->low().x, _range->high().x, inc*xpt);
			_positions.push_back(_range->toModelSpace({x, y, _func->eval(x, y)}));

			if (clampZ) {
				auto& p = _positions.back().vec;
				p.z = glm::clamp(p.z, -1.0f, 1.0f);
			}
		}
	}
	_perfTimers->stop((*_perfIds)["regenPositions"]);
}

void GraphMeshBuilder::generateColors() {
	float inc = 1.0f / static_cast<float>(cells);
	for (unsigned int ypt = 0; ypt <= cells; ypt++) {
		float lerp_a_y = inc*ypt;
		auto colornx = _appearance->colornx(lerp_a_y);
		auto colorpx = _appearance->colorpx(lerp_a_y);

		for (unsigned int xpt = 0; xpt <= cells; xpt++) {
			float lerp_a_x = inc*xpt;
			glm::vec3 color = glm::mix(colornx, colorpx, lerp_a_x);
			_colors.push_back(color);
		}
	}
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

void GraphMeshBuilder::regeneratePositions() {
	_positions.clear();
	_normals.clear();
	generatePositions();
	generateNormals();
	generateNormalPositions();
}

void GraphMeshBuilder::regenerateColors() {
	_colors.clear();
	generateColors();
}

void GraphMeshBuilder::regenerateIndices() {
	_lineIndices.clear();
	_triangleIndices.clear();
	_normalIndices.clear();

	generateLineIndices();
	generateTriangleIndices();
	generateNormalIndices();
}

void GraphMeshBuilder::regenerate(const GraphUpdateMode& mode) {
	// Indices need to be regenerated first, since the
	// generateNormals() is dependent on them.
	if (flagsEnabled(mode, GraphUpdateMode::indices)) {
		regenerateIndices();
	}

	if (flagsEnabled(mode, GraphUpdateMode::colors)) {
		regenerateColors();
	}

	if (flagsEnabled(mode, GraphUpdateMode::positions)) {
		regeneratePositions();
	}
}

void GraphFigure::handleRangeChanged(const RangeChangedEvent& e) {
	shouldUpdate.set();
}

void GraphFigure::handleFigureRemoved(const FigureRemovedEvent& e) {
	if (e.id != _id) return;

	for (const auto& [_, perfId] : _perfIds.cache()) {
		_perfTimers->removeTimer(perfId);
	}
}

void GraphFigure::populateSurfaceEntity(Renderer& renderer) {
	_surface.addComponent<TransformComponent>(renderer, Transform());

	_surface.addComponent<RenderModeComponent>(RenderMode::litTriangle);
	_surface.addComponent<DynamicVertexAttributeComponent<Position>>(_surfacePositions);
	_surface.addComponent<DynamicVertexAttributeComponent<Color>>(_surfaceColors);
	_surface.addComponent<DynamicVertexAttributeComponent<Normal>>(_surfaceNormals);
	_surface.addComponent<DynamicIndexBufferComponent>(_surfaceIndices);
	_surface.addComponent<MaterialComponent>(renderer, _appearance.material);

	_surface.setLastRender<DynamicIndexBufferComponent>();
}

void GraphFigure::populateGridEntity(Renderer& renderer, Entity& ent, float loftMult) {
	ent.addComponent<TransformComponent>(renderer, Transform({0.0f, 0.0f, loftMult*gridLoft}));

	ent.addComponent<RenderModeComponent>(RenderMode::line);
	ent.addComponent<DynamicVertexAttributeComponent<Position>>(_surfacePositions);
	ent.addComponent<DynamicVertexAttributeComponent<Color>>(_gridColors);
	ent.addComponent<DynamicIndexBufferComponent>(_gridIndices);

	ent.setLastRender<DynamicIndexBufferComponent>();
}

void GraphFigure::populateWireframeEntity(Renderer& renderer) {
	_wireframe.addComponent<TransformComponent>(renderer, Transform());

	_wireframe.addComponent<RenderModeComponent>(RenderMode::line);
	_wireframe.addComponent<DynamicVertexAttributeComponent<Position>>(_surfacePositions);
	_wireframe.addComponent<DynamicVertexAttributeComponent<Color>>(_surfaceColors);
	_wireframe.addComponent<DynamicIndexBufferComponent>(_gridIndices);

	_wireframe.setLastRender<DynamicIndexBufferComponent>();
}

void GraphFigure::populateNormalEntity(Renderer& renderer) {
	_normals.addComponent<TransformComponent>(renderer, Transform());

	_normals.addComponent<RenderModeComponent>(RenderMode::line);
	_normals.addComponent<DynamicVertexAttributeComponent<Position>>(_surfacePositions);
	_normals.addComponent<DynamicVertexAttributeComponent<Color>>(_normalColors);
	_normals.addComponent<DynamicIndexBufferComponent>(_normalIndices);

	_normals.setLastRender<DynamicIndexBufferComponent>();
}

std::vector<Color> GraphFigure::makeGridColors() {
	return { _builder.pointCount(), {0.1f, 0.1f, 0.1f} };
}

std::vector<Color> GraphFigure::makeNormalColors() {
	return { 2*_builder.pointCount(), {1.0f, 1.0f, 1.0f} };
}

void GraphFigure::setRegenMode(GraphUpdateMode mode) {
	inRegen.set();
	_regenMode |= mode;
}

void GraphFigure::setUploadMode(GraphUpdateMode mode) {
	uploadFrames = MAX_FRAMES_IN_FLIGHT;
	_uploadMode |= mode;
}

void GraphFigure::setUpdateMode(GraphUpdateMode mode) {
	setRegenMode(mode);
	setUploadMode(mode);
}

void GraphFigure::regen() {
	if (!doRegen) return;

	_perfTimers->start(_perfIds["regen"]);
	_builder.regenerate(_regenMode);
	_perfTimers->stop(_perfIds["regen"]);
}

void GraphFigure::uploadPositions() {
	_surfacePositions.copyData(_builder.positions());
	_surfaceNormals.copyData(_builder.normals());
}

void GraphFigure::uploadColors() {
	_surfaceColors.copyData(_builder.colors());
	_gridColors.copyData(makeGridColors());
	_normalColors.copyData(makeNormalColors());
}

void GraphFigure::uploadIndices() {
	_surfaceIndices.copyData(_builder.triangleIndices());
	_gridIndices.copyData(_builder.lineIndices());
	_normalIndices.copyData(_builder.normalIndices());
}

void GraphFigure::upload() {
	if (!doUpload) return;

	_perfTimers->start(_perfIds["upload"]);

	if (flagsEnabled(_uploadMode, GraphUpdateMode::positions)) {
		uploadPositions();
	}

	if (flagsEnabled(_uploadMode, GraphUpdateMode::colors)) {
		uploadColors();
	}

	if (flagsEnabled(_uploadMode, GraphUpdateMode::indices)) {
		uploadIndices();
	}

	_perfTimers->stop(_perfIds["upload"]);
}

void GraphFigure::clampZ(bool b) {
	_builder.clampZ = b;
	shouldUpdate = true;
}

bool GraphFigure::clampZ() const {
	return _builder.clampZ;
}

void GraphFigure::cells(unsigned int cells) {
	_builder.cells = cells;
	cellsChanged.set();
}

unsigned int GraphFigure::cells() const {
	return _builder.cells;
}

void GraphFigure::updateSynchronized() {
	_perfTimers->start(_perfIds["updateSynchronized"]);
	shouldUpdate.setByConsuming(_function.updated());

	if (shouldUpdate.consume()) {
		setUpdateMode(GraphUpdateMode::positions);
	}

	if (_appearance.colorChanged.consume()) {
		setUpdateMode(GraphUpdateMode::colors);
	}

	if (cellsChanged.consume()) {
		setUpdateMode(allEnabled<GraphUpdateMode>());
	}

	regen();
	upload();

	if (inRegen.consume()) {
		_regenMode = GraphUpdateMode::none;
	}

	if (uploadFrames > 0) {
		uploadFrames--;
		if (uploadFrames == 0) _uploadMode = GraphUpdateMode::none;
	}
	_perfTimers->stop(_perfIds["updateSynchronized"]);
}

void GraphFigure::draw() {
	// TODO This is really dumb but it doesn't look too terrible...
	// Look into using textures for the grid, maybe.
	if (renderGrid && renderMode == GraphRenderMode::surface) {
		_gridTop.draw();
		_gridBottom.draw();
	}

	if (renderNormals) {
		_normals.draw();
	}

	if (renderMode == GraphRenderMode::wireframe) {
		_wireframe.draw();
	}

	if (renderMode == GraphRenderMode::surface) {
		_surface.draw();
	}
}
}
