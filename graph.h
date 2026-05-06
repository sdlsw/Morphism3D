#pragma once

#include "vk_renderer.h"

#include <concepts>
#include <vector>

namespace g3d {
template<typename F>
concept Graphable = requires(F f, float x, float y) {
	{ f.eval(x, y) } -> std::convertible_to<float>;
};

std::vector<Vertex> recolor(
	const std::vector<Vertex>& verts,
	const glm::vec3& newColor
);

// other2 must be clockwise from other1.
glm::vec3 calcTriangleNormal(
	const Vertex& thisVertex,
	const Vertex& other1,
	const Vertex& other2
);

template<typename T, typename U>
T lerp2d(const T& ul, const T& ur, const T& dl, const T& dr, U a, U b) {
	T u = glm::mix(ul, ur, a);
	T d = glm::mix(dl, dr, a);

	return glm::mix(u, d, b);
}

template<Graphable F>
class Graph {
private:
	// The number of discrete steps to walk along each input variable.
	// Higher values yield higher accuracy at the cost of additional
	// vertices.
	unsigned int _cells;


	// The x, y coordinates of the model vertices always range from
	// -1.0f to 1.0f. The range determines how those coordinates map to
	// function input space with a simple relation:
	//
	//     (model_x, model_y, model_z)*range = (graph_x, graph_y, f(graph_x, graph_y))
	//		
	// graph_x and graph_y will both range from -range to +range.
	float _range;
	const F* _func;

	std::vector<Vertex> _vertices;
	std::vector<glm::vec3> _normals;

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

	const Vertex& getVertex(unsigned int x, unsigned int y) {
		return _vertices[idx(x, y)];
	}

	glm::vec3 calcVertexNormal(unsigned int x, unsigned int y) {
		std::vector<glm::vec3> faceNormals;
		const g3d::Vertex& thisVertex = getVertex(x, y);

		if (x < _cells && y < _cells) {
			faceNormals.push_back(calcTriangleNormal(
				thisVertex,
				getVertex(x+1, y),
				getVertex(x, y+1)
			));
		}

		if (x > 0 && y < _cells) {
			faceNormals.push_back(calcTriangleNormal(
				thisVertex,
				getVertex(x-1, y+1),
				getVertex(x-1, y)
			));
			faceNormals.push_back(calcTriangleNormal(
				thisVertex,
				getVertex(x, y+1),
				getVertex(x-1, y+1)
			));
		}

		if (x > 0 && y > 0) {
			faceNormals.push_back(calcTriangleNormal(
				thisVertex,
				getVertex(x-1, y),
				getVertex(x, y-1)
			));
		}

		if (x < _cells && y > 0) {
			faceNormals.push_back(calcTriangleNormal(
				thisVertex,
				getVertex(x+1, y-1),
				getVertex(x+1, y)
			));
			faceNormals.push_back(calcTriangleNormal(
				thisVertex,
				getVertex(x, y-1),
				getVertex(x+1, y-1)
			));
		}

		glm::vec3 sum {};
		for (const auto& v : faceNormals) {
			sum += v;
		}

		return sum / static_cast<float>(faceNormals.size());
	}

	std::vector<Vertex> generateVertices() {
		std::vector<Vertex> out;

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

				out.push_back({
					toModelSpace({x, y, _func->eval(x, y)}),
					lerp2d(nxny, pxny, nxpy, pxpy, lerp_a_x, lerp_a_y)
				});
			}
		}

		return out;
	}
	std::vector<glm::vec3> generateNormals() {
		std::vector<glm::vec3> normals;

		for (unsigned int ypt = 0; ypt <= _cells; ypt++) {
			for (unsigned int xpt = 0; xpt <= _cells; xpt++) {
				normals.push_back(calcVertexNormal(xpt, ypt));
			}
		}

		return normals;
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
public:
	Graph() = delete;
	Graph(const F& f, unsigned int cells, float range)
	: _func { &f },
	  _cells { cells },
	  _range { range },
	  _vertices { generateVertices() },
	  _normals { generateNormals() }
	{}

	auto cells() const { return _cells; }
	auto range() const { return _range; }
	const auto& vertices() const { return _vertices; }
	const auto& normals() const { return _normals; }

	RenderObject makeSurfaceObject(GraphDevice& device, Renderer& renderer) {
		return {
			device,
			renderer,
			{
				device,
				_vertices,
				generateTriangleIndices()
			},
			{}
		};
	}

	RenderObject makeGridObject(GraphDevice& device, Renderer& renderer) {
		glm::vec3 color { 0.1f, 0.1f, 0.1f };

		return {
			device,
			renderer,
			{
				device,
				recolor(_vertices, color),
				generateLineIndices()
			},
			{}
		};
	}

	RenderObject makeNormalObject(GraphDevice& device, Renderer& renderer) {
		glm::vec3 color { 1.0f, 1.0f, 1.0f };
		float normLength = 0.1f;
		std::vector<Vertex> vertices { recolor(_vertices, color) };
		std::vector<uint16_t> indices {};

		for (size_t i = 0; i < _vertices.size(); i++) {
			vertices.push_back({_vertices[i].pos + _normals[i]*normLength, color});
			indices.push_back(static_cast<uint16_t>(i));
			indices.push_back(static_cast<uint16_t>(i + _vertices.size()));
		}

		return {
			device,
			renderer,
			{
				device,
				vertices,
				indices
			},
			{}
		};
	}
};
}
