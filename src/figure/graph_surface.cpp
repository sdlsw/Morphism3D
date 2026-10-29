#include "figure/graph_surface.h"

namespace g3d {
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
	_genericBuilder.generate2dLineIndices();
	_genericBuilder.generateTriangleIndices();
	_genericBuilder.generateNormalIndices();
}

std::vector<Color> SurfaceDisplay::makeGridColors(const GraphMeshBuilder& builder) {
	return { builder.pointCount(), {0.1f, 0.1f, 0.1f} };
}

std::vector<Color> SurfaceDisplay::makeNormalColors(const GraphMeshBuilder& builder) {
	return { 2*builder.pointCount(), {1.0f, 1.0f, 1.0f} };
}

void SurfaceDisplay::uploadPositions(const GraphMeshBuilder& builder) {
	_surfacePositions.copyData(builder.positions());
	_surfaceNormals.copyData(builder.normals());
}

void SurfaceDisplay::uploadColors(const GraphMeshBuilder& builder) {
	_surfaceColors.copyData(builder.colors());
	_gridColors.copyData(makeGridColors(builder));
	_normalColors.copyData(makeNormalColors(builder));
}

void SurfaceDisplay::uploadIndices(const GraphMeshBuilder& builder) {
	_surfaceIndices.copyData(builder.triangleIndices());
	_gridIndices.copyData(builder.lineIndices());
	_normalIndices.copyData(builder.normalIndices());
}

void SurfaceDisplay::populateSurfaceEntity(Renderer& renderer) {
	_surface.addComponent<TransformComponent>(renderer, Transform());

	_surface.addComponent<RenderModeComponent>(RenderMode::litTriangle);
	_surface.addComponent<DynamicVertexAttributeComponent<Position>>(_surfacePositions);
	_surface.addComponent<DynamicVertexAttributeComponent<Color>>(_surfaceColors);
	_surface.addComponent<DynamicVertexAttributeComponent<Normal>>(_surfaceNormals);
	_surface.addComponent<DynamicIndexBufferComponent>(_surfaceIndices);
	_surface.addComponent<MaterialComponent>(renderer, settings.material);

	_surface.setLastRender<DynamicIndexBufferComponent>();
}

void SurfaceDisplay::populateGridEntity(Renderer& renderer, Entity& ent, float loftMult) {
	populateDynamicLineEntity(
		renderer,
		ent,
		Transform({0.0f, 0.0f, loftMult*gridLoft}),
		_surfacePositions,
		_gridColors,
		_gridIndices
	);
}

void SurfaceDisplay::populateWireframeEntity(Renderer& renderer) {
	populateDynamicLineEntity(
		renderer,
		_wireframe,
		Transform(),
		_surfacePositions,
		_surfaceColors,
		_gridIndices
	);
}

void SurfaceDisplay::populateNormalEntity(Renderer& renderer) {
	populateDynamicLineEntity(
		renderer,
		_normals,
		Transform(),
		_surfacePositions,
		_normalColors,
		_normalIndices
	);
}

void SurfaceDisplay::draw() {
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
}
