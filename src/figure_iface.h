#pragma once

namespace g3d {
struct FigureRemovedEvent {
	unsigned int id;
};

class MathFigure {
public:
	virtual void update() {}
	virtual void updateSynchronized() {}
	virtual void draw() {}
	virtual unsigned int id() const = 0;

	virtual ~MathFigure() {}
};
}
