#include "graph.h"

namespace g3d {
glm::vec3 GraphMeshBuilder::toModelSpace(const glm::vec3& funcSpace) const {
	// Simplified from (funcSpace - (H+L)/2) / ((H - L)/2), that
	// is, a translation followed by a scale.
	return (2.0f*funcSpace - rangeHigh - rangeLow) / (rangeHigh - rangeLow);
}

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
	_perfTimers->start("regenPositions");
	float inc = 1.0f / static_cast<float>(cells);
	for (unsigned int ypt = 0; ypt <= cells; ypt++) {
		float y = glm::mix(rangeLow.y, rangeHigh.y, inc*ypt);

		for (unsigned int xpt = 0; xpt <= cells; xpt++) {
			float x = glm::mix(rangeLow.x, rangeHigh.x, inc*xpt);
			_positions.push_back(toModelSpace({x, y, _func->eval(x, y)}));

			if (clampZ) {
				auto& p = _positions.back().vec;
				p.z = glm::clamp(p.z, -1.0f, 1.0f);
			}
		}
	}
	_perfTimers->stop("regenPositions");
}

void GraphMeshBuilder::generateColors() {
	// Colors for extreme points of graph.
	// nxny - (-range, -range)
	// pxny - (range, -range)
	// nxpy - (-range, range)
	// pxpy - (range, range)
	glm::vec3 nxny {0.141f, 0.706f, 0.322f}; // green
	glm::vec3 pxny {0.988f, 0.804f, 0.000f}; // yellow orange
	glm::vec3 nxpy {0.400f, 0.255f, 0.953f}; // blue violet
	glm::vec3 pxpy {1.000f, 0.000f, 0.000f}; // red

	float inc = 1.0f / static_cast<float>(cells);
	for (unsigned int ypt = 0; ypt <= cells; ypt++) {
		float lerp_a_y = inc*ypt;
		glm::vec3 colornx = glm::mix(nxny, nxpy, lerp_a_y);
		glm::vec3 colorpx = glm::mix(pxny, pxpy, lerp_a_y);

		for (unsigned int xpt = 0; xpt <= cells; xpt++) {
			float lerp_a_x = inc*xpt;
			glm::vec3 color = glm::mix(colornx, colorpx, lerp_a_x);
			_colors.push_back(color);
		}
	}
}

void GraphMeshBuilder::generateNormals() {
	_perfTimers->start("regenNormals");
	autoGenerateNormals(_normals, _positions, _triangleIndices);
	_perfTimers->stop("regenNormals");
}

void GraphMeshBuilder::generateNormalPositions() {
	_perfTimers->start("regenNormalPositions");
	auto ptCount = pointCount();
	for (unsigned int i = 0; i < ptCount; i++) {
		// Note: The positions for visualizing the normals are
		// tacked onto the end of the surface positions, to
		// save some bandwidth when copying the positions to
		// the GPU.
		_positions.push_back(_positions[i].vec + normLength*_normals[i].vec);
	}
	_perfTimers->stop("regenNormalPositions");
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

void GraphMeshBuilder::regenerateVertices() {
	regeneratePositions();
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

glm::vec3 GraphMeshBuilder::origin() const {
	return toModelSpace({0, 0, 0});
}

void GraphMeshBuilder::regeneratePositions() {
	_positions.clear();
	_normals.clear();
	generatePositions();
	generateNormals();
	generateNormalPositions();
}

void GraphMeshBuilder::regenerateEverything() {
	// Indices need to be regenerated first, since the
	// generateNormals() is dependent on them.
	regenerateIndices();
	regenerateVertices();
}

void Graph::populateSurfaceEntity(Renderer& renderer) {
	_surface.addComponent<TransformComponent>(renderer, Transform());

	_surface.addComponent<RenderModeComponent>(RenderMode::litTriangle);
	_surface.addComponent<DynamicVertexAttributeComponent<Position>>(_surfacePositions);
	_surface.addComponent<DynamicVertexAttributeComponent<Color>>(_surfaceColors);
	_surface.addComponent<DynamicVertexAttributeComponent<Normal>>(_surfaceNormals);
	_surface.addComponent<DynamicIndexBufferComponent>(_surfaceIndices);
	_surface.addComponent<MaterialComponent>(renderer, _surfaceMaterial);

	_surface.setLastRender<DynamicIndexBufferComponent>();
}

void Graph::populateGridEntity(Renderer& renderer, Entity& ent, float loftMult) {
	ent.addComponent<TransformComponent>(renderer, Transform({0.0f, 0.0f, loftMult*gridLoft}));

	ent.addComponent<RenderModeComponent>(RenderMode::line);
	ent.addComponent<DynamicVertexAttributeComponent<Position>>(_surfacePositions);
	ent.addComponent<DynamicVertexAttributeComponent<Color>>(_gridColors);
	ent.addComponent<DynamicIndexBufferComponent>(_gridIndices);

	ent.setLastRender<DynamicIndexBufferComponent>();
}

void Graph::populateWireframeEntity(Renderer& renderer) {
	_wireframe.addComponent<TransformComponent>(renderer, Transform());

	_wireframe.addComponent<RenderModeComponent>(RenderMode::line);
	_wireframe.addComponent<DynamicVertexAttributeComponent<Position>>(_surfacePositions);
	_wireframe.addComponent<DynamicVertexAttributeComponent<Color>>(_surfaceColors);
	_wireframe.addComponent<DynamicIndexBufferComponent>(_gridIndices);

	_wireframe.setLastRender<DynamicIndexBufferComponent>();
}

void Graph::populateNormalEntity(Renderer& renderer) {
	_normals.addComponent<TransformComponent>(renderer, Transform());

	_normals.addComponent<RenderModeComponent>(RenderMode::line);
	_normals.addComponent<DynamicVertexAttributeComponent<Position>>(_surfacePositions);
	_normals.addComponent<DynamicVertexAttributeComponent<Color>>(_normalColors);
	_normals.addComponent<DynamicIndexBufferComponent>(_normalIndices);

	_normals.setLastRender<DynamicIndexBufferComponent>();
}

std::vector<Color> Graph::makeGridColors() {
	return { _builder.pointCount(), {0.1f, 0.1f, 0.1f} };
}

std::vector<Color> Graph::makeNormalColors() {
	return { 2*_builder.pointCount(), {1.0f, 1.0f, 1.0f} };
}

void Graph::setRegenMode(GraphRegenMode mode) {
	if (temporaryRegen) return;
	_regenMode = mode;
}

void Graph::setUploadMode(GraphUploadMode mode) {
	// Ignore sets if in temporary mode so we don't accidentally
	// override updates.
	if (temporaryUploadFrames > 0) return;
	_uploadMode = mode;
}

GraphRegenMode Graph::defaultRegenMode() {
	if (_function.animated()) {
		return GraphRegenMode::partial;
	}

	return GraphRegenMode::none;
}

GraphUploadMode Graph::defaultUploadMode() {
	if (_function.animated()) {
		return GraphUploadMode::partial;
	}

	return GraphUploadMode::none;
}

void Graph::setTemporaryRegenMode(GraphRegenMode mode) {
	_regenMode = mode;
	temporaryRegen = true;
}

void Graph::setTemporaryUploadMode(GraphUploadMode mode) {
	_uploadMode = mode;
	temporaryUploadFrames = MAX_FRAMES_IN_FLIGHT;
}

void Graph::regen() {
	if (!doRegen) return;

	_perfTimers->start("regen");
	switch (_regenMode) {
		case GraphRegenMode::partial:
			_builder.regeneratePositions();
			break;
		case GraphRegenMode::all:
			_builder.regenerateEverything();
			break;
		default:
			break;
	}
	_perfTimers->stop("regen");
}

void Graph::uploadPartial() {
	_surfacePositions.copyData(_builder.positions());
	_surfaceNormals.copyData(_builder.normals());
}

void Graph::uploadAll() {
	uploadPartial();
	_surfaceColors.copyData(_builder.colors());
	_surfaceIndices.copyData(_builder.triangleIndices());
	_gridIndices.copyData(_builder.lineIndices());
	_gridColors.copyData(makeGridColors());
	_normalIndices.copyData(_builder.normalIndices());
	_normalColors.copyData(makeNormalColors());
}

void Graph::upload() {
	if (!doUpload) return;

	_perfTimers->start("upload");
	switch (_uploadMode) {
		case GraphUploadMode::partial:
			uploadPartial();
			break;
		case GraphUploadMode::all:
			uploadAll();
			break;
		default:
			break;
	}
	_perfTimers->stop("upload");
}

void Graph::clampZ(bool b) {
	_builder.clampZ = b;
	shouldUpdate = true;
}

bool Graph::clampZ() const {
	return _builder.clampZ;
}

void Graph::cells(unsigned int cells) {
	_builder.cells = cells;
	cellsChanged = true;
}

unsigned int Graph::cells() const {
	return _builder.cells;
}

void Graph::rangeLow(const glm::vec3& range) {
	_builder.rangeLow = range;
	shouldUpdate = true;
}

const glm::vec3& Graph::rangeLow() const {
	return _builder.rangeLow;
}

void Graph::rangeHigh(const glm::vec3& range) {
	_builder.rangeHigh = range;
	shouldUpdate = true;
}

const glm::vec3& Graph::rangeHigh() const {
	return _builder.rangeHigh;
}

glm::vec3 Graph::origin() const {
	return _builder.origin();
}

void Graph::update() {
	_function.update();
}

void Graph::updateSynchronized() {
	if (_function.updated()) {
		shouldUpdate = true;
		_function.resetUpdated();
	}

	if (shouldUpdate) {
		if (_function.animated()) {
			setRegenMode(defaultRegenMode());
			setUploadMode(defaultUploadMode());
		} else {
			setTemporaryRegenMode(GraphRegenMode::partial);
			setTemporaryUploadMode(GraphUploadMode::partial);
		}
		shouldUpdate = false;
	}

	// Cell update overrides normal update handling
	if (cellsChanged) {
		setTemporaryRegenMode(GraphRegenMode::all);
		setTemporaryUploadMode(GraphUploadMode::all);
		cellsChanged = false;
	}

	regen();
	upload();

	if (temporaryRegen) {
		temporaryRegen = false;
		_regenMode = defaultRegenMode();
	}

	if (temporaryUploadFrames > 0) {
		temporaryUploadFrames--;
		if (temporaryUploadFrames == 0) _uploadMode = defaultUploadMode();
	}
}

void Graph::draw() {
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
