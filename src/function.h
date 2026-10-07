#pragma once

#include "expression.h"
#include "temporal.h"

#include <span>

namespace g3d {
template<size_t IN_DIMS, size_t OUT_DIMS>
class Function {
private:
	void handleVariableChanged(const VariableChangedEvent& event) {
		char s[] { '\0', '\0' };
		s[0] = event.c;

		for (size_t i = 0; i < OUT_DIMS; i++) {
			if (_parsedExpressions[i]->hasTokenStr(s)) _updated.set();
		}
	}

	CompoundMethodEventHandler<Function,
		VariableChangedEvent
	> _eventHandlers { this,
		std::mem_fn(handleVariableChanged)
	};

	TokenRegistry _tokenRegistry;
	VariableStore* _vars;

	std::array<float, IN_DIMS> _inputVars;
	std::array<std::unique_ptr<ParseNode>, OUT_DIMS> _parsedExpressions;

	Flag _updated;
public:
	Function(VariableStore& vars, std::span<const char, IN_DIMS> inputVars)
	: _tokenRegistry { makeTokenRegistry() },
	  _vars { &vars }
	{
		_eventHandlers.addToRouter(vars.eventRouter());

		// Use overrideVar to speed up manipulation of the rapidly
		// changing input variables. This also means that the input
		// variables won't mangle the variable store.
		for (size_t i = 0; i < IN_DIMS; i++) {
			_tokenRegistry.overrideVar(inputVars[i], &_inputVars[i]);
		}

		// Initialize expressions to dummy values that always evaluate
		// to zero, so that the unique_ptrs are never null.
		for (auto& expression : _parsedExpressions) {
			expression.reset(new ParseNode(emptyExpression()));
		}
	}

	Function(Function&& other)
	: _eventHandlers { std::move(other._eventHandlers) },
	  _tokenRegistry { std::move(other._tokenRegistry) },
	  _vars { other._vars },
	  _inputVars { std::move(other._inputVars) },
	  _parsedExpressions { std::move(other._parsedExpressions) }
	{
		_eventHandlers.updateThis(this);
	}

	Flag& updated() { return _updated; }
	auto& vars() { return _vars; }

	std::array<float, OUT_DIMS> eval(std::span<const float, IN_DIMS> inputs) {
		for (size_t i = 0; i < IN_DIMS; i++) {
			_inputVars[i] = inputs[i];
		}

		std::array<float, OUT_DIMS> out;
		for (size_t j = 0; j < OUT_DIMS; j++) {
			out[j] = _parsedExpressions[j]->eval();
		}

		return out;
	}

	void updateExpression(size_t n, const std::string& expression) {
		Parser p { _tokenRegistry, *_vars, expression };

		std::cerr << "Updating expression #" << n << " to \"" << expression << "\"... ";

		try {
			_parsedExpressions[n].reset(new ParseNode(p.parse()));
			_updated.set();

			std::cerr << "Success." << std::endl;
		} catch (const std::exception& e) {
			std::cerr << "Failed! Reason: " << std::endl;
			std::cerr << e.what() << std::endl;
		}
	}
};
}
