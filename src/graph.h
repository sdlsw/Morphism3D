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

template<typename T, typename U>
T lerp2d(const T& ul, const T& ur, const T& dl, const T& dr, U a, U b) {
	T u = glm::mix(ul, ur, a);
	T d = glm::mix(dl, dr, a);

	return glm::mix(u, d, b);
}

template<Graphable F>
class Graph {
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

	// Since the structure of the graph is already known, a custom function
	// that doesn't need an index buffer is used here. TODO maybe it would
	// be simpler to just use autoGenerateNormals?
	Normal calcVertexNormal(unsigned int x, unsigned int y) {
		std::vector<glm::vec3> faceNormals;
		const Position& thisPosition = getPosition(x, y);

		if (x < _cells && y < _cells) {
			faceNormals.push_back(calcTriangleNormal(
				thisPosition,
				getPosition(x+1, y),
				getPosition(x, y+1)
			));
		}

		if (x > 0 && y < _cells) {
			faceNormals.push_back(calcTriangleNormal(
				thisPosition,
				getPosition(x-1, y+1),
				getPosition(x-1, y)
			));
			faceNormals.push_back(calcTriangleNormal(
				thisPosition,
				getPosition(x, y+1),
				getPosition(x-1, y+1)
			));
		}

		if (x > 0 && y > 0) {
			faceNormals.push_back(calcTriangleNormal(
				thisPosition,
				getPosition(x-1, y),
				getPosition(x, y-1)
			));
		}

		if (x < _cells && y > 0) {
			faceNormals.push_back(calcTriangleNormal(
				thisPosition,
				getPosition(x+1, y-1),
				getPosition(x+1, y)
			));
			faceNormals.push_back(calcTriangleNormal(
				thisPosition,
				getPosition(x, y-1),
				getPosition(x+1, y-1)
			));
		}

		glm::vec3 sum {};
		for (const auto& v : faceNormals) {
			sum += v;
		}

		return sum / static_cast<float>(faceNormals.size());
	}

	void generateVertices() {
		// Colors for extreme points of graph.
		// nxny - (-range, -range)
		// pxny - (range, -range)
		// nxpy - (-range, range)
		// pxpy - (range, range)
		glm::vec3 nxny {0.141f, 0.706f, 0.322f}; // green
		glm::vec3 pxny {0.988f, 0.804f, 0.000f}; // yellow orange
		glm::vec3 nxpy {0.400f, 0.255f, 0.953f}; // blue violet
		glm::vec3 pxpy {1.000f, 0.000f, 0.000f}; // red

		for (unsigned int ypt = 0; ypt <= _cells; ypt++) {
			float lerp_a_y = static_cast<float>(ypt) / static_cast<float>(_cells);
			float y = glm::mix(-_range, _range, lerp_a_y);

			for (unsigned int xpt = 0; xpt <= _cells; xpt++) {
				float lerp_a_x = static_cast<float>(xpt) / static_cast<float>(_cells);
				float x = glm::mix(-_range, _range, lerp_a_x);

				_positions.push_back(toModelSpace({x, y, _func->eval(x, y)}));
				_colors.push_back(lerp2d(nxny, pxny, nxpy, pxpy, lerp_a_x, lerp_a_y));
			}
		}
	}

	void generateVertexNormals() {
		for (unsigned int ypt = 0; ypt <= _cells; ypt++) {
			for (unsigned int xpt = 0; xpt <= _cells; xpt++) {
				_normals.push_back(calcVertexNormal(xpt, ypt));
			}
		}
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

public:
	Graph() = delete;
	Graph(const F& f, unsigned int cells, float range)
	: _func { &f },
	  _cells { cells },
	  _range { range }
	{
		regenerateVertices();
	}

	auto cells() const { return _cells; }
	void cells(unsigned int cells) {
		_cells = cells;
		_cellsChangedFrames = MAX_FRAMES_IN_FLIGHT;
	}
	bool cellsChanged() const { return _cellsChangedFrames > 0; }
	void decChangeFrames() { _cellsChangedFrames--; }

	auto range() const { return _range; }
	const auto& positions() const { return _positions; }
	const auto& colors() const { return _colors; }
	const auto& normals() const { return _normals; }
	auto pointCount() const { return (_cells + 1) * (_cells + 1); }

	void regenerateVertices() {
		_positions.clear();
		_colors.clear();
		_normals.clear();

		generateVertices();
		generateVertexNormals();
		generateNormalPositions();
	}

	std::vector<uint16_t> generateLineIndices() {
		std::vector<uint16_t> out;

		// generate horizontal lines
		for (unsigned int ypt = 0; ypt <= _cells; ypt++) {
			for (unsigned int xpt = 0; xpt < _cells; xpt++) {
				out.push_back(idx(xpt, ypt));
				out.push_back(idx(xpt+1, ypt));
			}
		}

		// generate vertical lines
		for (unsigned int xpt = 0; xpt <= _cells; xpt++) {
			for (unsigned int ypt = 0; ypt < _cells; ypt++) {
				out.push_back(idx(xpt, ypt));
				out.push_back(idx(xpt, ypt+1));
			}
		}

		return out;
	}

	std::vector<uint16_t> generateTriangleIndices() {
		std::vector<uint16_t> out;

		// X and Y correspond to the top left vertex of the quad being
		// generated.
		for (unsigned int ypt = 0; ypt < _cells; ypt++) {
			for (unsigned int xpt = 0; xpt < _cells; xpt++) {
				// first tri
				out.push_back(idx(xpt, ypt));
				out.push_back(idx(xpt+1, ypt));
				out.push_back(idx(xpt, ypt+1));

				// second tri
				out.push_back(idx(xpt+1, ypt));
				out.push_back(idx(xpt+1, ypt+1));
				out.push_back(idx(xpt, ypt+1));
			}
		}

		return out;
	}

	std::vector<uint16_t> generateNormalIndices() {
		std::vector<uint16_t> out;
		auto ptCount = pointCount();

		for (unsigned int i = 0; i < ptCount; i++) {
			out.push_back(i);
			out.push_back(i + ptCount);
		}

		return out;
	}
};

template<typename T>
class GraphEntities {
private:
	static constexpr float gridLoft = 0.002f;

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

	std::vector<Color> makeGridColors(Graph<T>& graph) {
		return { graph.pointCount(), {0.1f, 0.1f, 0.1f} };
	}

	std::vector<Color> makeNormalColors(Graph<T>& graph) {
		return { 2*graph.pointCount(), {1.0f, 1.0f, 1.0f} };
	}
public:
	GraphEntities(
		Renderer& renderer,
		Graph<T>& graph
	)
	: _surfacePositions { renderer, graph.positions() },
	  _surfaceColors { renderer, graph.colors() },
	  _surfaceNormals { renderer, graph.normals() },
	  _surfaceIndices { renderer, graph.generateTriangleIndices() },
	  _gridIndices { renderer, graph.generateLineIndices() },
	  _gridColors { renderer, makeGridColors(graph) },
	  _normalIndices { renderer, graph.generateNormalIndices() },
	  _normalColors { renderer, makeNormalColors(graph) }
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

	void update(Graph<T>& graph) {
		_surfacePositions.copyData(graph.positions());
		_surfaceNormals.copyData(graph.normals());

		if (graph.cellsChanged()) {
			_surfaceColors.copyData(graph.colors());
			_surfaceIndices.copyData(graph.generateTriangleIndices());
			_gridIndices.copyData(graph.generateLineIndices());
			_gridColors.copyData(makeGridColors(graph));
			_normalIndices.copyData(graph.generateNormalIndices());
			_normalColors.copyData(makeNormalColors(graph));

			graph.decChangeFrames();
		}
	}
};
}
