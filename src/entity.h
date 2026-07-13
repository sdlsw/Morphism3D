#pragma once

#include <concepts>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

namespace g3d {
class Entity;

class Component {
	friend class Entity;

private:
	Entity* _owner = nullptr;

	void setOwner(Entity& entity);
public:
	// Draw calls should signal to the renderer that a component should be
	// rendered this frame.
	virtual void draw() {};

	// Updates the state of the component.
	virtual void update() {};

	// Render calls are called by the renderer when rendering an entity.
	// Should generally not be called outside the renderer.
	virtual void render() {};

	Entity& entity();
};

class Entity {
private:
	// Only one component of each type may be added.
	//
	// I'm divided on whether to make this a map or a simple vector.
	// Making it a vector could solve the render() call ordering issue, but
	// only if I don't allow further components that override render() to
	// be added. That's not exactly an obvious restriction and I might
	// accidentally step on it later.
	std::unordered_map<std::type_index, std::unique_ptr<Component>> _components;

	// One component can optionally be set to defer its render() call until
	// the end of Entity::render(). Used for mesh components that actually
	// run a draw call.
	Component* _lastRender = nullptr;
public:
	Entity() = default;

	// No copying entities.
	Entity(Entity&& entity) = default;

	bool active = true;

	void draw();
	void render();
	void update();

	template<std::derived_from<Component> C, typename... Args>
	void addComponent(Args&&... args) {
		auto component = std::make_unique<C>(std::forward<Args>(args)...);
		component.get()->setOwner(*this);
		_components.emplace(std::type_index(typeid(*component.get())), std::move(component));
	}

	template<std::derived_from<Component> C>
	C* getComponent() {
		auto idx = std::type_index(typeid(C));
		if (_components.contains(idx)) {
			return static_cast<C*>(_components.at(idx).get());
		}

		return nullptr;
	}

	template<std::derived_from<Component> C>
	C& requireComponent() {
		auto* ptr = getComponent<C>();
		if (ptr == nullptr) {
			// TODO: Better error message
			throw std::runtime_error("Missing required component!");
		}

		return *ptr;
	}

	template<std::derived_from<Component> C>
	void setLastRender() {
		auto* ptr = getComponent<C>();
		if (ptr == nullptr) {
			std::cerr << "warning: Attempt to setLastRender() on nonexistent component" << std::endl;
		}

		_lastRender = ptr;
	}
};
};
