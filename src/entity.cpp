#include "entity.h"

namespace g3d {
void Component::setOwner(Entity& entity) {
	_owner = &entity;
}

Entity& Component::entity() {
	if (_owner == nullptr) {
		throw std::runtime_error("Component has no owner");
	}

	return *_owner;
}

void Entity::draw() {
	if (!active) return;

	for (auto& [idx, component] : _components) {
		component->draw();
	}
}

void Entity::render() {
	if (!active) return;

	for (auto& [idx, component] : _components) {
		if (component.get() == _lastRender) continue;
		component->render();
	}

	if (_lastRender != nullptr) _lastRender->render();
}

void Entity::update() {
	if (!active) return;

	for (auto& [idx, component] : _components) {
		component->update();
	}
}
};
