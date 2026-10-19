#pragma once

#include "global_defines.h"
#include <glm/glm.hpp>

namespace g3d {
// Helper template to assist in generically implementing the common pattern of
// nested for loops used while generating model data for figures.
template<size_t S>
class MultidimensionalCounter {
private:
	std::array<size_t, S> _array;
	bool _atEnd;
	size_t _max;
	std::function<void(size_t, size_t)> _updateFunc;

public:
	void reset() {
		for (size_t i = 0; i < S; i++) {
			_array[i] = 0;
			_updateFunc(i, _array[i]);
		}

		_atEnd = false;
	}

	MultidimensionalCounter(
		size_t max,
		std::function<void(size_t, size_t)> updateFunc
	) : _max { max }, _updateFunc { updateFunc } {
		reset();
	}

	void inc() {
		for (size_t i = 0; i < S; i++) {
			if (_array[i] < _max) {
				_array[i]++;
				_updateFunc(i, _array[i]);
				return;
			}

			_array[i] = 0;
			_updateFunc(i, _array[i]);
		}

		_atEnd = true;
	}

	bool atEnd() const { return _atEnd; }
	size_t size() const { return S; }

	const size_t& operator[](size_t i) const {
		return _array[i];
	}
};

template<typename T>
concept GlmMixable = requires(T t1, T t2, float f) {
	{ glm::mix(t1, t2, f) } -> std::convertible_to<T>;
};

template<GlmMixable T, size_t DIMS>
FixedSizeBinaryTree<T, DIMS> multiLerp(
	const FixedSizeBinaryTree<T, DIMS>& low,
	const FixedSizeBinaryTree<T, DIMS>& high,
	float factor
) {
	FixedSizeBinaryTree<T, DIMS> out;

	out.left = multiLerp(low.left, high.left, factor);
	out.right = multiLerp(low.right, high.right, factor);

	return out;
}

template<GlmMixable T>
FixedSizeBinaryTree<T, 0> multiLerp(
	const FixedSizeBinaryTree<T, 0>& low,
	const FixedSizeBinaryTree<T, 0>& high,
	float factor
) {
	return { glm::mix(low.value, high.value, factor) };
}

template<GlmMixable T, size_t DIMS>
FixedSizeBinaryTree<T, DIMS-1> multiLerpLeftRight(
	const FixedSizeBinaryTree<T, DIMS>& tree,
	float factor
) {
	return multiLerp(tree.left, tree.right, factor);
}

template<GlmMixable T, size_t DIMS, size_t N>
class _MultidimensionalLerpCursor {
	template<GlmMixable U, size_t DIMS_OTHER, size_t N_OTHER>
	friend class _MultidimensionalLerpCursor;

private:
	FixedSizeBinaryTree<T, N> _cursor;
	_MultidimensionalLerpCursor<T, DIMS, N+1> _nextDim;

public:
	void update(size_t dim, float factor) {
		if (dim == N) {
			_cursor = multiLerpLeftRight(_nextDim._cursor, factor);
		} else {
			_nextDim.update(dim, factor);
			_cursor = multiLerpLeftRight(_nextDim._cursor, 0.0f);
		}

	}

	_MultidimensionalLerpCursor(const FixedSizeBinaryTree<T, DIMS>& originalRange)
	: _nextDim { originalRange }
	{}
};

template<GlmMixable T, size_t DIMS>
class _MultidimensionalLerpCursor<T, DIMS, 0> {
	template<GlmMixable U, size_t DIMS_OTHER, size_t N_OTHER>
	friend class _MultidimensionalLerpCursor;

private:
	T _cursor;
	_MultidimensionalLerpCursor<T, DIMS, 1> _nextDim;

public:
	void update(size_t dim, float factor) {
		if (dim == 0) {
			_cursor = multiLerpLeftRight(_nextDim._cursor, factor).value;
		} else {
			_nextDim.update(dim, factor);
			_cursor = multiLerpLeftRight(_nextDim._cursor, 0.0f).value;
		}
	}

	const T& value() { return _cursor; }

	_MultidimensionalLerpCursor(const FixedSizeBinaryTree<T, DIMS>& originalRange)
	: _nextDim { originalRange } {
		// update() dim=DIMS will cascade all the way up to N=DIMS and
		// set every intermediate cursor to its 0.0f lerp state.
		update(DIMS, 0.0f);
	}
};

template<GlmMixable T, size_t DIMS>
struct _MultidimensionalLerpCursor<T, DIMS, DIMS> {
	// Original range. Called _cursor here for compatibility with N=DIMS-1
	FixedSizeBinaryTree<T, DIMS> _cursor;

	void update(size_t dim, float factor) {}

	_MultidimensionalLerpCursor(const FixedSizeBinaryTree<T, DIMS>& originalRange)
	: _cursor { originalRange } {}
};

template<GlmMixable T, size_t DIMS>
using MultidimensionalLerpCursor = _MultidimensionalLerpCursor<T, DIMS, 0>;
}
