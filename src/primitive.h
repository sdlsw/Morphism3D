#pragma once

#include "vk/renderer.h"

namespace g3d {
// STANDALONE GENERATION FUNCTIONS

// CUBES
void cubePositions(
	std::vector<Position>& positions,
	float size,
	const Position& center = {0.0f, 0.0f, 0.0f}
);
void cubeLineIndices(
	std::vector<uint16_t>& indices,
	uint16_t offset = 0
);
void cubeTriangleIndices(
	std::vector<uint16_t>& indices,
	uint16_t offset = 0
);

// SPHERES
void spherePositions(
	std::vector<Position>& positions,
	float radius,
	unsigned int horizontalResolution,
	unsigned int verticalResolution,
	const Position& center = {0.0f, 0.0f, 0.0f}
);
void sphereLineIndices(
	std::vector<uint16_t>& indices,
	unsigned int horizontalResolution,
	unsigned int verticalResolution,
	uint16_t offset = 0
);
void sphereTriangleIndices(
	std::vector<uint16_t>& indices,
	unsigned int horizontalResolution,
	unsigned int verticalResolution,
	uint16_t offset = 0
);

// CYLINDERS
void cylinderPositions(
	std::vector<Position>& positions,
	float radius,
	float height,
	unsigned int horizontalResolution,
	unsigned int verticalResolution,
	const Position& center = {0.0f, 0.0f, 0.0f}
);
void cylinderLineIndices(
	std::vector<uint16_t>& indices,
	unsigned int horizontalResolution,
	unsigned int verticalResolution,
	uint16_t offset = 0
);
void cylinderTriangleIndices(
	std::vector<uint16_t>& indices,
	unsigned int horizontalResolution,
	unsigned int verticalResolution,
	uint16_t offset = 0
);

// CONES
void conePositions(
	std::vector<Position>& positions,
	float radius,
	float height,
	unsigned int horizontalResolution,
	unsigned int verticalResolution,
	const Position& center = {0.0f, 0.0f, 0.0f}
);
void coneLineIndices(
	std::vector<uint16_t>& indices,
	unsigned int horizontalResolution,
	unsigned int verticalResolution,
	uint16_t offset = 0
);
void coneTriangleIndices(
	std::vector<uint16_t>& indices,
	unsigned int horizontalResolution,
	unsigned int verticalResolution,
	uint16_t offset = 0
);

// Positions should be specified in CCW order.
glm::vec3 calcTriangleNormal(
	const Position& thisPosition,
	const Position& other1,
	const Position& other2
);

// Automatically generate the normal vectors for a triangle mesh.
std::vector<Normal> autoGenerateNormals(
	const std::vector<Position>& positions,
	const std::vector<uint16_t>& indices
);

// Creates a solid color resource of the given size.
StaticVertexAttributes<Color> solidColor(
	Renderer& renderer,
	size_t size,
	const Color& color
);

// Creates a solid color resource that matches an existing mesh in size.
StaticVertexAttributes<Color> solidColor(
	const StaticMesh& mesh,
	const Color& color
);

// Creates a new default material
Material defaultMaterial();

template<typename T>
struct enableBitmaskOps {
	static constexpr bool value = false;
};

template<typename T>
concept BitmaskOpsEnabled = enableBitmaskOps<T>::value;

template<BitmaskOpsEnabled T>
constexpr T operator|(const T& a, const T& b) {
	return static_cast<T>(std::to_underlying(a) | std::to_underlying(b));
}

template<BitmaskOpsEnabled T>
constexpr T operator&(const T& a, const T& b) {
	return static_cast<T>(std::to_underlying(a) & std::to_underlying(b));
}

template<BitmaskOpsEnabled T>
constexpr T operator~(const T& a) {
	return static_cast<T>(~std::to_underlying(a));
}

template<BitmaskOpsEnabled T>
constexpr bool any(const T& a) {
	return std::to_underlying(a) != 0;
}

enum class MeshBuilderMode : uint8_t {
	none = 0,
	genLines = 1 << 0,
	genTris = 1 << 1,
	genColors = 1 << 2
};

template<>
struct enableBitmaskOps<MeshBuilderMode> {
	static constexpr bool value = true;
};

enum class RotateDirection : uint8_t {
	CCW,
	CW
};

class MeshBuilder {
private:
	std::vector<Position> _positions;
	std::vector<uint16_t> _lineIndices;
	std::vector<uint16_t> _triIndices;
	std::vector<Color> _colors;
	MeshBuilderMode _mode;
	Renderer* _renderer;
	Color _color { 0.4f, 0.4f, 0.4f };
	Material _material;

	bool _inGroup = false;
	size_t _groupStart = 0;
	size_t _groupEnd = 0;

public:
	MeshBuilder(
		Renderer& renderer,
		MeshBuilderMode mode
	)
	: _renderer { &renderer },
	  _mode { mode },
	  _material { defaultMaterial() }
	{}

	Renderer& renderer() { return *_renderer; }
	bool modeHas(MeshBuilderMode check) const;

	MeshBuilder& beginGroup();
	MeshBuilder& endGroup();
	MeshBuilder& resetGroup();
	bool groupIsAll() const;
	size_t groupStart() const;
	size_t groupEnd() const;
	uint16_t currentOffset() const;

	// Rotate 90 degrees on the X, Y, Z axes respectively.
	MeshBuilder& rot90X(RotateDirection dir);
	MeshBuilder& rot90Y(RotateDirection dir);
	MeshBuilder& rot90Z(RotateDirection dir);

	MeshBuilder& cube(
		float size,
		const Position& center = { 0.0f, 0.0f, 0.0f }
	);

	MeshBuilder& sphere(
		float radius,
		unsigned int horizontalResolution,
		unsigned int verticalResolution,
		const Position& center = { 0.0f, 0.0f, 0.0f }
	);

	MeshBuilder& cylinder(
		float radius,
		float height,
		unsigned int horizontalResolution,
		unsigned int verticalResolution,
		const Position& center = { 0.0f, 0.0f, 0.0f }
	);

	MeshBuilder& cone(
		float radius,
		float height,
		unsigned int horizontalResolution,
		unsigned int verticalResolution,
		const Position& center = { 0.0f, 0.0f, 0.0f }
	);

	// Sets the color to be used for subsequent shape generation calls.
	// If MeshBuilderMode::genColors is disabled, then a single solid color
	// buffer is generated upon calling colors() instead.
	MeshBuilder& setColor(const Color& color) {
		_color = color;
		return *this;
	}

	// Since materials are per-object, setting the material with this
	// function will affect the entire object when generated.
	MeshBuilder& setMaterial(const Material& material) {
		_material = material;
		return *this;
	}

	StaticMesh lineMesh();
	StaticMesh triMesh();
	StaticVertexAttributes<Color> colors();
	StaticVertexAttributes<Normal> normals();
	Material material() { return _material; }
};

class SimpleUnlitObject {
private:
	StaticMesh _mesh;
	StaticVertexAttributes<Color> _colors;
	Entity _entity;

public:
	static constexpr MeshBuilderMode flags = MeshBuilderMode::genTris;
	static MeshBuilder mkBuilder(
		Renderer& renderer,
		MeshBuilderMode mode = MeshBuilderMode::none
	) {
		return { renderer, flags | mode };
	}

	SimpleUnlitObject(MeshBuilder& builder)
	: _mesh { builder.triMesh() },
	  _colors { builder.colors() }
	{
		populateStaticEntity(builder.renderer(), _entity, {}, _mesh, _colors);
	}

	void render() {
		_entity.render();
	}

	Entity& entity() {
		return _entity;
	}
};

class SimpleLitObject {
private:
	StaticMesh _mesh;
	StaticVertexAttributes<Color> _colors;
	StaticVertexAttributes<Normal> _normals;
	Material _material;
	Entity _entity;

public:
	static constexpr MeshBuilderMode flags = MeshBuilderMode::genTris;
	static MeshBuilder mkBuilder(
		Renderer& renderer,
		MeshBuilderMode mode = MeshBuilderMode::none
	) {
		return { renderer, flags | mode };
	}

	SimpleLitObject(MeshBuilder& builder)
	: _mesh { builder.triMesh() },
	  _colors { builder.colors() },
	  _normals { builder.normals() },
	  _material { builder.material() }
	{
		populateStaticEntity(builder.renderer(), _entity, {}, _mesh, _colors, _normals);
		_entity.addComponent<MaterialComponent>(builder.renderer(), _material);
	}

	void render() {
		_entity.render();
	}

	Entity& entity() {
		return _entity;
	}
};

// Compares the triangle and line meshes for the object contained in a
// MeshBuilder.
class MeshComparison {
private:
	StaticMesh _lineMesh;
	StaticMesh _triMesh;
	StaticVertexAttributes<Color> _colors;
	StaticVertexAttributes<Normal> _normals;
	Material _material;
	Entity _lineEntity;
	Entity _triEntity;

public:
	// Convenience function that creates a mesh builder with all flags
	// required by this class.
	static constexpr MeshBuilderMode flags = MeshBuilderMode::genLines | MeshBuilderMode::genTris;
	static MeshBuilder mkBuilder(Renderer& renderer) {
		return { renderer, flags };
	}

	MeshComparison(MeshBuilder& builder, float x, float z)
	: _lineMesh { builder.lineMesh() },
	  _triMesh { builder.triMesh() },
	  _colors { builder.colors() },
	  _normals { builder.normals() },
	  _material { defaultMaterial() }
	{
		populateStaticEntity(builder.renderer(), _lineEntity, {}, _lineMesh, _colors);
		getTransform(_lineEntity).translation = {x, 0.0f, -z};

		populateStaticEntity(builder.renderer(), _triEntity, {}, _triMesh, _colors, _normals);
		_triEntity.addComponent<MaterialComponent>(builder.renderer(), _material);
		getTransform(_triEntity).translation = {x, 0.0f, z};
	}

	Entity& lineEntity() {
		return _lineEntity;
	}

	Entity& triEntity() {
		return _triEntity;
	}
};

// Combined object that renders all the primitives in both wireframe and lit
// filled surface mode. Used for debug.
class PrimitiveTest {
private:
	static constexpr float size = 0.3f;
	static constexpr float padding = 0.1f;
	static constexpr float moveX = 2.0f*size + padding;
	static constexpr float moveZ = size + padding;
	static constexpr unsigned int resolution = 20;
	static constexpr Color color { 0.1f, 1.0f, 0.1f };

	MeshComparison _cones;
	MeshComparison _cylinders;
	MeshComparison _spheres;
	MeshComparison _cubes;

public:
	PrimitiveTest(Renderer& renderer) :
	_cones {
		MeshComparison::mkBuilder(renderer)
			.setColor(color)
			.cone(size, size, resolution, resolution),
		moveX, moveZ
	},
	_cylinders {
		MeshComparison::mkBuilder(renderer)
			.setColor(color)
			.cylinder(size, size, resolution, resolution),
		0.0f, moveZ
	},
	_spheres {
		MeshComparison::mkBuilder(renderer)
			.setColor(color)
			.sphere(size, resolution, resolution),
		-moveX, moveZ
	},
	_cubes {
		MeshComparison::mkBuilder(renderer)
			.setColor(color)
			.cube(size),
		-2.0*moveX, moveZ
	}
	{}

	void renderLit() {
		_cones.triEntity().render();
		_cylinders.triEntity().render();
		_spheres.triEntity().render();
		_cubes.triEntity().render();
	}

	void renderLines() {
		_cones.lineEntity().render();
		_cylinders.lineEntity().render();
		_spheres.lineEntity().render();
		_cubes.lineEntity().render();
	}
};
}
