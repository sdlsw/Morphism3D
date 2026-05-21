#pragma once

#include "container.h"
#include "primitive.h"
#include "vk/renderer.h"

#include <concepts>
#include <vector>

namespace g3d {
template<typename F>
concept Graphable = requires(F f, float x, float y) {
	{ f.eval(x, y) } -> std::convertible_to<float>;
};

template<Graphable F>
class GraphMeshBuilder {
private:
	static constexpr float normLength = 0.1f;

	// The number of discrete steps to walk along each input variable.
	// Higher values yield higher accuracy at the cost of additional
	// vertices.
	unsigned int _cells;

	// When the cells value changes, the buffers need to be updated for
	// multiple frames since everything is double buffered.
	unsigned int _cellsChangedFrames = 0;

	// The x, y coordinates of the model vertices always range from
	// -1.0f to 1.0f. The range determines how those coordinates map to
	// function input space with a simple relation:
	//
	//     (model_x, model_y, model_z)*range = (graph_x, graph_y, f(graph_x, graph_y))
	//		
	// graph_x and graph_y will both range from -range to +range.
	float _range;
	const F* _func;

	// TODO Store Positions in func space coordinates and convert to model
	// space when constructing entities
	std::vector<Position> _positions;
	std::vector<Color> _colors;
	std::vector<Normal> _normals;

	std::vector<uint16_t> _triangleIndices;
	std::vector<uint16_t> _lineIndices;
	std::vector<uint16_t> _normalIndices;

	glm::vec3 toModelSpace(const glm::vec3& funcSpace) {
		return funcSpace / _range;
	}

	uint16_t idx(unsigned int x, unsigned int y) {
		// Note: cells+1 here because there's one more point than the
		// number of cells, for instance:
		//
		//  _ _
		// |_|_|
		// |_|_|
		//
		// 2 cell grid, but 3 points.
		return y*(_cells+1) + x;
	}

	const Position& getPosition(unsigned int x, unsigned int y) {
		return _positions[idx(x, y)];
	}

	void generatePositions() {
		float inc = 1.0f / static_cast<float>(_cells);
		for (unsigned int ypt = 0; ypt <= _cells; ypt++) {
			float y = glm::mix(-_range, _range, inc*ypt);

			for (unsigned int xpt = 0; xpt <= _cells; xpt++) {
				float x = glm::mix(-_range, _range, inc*xpt);
				_positions.push_back(toModelSpace({x, y, _func->eval(x, y)}));
			}
		}
	}

	void generateColors() {
		// Colors for extreme points of graph.
		// nxny - (-range, -range)
		// pxny - (range, -range)
		// nxpy - (-range, range)
		// pxpy - (range, range)
		glm::vec3 nxny {0.141f, 0.706f, 0.322f}; // green
		glm::vec3 pxny {0.988f, 0.804f, 0.000f}; // yellow orange
		glm::vec3 nxpy {0.400f, 0.255f, 0.953f}; // blue violet
		glm::vec3 pxpy {1.000f, 0.000f, 0.000f}; // red

		float inc = 1.0f / static_cast<float>(_cells);
		for (unsigned int ypt = 0; ypt <= _cells; ypt++) {
			float lerp_a_y = inc*ypt;
			float y = glm::mix(-_range, _range, lerp_a_y);

			glm::vec3 colornx = glm::mix(nxny, nxpy, lerp_a_y);
			glm::vec3 colorpx = glm::mix(pxny, pxpy, lerp_a_y);

			for (unsigned int xpt = 0; xpt <= _cells; xpt++) {
				float lerp_a_x = inc*xpt;
				glm::vec3 color = glm::mix(colornx, colorpx, lerp_a_x);
				_colors.push_back(color);
			}
		}
	}

	void generateNormals() {
		autoGenerateNormals(_normals, _positions, _triangleIndices);
	}

	void generateNormalPositions() {
		auto ptCount = pointCount();
		for (unsigned int i = 0; i < ptCount; i++) {
			// Note: The positions for visualizing the normals are
			// tacked onto the end of the surface positions, to
			// save some bandwidth when copying the positions to
			// the GPU.
			_positions.push_back(_positions[i].vec + normLength*_normals[i].vec);
		}
	}

	void generateLineIndices() {
		// generate horizontal lines
		for (unsigned int ypt = 0; ypt <= _cells; ypt++) {
			for (unsigned int xpt = 0; xpt < _cells; xpt++) {
				_lineIndices.push_back(idx(xpt, ypt));
				_lineIndices.push_back(idx(xpt+1, ypt));
			}
		}

		// generate vertical lines
		for (unsigned int xpt = 0; xpt <= _cells; xpt++) {
			for (unsigned int ypt = 0; ypt < _cells; ypt++) {
				_lineIndices.push_back(idx(xpt, ypt));
				_lineIndices.push_back(idx(xpt, ypt+1));
			}
		}
	}

	void generateTriangleIndices() {
		// X and Y correspond to the top left vertex of the quad being
		// generated.
		for (unsigned int ypt = 0; ypt < _cells; ypt++) {
			for (unsigned int xpt = 0; xpt < _cells; xpt++) {
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

	void generateNormalIndices() {
		auto ptCount = pointCount();

		for (unsigned int i = 0; i < ptCount; i++) {
			_normalIndices.push_back(i);
			_normalIndices.push_back(i + ptCount);
		}
	}
public:
	GraphMeshBuilder() = delete;
	GraphMeshBuilder(const F& f, unsigned int cells, float range)
	: _func { &f },
	  _cells { cells },
	  _range { range }
	{
		regenerateEverything();
	}

	auto cells() const { return _cells; }
	void cells(unsigned int cells) { _cells = cells; }

	auto range() const { return _range; }
	const auto& positions() const { return _positions; }
	const auto& colors() const { return _colors; }
	const auto& normals() const { return _normals; }
	const auto& lineIndices() const { return _lineIndices; }
	const auto& triangleIndices() const { return _triangleIndices; }
	const auto& normalIndices() const { return _normalIndices; }
	auto pointCount() const { return (_cells + 1) * (_cells + 1); }

	void regeneratePositions() {
		_positions.clear();
		_normals.clear();
		generatePositions();
		generateNormals();
		generateNormalPositions();
	}

	void regenerateVertices() {
		regeneratePositions();
		_colors.clear();
		generateColors();
	}

	void regenerateIndices() {
		_lineIndices.clear();
		_triangleIndices.clear();
		_normalIndices.clear();

		generateLineIndices();
		generateTriangleIndices();
		generateNormalIndices();
	}

	void regenerateEverything() {
		// Indices need to be regenerated first, since the
		// generateNormals() is dependent on them.
		regenerateIndices();
		regenerateVertices();
	}
};

template<Graphable F>
class Graph {
private:
	static constexpr float gridLoft = 0.002f;

	GraphMeshBuilder<F> _builder;
	bool cellsChanged = false;

	// When the cells value changes, the buffers need to be updated for
	// multiple frames since everything is double buffered.
	unsigned int cellChangeUploadFrames = 0;

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

	void populateSurfaceEntity(Renderer& renderer) {
		_surface.addComponent<TransformComponent>(renderer, Transform());

		_surface.addComponent<DynamicVertexAttributeComponent<Position>>(_surfacePositions);
		_surface.addComponent<DynamicVertexAttributeComponent<Color>>(_surfaceColors);
		_surface.addComponent<DynamicVertexAttributeComponent<Normal>>(_surfaceNormals);
		_surface.addComponent<DynamicIndexBufferComponent>(_surfaceIndices);
		_surface.addComponent<MaterialComponent>(renderer, _surfaceMaterial);

		_surface.setLastRender<DynamicIndexBufferComponent>();
	}

	void populateGridEntity(Renderer& renderer, Entity& ent, float loftMult) {
		ent.addComponent<TransformComponent>(renderer, Transform({0.0f, 0.0f, loftMult*gridLoft}));

		ent.addComponent<DynamicVertexAttributeComponent<Position>>(_surfacePositions);
		ent.addComponent<DynamicVertexAttributeComponent<Color>>(_gridColors);
		ent.addComponent<DynamicIndexBufferComponent>(_gridIndices);

		ent.setLastRender<DynamicIndexBufferComponent>();
	}

	void populateWireframeEntity(Renderer& renderer) {
		_wireframe.addComponent<TransformComponent>(renderer, Transform());

		_wireframe.addComponent<DynamicVertexAttributeComponent<Position>>(_surfacePositions);
		_wireframe.addComponent<DynamicVertexAttributeComponent<Color>>(_surfaceColors);
		_wireframe.addComponent<DynamicIndexBufferComponent>(_gridIndices);

		_wireframe.setLastRender<DynamicIndexBufferComponent>();
	}

	void populateNormalEntity(Renderer& renderer) {
		_normals.addComponent<TransformComponent>(renderer, Transform());

		_normals.addComponent<DynamicVertexAttributeComponent<Position>>(_surfacePositions);
		_normals.addComponent<DynamicVertexAttributeComponent<Color>>(_normalColors);
		_normals.addComponent<DynamicIndexBufferComponent>(_normalIndices);

		_normals.setLastRender<DynamicIndexBufferComponent>();
	}

	std::vector<Color> makeGridColors() {
		return { _builder.pointCount(), {0.1f, 0.1f, 0.1f} };
	}

	std::vector<Color> makeNormalColors() {
		return { 2*_builder.pointCount(), {1.0f, 1.0f, 1.0f} };
	}
public:
	bool doUpload = true;
	bool doRegen = true;

	Graph(
		Renderer& renderer,
		const F& func,
		unsigned int cells,
		float range
	)
	: _builder { func, cells, range },
	  _surfacePositions { renderer, _builder.positions() },
	  _surfaceColors { renderer, _builder.colors() },
	  _surfaceNormals { renderer, _builder.normals() },
	  _surfaceIndices { renderer, _builder.triangleIndices() },
	  _gridIndices { renderer, _builder.lineIndices() },
	  _gridColors { renderer, makeGridColors() },
	  _normalIndices { renderer, _builder.normalIndices() },
	  _normalColors { renderer, makeNormalColors() }
	{
		populateSurfaceEntity(renderer);
		populateGridEntity(renderer, _gridTop, 1.0f);
		populateGridEntity(renderer, _gridBottom, -1.0f);
		populateWireframeEntity(renderer);
		populateNormalEntity(renderer);
	}

	// Allow outside access to the surface material so the UI can
	// manipulate it.
	auto& surfaceMaterial() { return _surfaceMaterial; }

	auto& surface() { return _surface; }
	auto& gridTop() { return _gridTop; }
	auto& gridBottom() { return _gridBottom; }
	auto& normals() { return _normals; }
	auto& wireframe() { return _wireframe; }

	void cells(unsigned int cells) {
		_builder.cells(cells);
		cellsChanged = true;
		cellChangeUploadFrames = MAX_FRAMES_IN_FLIGHT;
	}

	unsigned int cells() const {
		return _builder.cells();
	}

	void update() {
		if (doRegen) {
			if (cellsChanged) {
				_builder.regenerateEverything();
			} else {
				_builder.regeneratePositions();
			}
		}

		if (doUpload || cellChangeUploadFrames > 0) {
			_surfacePositions.copyData(_builder.positions());
			_surfaceNormals.copyData(_builder.normals());

			if (cellChangeUploadFrames > 0) {
				_surfaceColors.copyData(_builder.colors());
				_surfaceIndices.copyData(_builder.triangleIndices());
				_gridIndices.copyData(_builder.lineIndices());
				_gridColors.copyData(makeGridColors());
				_normalIndices.copyData(_builder.normalIndices());
				_normalColors.copyData(makeNormalColors());

				cellChangeUploadFrames--;
			}
		}

		if (cellsChanged) cellsChanged = false;
	}
};
}
