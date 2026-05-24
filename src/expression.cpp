#include "expression.h"

#include <cmath>
#include <iostream>

bool isDigit(char c) {
	return c >= '0' && c <= '9';
}

unsigned int toDigit(char c) {
	return static_cast<unsigned int>(c - '0');
}

bool isAlpha(char c) {
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

namespace g3d {
float ParseNode::eval() const {
	return _token.get()->eval(_children);
}

std::string ParseNode::toString() {
	if (_children.size() == 0) {
		return std::format("({})", _token.get()->toString());
	} else if (_children.size() == 1) {
		return std::format("({} {})", _token.get()->toString(), _children[0].toString());
	} else if (_children.size() == 2) {
		return std::format(
			"({} {} {})",
			_token.get()->toString(),
			_children[0].toString(),
			_children[1].toString()
		);
	} else {
		throw std::runtime_error("Bad _children size");
	}
}

ParseIntResult Tokenizer::parseInt() {
	ParseIntResult result;
	while (_position < _expression.size() && isDigit(_expression[_position])) {
		result.val *= 10;
		result.val += toDigit(_expression[_position]);
		_position++;
		result.digits++;
	}

	return result;
}

float Tokenizer::parseFloat() {
	// We expect to be on an int already, so parse it
	float whole = static_cast<float>(parseInt().val);

	// If no decimal point, just return whole number
	if (_position >= _expression.size() || _expression[_position] != DEC_POINT) {
		return whole;
	}

	// Found dec point, so grab fractional part
	_position++;
	if (_position >= _expression.size()) {
		throw std::runtime_error("Unexpected end of expression while parsing float literal");
	}

	if (!isDigit(_expression[_position])) {
		throw std::runtime_error("Non-digit character after decimal point");
	}

	auto fracResult = parseInt();
	float frac = fracResult.val * std::pow(10.0f, -static_cast<float>(fracResult.digits));

	return whole + frac;
}

std::string Tokenizer::parseAlphaString() {
	std::string s;

	while (_position < _expression.size() && isAlpha(_expression[_position])) {
		s.push_back(_expression[_position]);
		_position++;
	}

	return s;
}

std::unique_ptr<Token> Tokenizer::makeFunction(std::string&& s) {
	if (s == "sin") {
		return std::make_unique<FuncToken<FuncSin>>(std::move(s));
	} else if (s == "cos") {
		return std::make_unique<FuncToken<FuncCos>>(std::move(s));
	} else if (s == "exp") {
		return std::make_unique<FuncToken<FuncExp>>(std::move(s));
	}

	throw std::runtime_error(std::format("Unsupported function \"{}\"", s));
}

std::unique_ptr<Token> Tokenizer::next() {
	// skip whitespace
	while (_position < _expression.size() && _expression[_position] == ' ') {
		_position++;
	}

	if (_position >= _expression.size()) {
		_atEnd = true;
		return std::make_unique<EndToken>();
	}

	char curChar = _expression[_position];

	if (isDigit(curChar)) {
		return std::make_unique<LiteralToken>(parseFloat());
	}

	if (isAlpha(curChar)) {
		std::string s = parseAlphaString();

		if (s.size() == 1) {
			return std::make_unique<VarToken>(*_vars, s[0]);
		} else {
			return makeFunction(std::move(s));
		}
	}

	_position++;
	switch (curChar) {
		case LPAREN:
			return std::make_unique<StartParenToken>();
		case RPAREN:
			return std::make_unique<EndParenToken>();
		case OpAdd::c:
			return std::make_unique<BinaryOpToken<OpAdd>>();
		case OpSub::c:
			return std::make_unique<BinaryOpToken<OpSub>>();
		case OpMul::c:
			return std::make_unique<BinaryOpToken<OpMul>>();
		case OpDiv::c:
			return std::make_unique<BinaryOpToken<OpDiv>>();
		case OpExp::c:
			return std::make_unique<BinaryOpToken<OpExp>>();
		case OpGt::c:
			return std::make_unique<BinaryOpToken<OpGt>>();
		case OpLt::c:
			return std::make_unique<BinaryOpToken<OpLt>>();
		case OpEq::c:
			return std::make_unique<BinaryOpToken<OpEq>>();
		default:
			throw std::runtime_error(
				"Could not tokenize expression, invalid character"
			);
	}
}

void Parser::nextToken() {
	_prev = std::move(_tokenizer.next());
	_prev.swap(_cur);
}

ParseNode Parser::expression(unsigned int rbp) {
	nextToken();
	ParseNode left = _prev.get()->nud(*this);
	while (rbp < _cur.get()->lbp) {
		nextToken();
		left = std::move(_prev.get()->led(*this, std::move(left)));
	}

	return left;
}

void Parser::expect(char c) {
	if (c != _cur.get()->c) {
		throw std::runtime_error(std::format("Expected \"{}\"", c));
	}
	nextToken();
}

ParseNode Parser::parse() {
	_cur = std::move(_tokenizer.next());
	return expression();
}

void tokenizerTest(const std::string& expression) {
	VariableRegistry vars {};
	Tokenizer t { vars, expression };

	std::cout << "Tokenizer test: " << expression << std::endl;
	try {
		while (!t.atEnd()) {
			auto tok = t.next();
			std::cerr << tok.get()->toString() << std::endl;
		}
	} catch (const std::exception& e) {
		std::cerr << "Tokenize failure. Reason:" << std::endl;
		std::cerr << e.what() << std::endl;
	}
}

void parserTest(const std::string& expression) {
	VariableRegistry vars {};
	Parser p { vars, expression };

	// Populate with some test variables
	vars.set('x', 1.0f);
	vars.set('y', -1.0f);

	std::cout << "Parser test: " << expression << std::endl;
	try {
		auto node = p.parse();
		std::cout << node.toString() << std::endl;
		std::cout << node.eval() << std::endl;
	} catch (const std::exception& e) {
		std::cerr << "Parse failure. Reason:" << std::endl;
		std::cerr << e.what() << std::endl;
	}
}
}
