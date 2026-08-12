#pragma once

#include "type.h"

#include <cstdint>
#include <iostream>
#include <typeindex>
#include <stdexcept>
#include <unordered_map>

namespace g3d {
constexpr uint64_t LINKED_NO_ID = 0;

template<typename T>
concept LinkPoint = requires(T linkPoint, uint64_t id) {
	{ linkPoint.deleteLink(id) };
};

template<typename T>
class Linked {
	friend T;

private:
	uint64_t _id = LINKED_NO_ID;
	T* _linkPoint = nullptr;

	void link(T* linkPoint, uint64_t id) {
		_id = id;
		_linkPoint = linkPoint;
	}

	void unlink() {
		_id = LINKED_NO_ID;
		_linkPoint = nullptr;
	}

	bool isLinked() { return _linkPoint != nullptr; }

public:
	virtual ~Linked() {
		if (isLinked()) {
			std::cerr << "~Linked(): id " << _id << std::endl;
			_linkPoint->deleteLink(_id);
			unlink();
		} else {
			std::cerr << "~Linked() called on unlinked" << std::endl;
		}
	}

	Linked() = default;
	Linked(const Linked& other) = delete;

	// FIXME gross gross gross gross
	Linked(Linked&& other) : _id { other._id }, _linkPoint { other._linkPoint } {
		std::cerr << "Linked(Linked&&): moving..." << std::endl;
		_linkPoint->updateLink(_id, this);
		other.unlink();
	}
};

class EventRouter;

template<typename E>
class EventHandler : public Linked<EventRouter> {
public:
	virtual void handle(const E& event) = 0;
};

class EventRouter {
	friend Linked<EventRouter>;

private:
	uint64_t _nextId = LINKED_NO_ID + 1;

	// Tracks event handlers by event type, for more efficient
	// event routing. Using Linked<> pointers here is a bit nasty, but it means
	// it's not required to know the handler types in advance.
	//
	// EventHandler<> pointers are not being used because these tables need
	// to be modified by Linked<> in the case of move operations, so I use
	// those pointers for handler identity.
	std::unordered_map<std::type_index, std::unordered_map<uint64_t, Linked<EventRouter>*>> _typemap;

	// Tracks event handlers and their corresponding type by ID. Used for
	// automatic unlinking.
	std::unordered_map<uint64_t, std::pair<Linked<EventRouter>*, std::type_index>> _idmap;

	// Handles automatic unlinking behavior when Linked<EventRouter> goes
	// out of scope.
	void deleteLink(uint64_t id);

	// When an object containing an event handler is moved, the pointer
	// must be updated.
	void updateLink(uint64_t id, Linked<EventRouter>* newPtr);

public:
	~EventRouter() {
		for (auto& [_, submap] : _typemap) {
			for (auto& [id, ptr] : submap) {
				ptr->unlink();
			}
		}
	}

	template<typename E>
	void addHandler(EventHandler<E>& handler) {
		if (handler.isLinked()) {
			throw std::runtime_error("Cannot add handler; already linked.");
		}

		auto type_index = std::type_index(typeid(E));

		handler.link(this, _nextId);
		_typemap[type_index][_nextId] = &handler;
		auto p = std::pair<Linked<EventRouter>*, std::type_index>(
			dynamic_cast<Linked<EventRouter>*>(&handler), type_index
		);
		_idmap.emplace(_nextId, std::move(p));
		_nextId++;

		std::cerr <<
			"Added EventHandler of type " << prettyType<E>() <<
			" id " << _nextId-1 <<
			" addr " << &handler <<
			std::endl;
	}

	// Note: This operation will be performed automatically if the handler
	// goes out of scope.
	template<typename E>
	void removeHandler(EventHandler<E>& handler) {
		if (!handler.isLinked()) {
			throw std::runtime_error("Cannot remove handler; not linked");
		}

		if (handler._linkPoint != this) {
			throw std::runtime_error("Cannot remove handler linked to another router");
		}

		std::cerr <<
			"Removed EventHandler of type " << prettyType<E>() <<
			" id " << handler._id <<
			" addr " << &handler <<
			std::endl;

		deleteLink(handler._id);
		handler.unlink();
	}

	template<typename E>
	void routeEvent(const E& event) {
		auto type_index = std::type_index(typeid(E));

		if (_typemap.contains(type_index)) {
			for (auto& [_, ptr] : _typemap[type_index]) {
				dynamic_cast<EventHandler<E>*>(ptr)->handle(event);
			}
		}

		// Event types with no known handlers are just skipped
	}
};

template<typename T>
concept Printable = requires(std::ostream out, T printable) {
	{ out << printable } -> std::convertible_to<std::ostream&>;
};

// Utility template for debug purposes.
template<Printable E>
class EventPrinter : public EventHandler<E> {
public:
	bool enabled = false;

	void handle(const E& e) override {
		if (!enabled) return;
		std::cerr << prettyType<E>() << ": " << e << std::endl;
	}

	EventPrinter(EventRouter& router) {
		router.addHandler(*this);
	}
};
}
