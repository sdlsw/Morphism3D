#include "primitive.h"

#include <glm/gtc/matrix_transform.hpp>

template<typename T>
concept IndexSource = requires(T obj, uint16_t x, uint16_t y) {
	{ obj.index(x, y) } -> std::same_as<uint16_t>;

	// X and Y are expected to range [0, endX()), [0, endY()) respectively.
	{ obj.endX() } -> std::same_as<uint16_t>;
	{ obj.endY() } -> std::same_as<uint16_t>;
};

static void genRing(
	std::vector<g3d::Position>& positions,
	float radius,
	const g3d::Position& center,
	unsigned int resolution
) {
	for (unsigned int pt = 0; pt < resolution; pt++) {
		float lerp = static_cast<float>(pt) / static_cast<float>(resolution);
		float angle = glm::mix(0.0f, 2.0f*glm::pi<float>(), lerp);

		glm::vec3 offset { radius*glm::cos(angle), radius*glm::sin(angle), 0.0f };
		positions.push_back(offset + center.vec);
	}
}

// Generates a disc of points, leaving out the center. Expands outward from the
// center, unless `shrink` is true, then shrinks inward from furthest ring.
// (The ordering is important for the method used to generate indices.)
//
// Even though this is a flat surface, lots of vertices are needed to help
// sharpen the interpolated vertex normals used for lighting the generated
// primitives. This effect is most obvious when using low vertical resolution
// to generate a cone.
//
// TODO: Might be more optimal to use a nonlinear interpolation scheme to
// cluster the rings closer to the edge of the disc.
static void genDisc(
	std::vector<g3d::Position>& positions,
	float radius,
	const g3d::Position& center,
	unsigned int ringResolution,
	unsigned int expandResolution,
	bool shrink = false
) {
	unsigned int startRing = 1;
	unsigned int endRing = expandResolution + 1;
	float startRadius = 0.0f;
	float endRadius = radius;

	if (shrink) {
		startRing--;
		endRing--;
		startRadius = radius;
		endRadius = 0.0f;
	}

	for (unsigned int ring = startRing; ring < endRing; ring++) {
		float lerp = static_cast<float>(ring) / static_cast<float>(expandResolution);
		float r = glm::mix(startRadius, endRadius, lerp);
		genRing(positions, r, center, ringResolution);
	}
}

enum class IxOrd : int {
	CW,
	CCW
};

// Utility class that presents useful higher level functions for populating
// index buffers.
template<IndexSource T>
struct IndexBuilder {
private:
	std::vector<uint16_t>* indices;
	uint16_t offset;
public:
	T indexSource;

	IndexBuilder(
		std::vector<uint16_t>& _indices,
		const T& source,
		uint16_t _offset = 0
	) : indices { &_indices }, indexSource { source }, offset { _offset } {}

	inline uint16_t index(uint16_t x, uint16_t y) const { return indexSource.index(x, y); }
	inline uint16_t endX() const { return indexSource.endX(); }
	inline uint16_t endY() const { return indexSource.endY(); }

	void putLine(uint16_t ix1, uint16_t ix2) {
		indices->push_back(ix1+offset);
		indices->push_back(ix2+offset);
	}

	void putLineVertical(uint16_t x, uint16_t y) {
		putLine(index(x, y), index(x, y+1));
	}

	void putLineHorizontal(uint16_t x, uint16_t y) {
		putLine(index(x, y), index(x+1, y));
	}

	void lineRadialX(uint16_t center, uint16_t start, uint16_t end, uint16_t y) {
		for (uint16_t x = start; x < end; x++) {
			putLine(center, index(x, y));
		}
	}

	void lineRadialX(uint16_t center, uint16_t y) {
		lineRadialX(center, 0, endX(), y);
	}

	void lineRunX(uint16_t start, uint16_t end, uint16_t y, bool connected = true) {
		for (uint16_t x = start; x < end-1; x++) {
			putLineHorizontal(x, y);
		}

		if (connected) {
			putLine(index(end-1, y), index(start, y));
		}
	}

	void lineRunX(uint16_t y, bool connected = true) {
		lineRunX(0, endX(), y, connected);
	}

	void lineRunY(uint16_t x, uint16_t start, uint16_t end, bool connected = true) {
		for (uint16_t y = start; y < end-1; y++) {
			putLineVertical(x, y);
		}

		if (connected) {
			putLine(index(x, end-1), index(x, start));
		}
	}

	void lineGrid(
		uint16_t startx,
		uint16_t endx,
		uint16_t starty,
		uint16_t endy,
		bool connectedx = true,
		bool connectedy = true
	) {
		// Horizontal lines
		for (uint16_t y = starty; y < endy; y++) {
			lineRunX(startx, endx, y, connectedx);
		}

		// Vertical lines
		for (uint16_t x = startx; x < endx; x++) {
			lineRunY(x, starty, endy, connectedy);
		}
	}

	void lineGrid(bool connectedx = true, bool connectedy = true) {
		lineGrid(0, endX(), 0, endY(), connectedx, connectedy);
	}

	template<IxOrd O>
	void putTri(uint16_t ix1, uint16_t ix2, uint16_t ix3) {
		indices->push_back(ix1+offset);

		if constexpr (O == IxOrd::CCW) {
			indices->push_back(ix2+offset);
			indices->push_back(ix3+offset);
		} else {
			indices->push_back(ix3+offset);
			indices->push_back(ix2+offset);
		}
	}

	template<IxOrd O>
	void putQuadIx(uint16_t tl, uint16_t tr, uint16_t bl, uint16_t br) {
		putTri<O>(tl, bl, tr);
		putTri<O>(tr, bl, br);
	}

	template<IxOrd O>
	void putQuadXY(uint16_t x, uint16_t y) {
		putQuadIx<O>(index(x, y), index(x+1, y), index(x, y+1), index(x+1, y+1));
	}

	template<IxOrd O>
	void quadRunX(uint16_t start, uint16_t end, uint16_t y, bool connected = true) {
		for (uint16_t x = start; x < end - 1; x++) {
			putQuadXY<O>(x, y);
		}

		if (connected) {
			putQuadIx<O>(
				index(end-1, y),
				index(start, y),
				index(end-1, y+1),
				index(start, y+1)
			);
		}
	}

	template<IxOrd O>
	void quadRunX(uint16_t y, bool connected = true) {
		quadRunX<O>(0, endX(), y, connected);
	}

	template<IxOrd O>
	void quadFieldX(bool connected = true) {
		for (uint16_t y = 0; y < endY() - 1; y++) {
			quadRunX<O>(y, connected);
		}
	}

	// Builds a fan with center index "center", by running along the x
	// points [start, end). Optionally connects the endpoint.
	template<IxOrd O>
	void fanRunX(uint16_t center, uint16_t start, uint16_t end, uint16_t y, bool connected = true) {
		for (uint16_t x = start; x < end-1; x++) {
			putTri<O>(center, index(x, y), index(x+1, y));
		}

		if (connected) {
			putTri<O>(center, index(end-1, y), index(start, y));
		}
	}

	template<IxOrd O>
	void fanRunX(uint16_t center, uint16_t y, bool connected = true) {
		fanRunX<O>(center, 0, endX(), y, connected);
	}
};

struct CubeIndexSource {
	inline uint16_t index(uint16_t x, uint16_t y) const {
		return 4*y + x;
	}

	inline uint16_t endX() const { return 4; }
	inline uint16_t endY() const { return 2; }
};

struct SphereInfo { static constexpr unsigned int endYmult = 1; };
struct CylinderInfo { static constexpr unsigned int endYmult = 3; };
struct ConeInfo { static constexpr unsigned int endYmult = 2; };

// Index generation funcs for "radial" shapes like spheres, cones and cylinders are
// almost identical except for a few parameters.
template<typename T>
struct RadialIndexSource {
	unsigned int horizontalResolution;
	unsigned int verticalResolution;

	inline uint16_t index(uint16_t pt, uint16_t ring) const {
		return 1 + horizontalResolution*ring + pt;
	}

	inline uint16_t endX() const { return horizontalResolution; }
	inline uint16_t endY() const { return (T::endYmult)*verticalResolution - 1; }
	inline uint16_t posCount() const { return endY()*horizontalResolution + 2; }
};

template<typename T>
static inline void radialLineIndices(
	std::vector<uint16_t>& indices,
	unsigned int horizontalResolution,
	unsigned int verticalResolution,
	uint16_t offset = 0
) {
	IndexBuilder<RadialIndexSource<T>> buf {
		indices,
		{ horizontalResolution, verticalResolution },
		offset
	};
	uint16_t posCount = buf.indexSource.posCount();

	buf.lineRadialX(0, 0);
	buf.lineGrid(true, false);
	buf.lineRadialX(posCount - 1, buf.endY() - 1);
}

template<typename T>
static inline void radialTriangleIndices(
	std::vector<uint16_t>& indices,
	unsigned int horizontalResolution,
	unsigned int verticalResolution,
	uint16_t offset = 0
) {
	IndexBuilder<RadialIndexSource<T>> buf {
		indices,
		{ horizontalResolution, verticalResolution },
		offset
	};
	uint16_t posCount = buf.indexSource.posCount();

	buf.fanRunX<IxOrd::CCW>(0, 0);
	buf.quadFieldX<IxOrd::CCW>();
	buf.fanRunX<IxOrd::CW>(posCount - 1, buf.endY() - 1);
}

// IMPLEMENTATION OF PUBLIC INTERFACE
namespace g3d {
void cubePositions(
	std::vector<Position>& positions,
	float size,
	const Position& center
) {
	// half size
	float hs = size / 2.0f;

	for (unsigned int z = 0; z < 2; z++) {
		// We generate a square by looping CCW around the center.
		// Imagine the Y axis pointing away, X axis pointing right,
		// Z axis pointing up.
		std::array<glm::vec3, 4> offsets {
			glm::vec3( -hs, -hs, hs - z*size ), // bl
			glm::vec3(  hs, -hs, hs - z*size ), // br
			glm::vec3(  hs,  hs, hs - z*size ), // tr
			glm::vec3( -hs,  hs, hs - z*size )  // tl
		};

		for (const auto& offset : offsets) {
			positions.push_back(offset + center.vec);
		}
	}
}

void cubeLineIndices(std::vector<uint16_t>& indices, uint16_t offset) {
	IndexBuilder<CubeIndexSource> buf { indices, {}, offset };
	buf.lineGrid(true, false);
}

void cubeTriangleIndices(std::vector<uint16_t>& indices, uint16_t offset) {
	IndexBuilder<CubeIndexSource> buf { indices, {}, offset };
	buf.fanRunX<IxOrd::CCW>(0, 1, buf.endX(), 0, false);
	buf.quadFieldX<IxOrd::CCW>();
	buf.fanRunX<IxOrd::CW>(4, 1, buf.endX(), 0, false);
}

void spherePositions(
	std::vector<Position>& positions,
	float radius,
	unsigned int horizontalResolution,
	unsigned int verticalResolution,
	const Position& center
) {
	// Top point
	glm::vec3 tpOffset { 0.0f, 0.0f, radius };
	positions.push_back(center.vec + tpOffset);

	for (unsigned int ring = 1; ring < verticalResolution; ring++) {
		float lerp = static_cast<float>(ring) / static_cast<float>(verticalResolution);
		float angle = glm::mix(0.0f, glm::pi<float>(), lerp);
		float z = radius*glm::cos(angle);
		float r = radius*glm::sin(angle);

		glm::vec3 offset { 0.0f, 0.0f, z };
		genRing(positions, r, center.vec + offset, horizontalResolution);
	}

	// Bottom point
	glm::vec3 bpOffset {0.0f, 0.0f, -radius};
	positions.push_back(center.vec + bpOffset);
}

void sphereLineIndices(
	std::vector<uint16_t>& indices,
	unsigned int horizontalResolution,
	unsigned int verticalResolution,
	uint16_t offset
) {
	radialLineIndices<SphereInfo>(indices, horizontalResolution, verticalResolution, offset);
}

void sphereTriangleIndices(
	std::vector<uint16_t>& indices,
	unsigned int horizontalResolution,
	unsigned int verticalResolution,
	uint16_t offset
) {
	radialTriangleIndices<SphereInfo>(indices, horizontalResolution, verticalResolution, offset);
}

void cylinderPositions(
	std::vector<Position>& positions,
	float radius,
	float height,
	unsigned int horizontalResolution,
	unsigned int verticalResolution,
	const Position& center
) {
	float halfHeight = height / 2.0f;

	glm::vec3 topOffset { 0.0f, 0.0f, halfHeight };
	glm::vec3 bottomOffset { 0.0f, 0.0f, -halfHeight };

	// Center of top face
	positions.push_back(center.vec + topOffset);

	// Top
	genDisc(
		positions,
		radius,
		center.vec + topOffset,
		horizontalResolution,
		verticalResolution,
		false
	);

	// Sides
	for (unsigned int ring = 1; ring < verticalResolution; ring++) {
		float lerp = static_cast<float>(ring) / static_cast<float>(verticalResolution);
		float z = glm::mix(halfHeight, -halfHeight, lerp);
		glm::vec3 offset { 0.0f, 0.0f, z };
		genRing(positions, radius, center.vec + offset, horizontalResolution);
	}

	// Bottom
	genDisc(
		positions,
		radius,
		center.vec + bottomOffset,
		horizontalResolution,
		verticalResolution,
		true
	);

	// Center of bottom face
	positions.push_back(center.vec + bottomOffset);
}

void cylinderLineIndices(
	std::vector<uint16_t>& indices,
	unsigned int horizontalResolution,
	unsigned int verticalResolution,
	uint16_t offset
) {
	radialLineIndices<CylinderInfo>(indices, horizontalResolution, verticalResolution, offset);
}

void cylinderTriangleIndices(
	std::vector<uint16_t>& indices,
	unsigned int horizontalResolution,
	unsigned int verticalResolution,
	uint16_t offset
) {
	radialTriangleIndices<CylinderInfo>(indices, horizontalResolution, verticalResolution, offset);
}

void conePositions(
	std::vector<Position>& positions,
	float radius,
	float height,
	unsigned int horizontalResolution,
	unsigned int verticalResolution,
	const Position& center
) {
	// Tip
	glm::vec3 tipOffset { 0.0f, 0.0f, height };
	positions.push_back(center.vec + tipOffset);

	// Sides
	for (unsigned int ring = 1; ring < verticalResolution; ring++) {
		float lerp = static_cast<float>(ring) / static_cast<float>(verticalResolution);
		float r = glm::mix(0.0f, radius, lerp);
		float z = glm::mix(height, 0.0f, lerp);

		glm::vec3 offset { 0.0f, 0.0f, z };
		genRing(positions, r, center.vec + offset, horizontalResolution);
	}

	// Bottom
	genDisc(
		positions,
		radius,
		center,
		horizontalResolution,
		verticalResolution,
		true
	);

	// Center of bottom
	positions.push_back(center);
}

void coneLineIndices(
	std::vector<uint16_t>& indices,
	unsigned int horizontalResolution,
	unsigned int verticalResolution,
	uint16_t offset
) {
	radialLineIndices<ConeInfo>(indices, horizontalResolution, verticalResolution, offset);
}

void coneTriangleIndices(
	std::vector<uint16_t>& indices,
	unsigned int horizontalResolution,
	unsigned int verticalResolution,
	uint16_t offset
) {
	radialTriangleIndices<ConeInfo>(indices, horizontalResolution, verticalResolution, offset);
}

glm::vec3 calcTriangleNormal(
	const Position& thisPosition,
	const Position& other1,
	const Position& other2
) {
	return glm::normalize(glm::cross(
		other1.vec-thisPosition.vec, other2.vec-thisPosition.vec
	));
}

std::vector<Normal> autoGenerateNormals(
	const std::vector<Position>& positions,
	const std::vector<uint16_t>& indices
) {
	if (indices.size() % 3 != 0) {
		throw std::runtime_error("autoGenerateNormals: index amount not divisible by 3!");
	}

	std::vector<Normal> normals { positions.size(), {0.0f, 0.0f, 0.0f} };
	auto counts = std::vector<uint16_t>(positions.size(), 0);

	unsigned int triangleBase = 0;

	// Add up face normals and identify how many faces each vertex is
	// involved in
	while (triangleBase < indices.size()) {
		std::array<uint16_t, 3> ix;
		for (unsigned int pt = 0; pt < 3; pt++) {
			ix[pt] = indices[triangleBase+pt];
		}

		glm::vec3 faceNorm = calcTriangleNormal(
			positions[ix[0]],
			positions[ix[1]],
			positions[ix[2]]
		);

		for (unsigned int pt = 0; pt < 3; pt++) {
			normals[ix[pt]].vec += faceNorm;
			counts[ix[pt]]++;
		}

		triangleBase += 3;
	}

	// Perform average
	for (unsigned int vertIx = 0; vertIx < positions.size(); vertIx++) {
		normals[vertIx].vec /= counts[vertIx];
	}

	return normals;
}

StaticVertexAttributes<g3d::Color> solidColor(
	Renderer& renderer,
	size_t size,
	const g3d::Color& color
) {
	std::vector<Color> colors { size, color };
	return { renderer, colors };
}

StaticVertexAttributes<Color> solidColor(
	const StaticMesh& mesh,
	const Color& color
) {
	return solidColor(mesh.renderer(), mesh.positionCount(), color);
}

Material defaultMaterial() {
	return { 0.01f, 0.75f, 0.5f, 8.0f };
}

MeshBuilder& MeshBuilder::beginGroup() {
	if (_inGroup) {
		throw std::runtime_error("Cannot begin group while already in group");
	}

	_inGroup = true;
	_groupStart = _positions.size();

	return *this;
}

MeshBuilder& MeshBuilder::endGroup() {
	if (!_inGroup) {
		throw std::runtime_error("Cannot end group while not in group");
	}

	_inGroup = false;
	_groupEnd = _positions.size();

	return *this;
}

MeshBuilder& MeshBuilder::resetGroup() {
	_inGroup = false;
	_groupStart = 0;
	_groupEnd = 0;

	return *this;
}

// The builder starts with an implicit automatically size group that includes
// everything currently in the position buffer.
bool MeshBuilder::groupIsAll() const {
	return _groupStart == 0 && _groupEnd == 0;
}

size_t MeshBuilder::groupStart() const {
	return _groupStart;
}

size_t MeshBuilder::groupEnd() const {
	if (groupIsAll()) return _positions.size();
	return _groupEnd;
}

uint16_t MeshBuilder::currentOffset() const {
	if (_positions.size() > std::numeric_limits<uint16_t>::max()) {
		throw std::overflow_error("Positions vector is too large");
	}

	return static_cast<uint16_t>(_positions.size());
}

MeshBuilder& MeshBuilder::rot90X(RotateDirection dir) {
	if (_inGroup) {
		throw std::runtime_error("Cannot rotate while defining group");
	}

	float dirFactor = dir == RotateDirection::CCW ? 1.0f : -1.0f;

	for (size_t i = groupStart(); i < groupEnd(); i++) {
		const auto& p = _positions[i].vec;
		_positions[i] = Position(p.x, -dirFactor*p.z, dirFactor*p.y);
	}

	return *this;
}

MeshBuilder& MeshBuilder::rot90Y(RotateDirection dir) {
	if (_inGroup) {
		throw std::runtime_error("Cannot rotate while defining group");
	}

	float dirFactor = dir == RotateDirection::CCW ? 1.0f : -1.0f;

	for (size_t i = groupStart(); i < groupEnd(); i++) {
		const auto& p = _positions[i].vec;
		_positions[i] = Position(dirFactor*p.z, p.y, -dirFactor*p.x);
	}

	return *this;
}

MeshBuilder& MeshBuilder::rot90Z(RotateDirection dir) {
	if (_inGroup) {
		throw std::runtime_error("Cannot rotate while defining group");
	}

	float dirFactor = dir == RotateDirection::CCW ? 1.0f : -1.0f;

	for (size_t i = groupStart(); i < groupEnd(); i++) {
		const auto& p = _positions[i].vec;
		_positions[i] = Position(-dirFactor*p.y, dirFactor*p.x, p.z);
	}

	return *this;
}

bool MeshBuilder::modeHas(MeshBuilderMode check) const {
	return any(_mode & check);
}

MeshBuilder& MeshBuilder::cube(
	float size,
	const Position& center
) {
	if (modeHas(MeshBuilderMode::genLines)) {
		cubeLineIndices(
			_lineIndices,
			currentOffset()
		);
	}

	if (modeHas(MeshBuilderMode::genTris)) {
		cubeTriangleIndices(
			_triIndices,
			currentOffset()
		);
	}

	size_t start = _positions.size();
	cubePositions(_positions, size, center);
	size_t end = _positions.size();
	size_t diff = end - start;

	if (modeHas(MeshBuilderMode::genColors)) {
		for (int i = 0; i < diff; i++) {
			_colors.push_back(_color);
		}
	}

	return *this;
}

MeshBuilder& MeshBuilder::sphere(
	float radius,
	unsigned int horizontalResolution,
	unsigned int verticalResolution,
	const Position& center
) {
	if (modeHas(MeshBuilderMode::genLines)) {
		sphereLineIndices(
			_lineIndices,
			horizontalResolution,
			verticalResolution,
			currentOffset()
		);
	}

	if (modeHas(MeshBuilderMode::genTris)) {
		sphereTriangleIndices(
			_triIndices,
			horizontalResolution,
			verticalResolution,
			currentOffset()
		);
	}

	size_t start = _positions.size();
	spherePositions(_positions, radius, horizontalResolution, verticalResolution, center);
	size_t end = _positions.size();
	size_t diff = end - start;

	if (modeHas(MeshBuilderMode::genColors)) {
		for (int i = 0; i < diff; i++) {
			_colors.push_back(_color);
		}
	}

	return *this;
}

MeshBuilder& MeshBuilder::cylinder(
	float radius,
	float height,
	unsigned int horizontalResolution,
	unsigned int verticalResolution,
	const Position& center
) {
	if (modeHas(MeshBuilderMode::genLines)) {
		cylinderLineIndices(
			_lineIndices,
			horizontalResolution,
			verticalResolution,
			currentOffset()
		);
	}

	if (modeHas(MeshBuilderMode::genTris)) {
		cylinderTriangleIndices(
			_triIndices,
			horizontalResolution,
			verticalResolution,
			currentOffset()
		);
	}

	size_t start = _positions.size();
	cylinderPositions(_positions, radius, height, horizontalResolution, verticalResolution, center);
	size_t end = _positions.size();
	size_t diff = end - start;

	if (modeHas(MeshBuilderMode::genColors)) {
		for (int i = 0; i < diff; i++) {
			_colors.push_back(_color);
		}
	}

	return *this;
}

MeshBuilder& MeshBuilder::cone(
	float radius,
	float height,
	unsigned int horizontalResolution,
	unsigned int verticalResolution,
	const Position& center
) {
	if (modeHas(MeshBuilderMode::genLines)) {
		coneLineIndices(
			_lineIndices,
			horizontalResolution,
			verticalResolution,
			currentOffset()
		);
	}

	if (modeHas(MeshBuilderMode::genTris)) {
		coneTriangleIndices(
			_triIndices,
			horizontalResolution,
			verticalResolution,
			currentOffset()
		);
	}

	size_t start = _positions.size();
	conePositions(_positions, radius, height, horizontalResolution, verticalResolution, center);
	size_t end = _positions.size();
	size_t diff = end - start;

	if (modeHas(MeshBuilderMode::genColors)) {
		for (int i = 0; i < diff; i++) {
			_colors.push_back(_color);
		}
	}

	return *this;
}

StaticMesh MeshBuilder::lineMesh() {
	if (!modeHas(MeshBuilderMode::genLines)) {
		throw std::runtime_error("Cannot generate lineMesh() with genLines turned off");
	}

	return { *_renderer, _positions, _lineIndices };
}

StaticMesh MeshBuilder::triMesh() {
	if (!modeHas(MeshBuilderMode::genTris)) {
		throw std::runtime_error("Cannot generate triMesh() with genTris turned off");
	}

	return { *_renderer, _positions, _triIndices };
}

StaticVertexAttributes<Color> MeshBuilder::colors() {
	if (modeHas(MeshBuilderMode::genColors)) {
		return { *_renderer, _colors };
	}

	return solidColor(*_renderer, _positions.size(), _color);
}

StaticVertexAttributes<Normal> MeshBuilder::normals() {
	if (!modeHas(MeshBuilderMode::genTris)) {
		throw std::runtime_error("Cannot generate normals() with genTris turned off");
	}

	return { *_renderer, autoGenerateNormals(_positions, _triIndices) };
}
}
