#pragma once

#include "expression.h"
#include "temporal.h"

namespace g3d {
class Function {
private:
	class _VariableChangedHandler : public EventHandler<VariableChangedEvent> {
	private:
		Function* _this;
	public:
		void handle(const VariableChangedEvent& e) override;
		_VariableChangedHandler(Function* _this) : _this { _this } {}
	};

	TokenRegistry _tokenRegistry;
	VariableStore* _vars;
	TimePoint _startTime;
	std::unique_ptr<ParseNode> _parsedExpression;
	bool _animated = false;
	bool _updated = false;

	_VariableChangedHandler _varChangedHandler { this };
public:
	Function(VariableStore& vars)
	: _vars { &vars },
	  _tokenRegistry { makeTokenRegistry() },
	  _startTime { now() }
	{
		vars.eventRouter().addHandler(_varChangedHandler);
	}

	bool animated() const { return _animated; }
	bool updated() const { return _updated; }
	void setUpdated() { _updated = true; }
	void resetUpdated() { _updated = false; }
	auto& vars() { return _vars; }

	float eval(float x, float y);
	void update();
	void updateAnimated();
	void updateExpression(const std::string& expression);
};
}
