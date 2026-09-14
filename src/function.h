#pragma once

#include "expression.h"
#include "temporal.h"

namespace g3d {
class Function {
private:
	void handleVariableChanged(const VariableChangedEvent& e);

	CompoundMethodEventHandler<Function,
		VariableChangedEvent
	> _eventHandlers { this,
		std::mem_fn(handleVariableChanged)
	};

	TokenRegistry _tokenRegistry;
	VariableStore* _vars;
	TimePoint _startTime;
	std::unique_ptr<ParseNode> _parsedExpression;
	Flag _updated;
public:
	Function(VariableStore& vars)
	: _vars { &vars },
	  _tokenRegistry { makeTokenRegistry() },
	  _startTime { now() }
	{
		_eventHandlers.addToRouter(vars.eventRouter());
	}

	Function(Function&& other)
	: _vars { other._vars },
	  _tokenRegistry { std::move(other._tokenRegistry) },
	  _startTime { other._startTime },
	  _eventHandlers { std::move(other._eventHandlers) }
	{
		_eventHandlers.updateThis(this);
	}

	Flag& updated() { return _updated; }
	auto& vars() { return _vars; }

	float eval(float x, float y);
	void update();
	void updateExpression(const std::string& expression);
};
}
