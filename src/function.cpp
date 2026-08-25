#include "function.h"

namespace g3d {
void Function::_VariableChangedHandler::handle(const VariableChangedEvent& event) {
	char s[] { '\0', '\0' };
	s[0] = event.c;

	if (_this->_parsedExpression && _this->_parsedExpression.get()->hasTokenStr(s)) {
		_this->_updated = true;
	}
}

float Function::eval(float x, float y) {
	if (!_parsedExpression) {
		return 0.0f;
	}

	// Don't want to send event for x/y changes
	// TODO find a less hacky way to do this that doesn't involve updating
	// the variable store on every eval. Other graphs are going to need to
	// share this.
	_vars->set('x', x, false);
	_vars->set('y', y, false);
	return _parsedExpression.get()->eval();
}

void Function::update() {
	_vars->set('t', secondsSince(_startTime));
}

void Function::updateAnimated() {
	_animated = (_parsedExpression && _parsedExpression.get()->hasTokenStr("t"));
}

void Function::updateExpression(const std::string& expression) {
	Parser p { _tokenRegistry, *_vars, expression };

	try {
		_parsedExpression.reset(new ParseNode(p.parse()));
		updateAnimated();
		_updated = true;

		std::cerr << "Expression updated: " << expression << std::endl;
		std::cerr << "Animated: " << _animated << std::endl;
	} catch (const std::exception& e) {
		std::cerr << "Failed to parse expression: " << expression << std::endl;
		std::cerr << e.what() << std::endl;
	}
}
}
