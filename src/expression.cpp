#include "expression.h"

#include <cmath>
#include <iostream>

bool isDigit(char c) {
	return c >= '0' && c <= '9';
}

unsigned int toDigit(char c) {
	return static_cast<unsigned int>(c - '0');
}

namespace g3d {
bool isAlpha(char c) {
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

TokenRegistry makeTokenRegistry() {
	TokenRegistry r;

	r.registerSymbol<StartParenToken>();
	r.registerSymbol<EndParenToken>();

	// Binary operators
	r.registerSymbol<BinaryOpToken<OpAdd>>();
	r.registerSymbol<BinaryOpToken<OpSub>>();
	r.registerSymbol<BinaryOpToken<OpMul>>();
	r.registerSymbol<BinaryOpToken<OpDiv>>();
	r.registerSymbol<BinaryOpToken<OpMod>>();
	r.registerSymbol<BinaryOpToken<OpExp>>();
	r.registerSymbol<BinaryOpToken<OpGt>>();
	r.registerSymbol<BinaryOpToken<OpLt>>();
	r.registerSymbol<BinaryOpToken<OpEq>>();

	// Built in functions
	r.registerSymbol<BuiltinFuncToken<FuncSin>>();
	r.registerSymbol<BuiltinFuncToken<FuncCos>>();
	r.registerSymbol<BuiltinFuncToken<FuncTan>>();
	r.registerSymbol<BuiltinFuncToken<FuncExp>>();
	r.registerSymbol<BuiltinFuncToken<FuncLog>>();
	r.registerSymbol<BuiltinFuncToken<FuncAbs>>();

	return r;
}

void VariableStore::set(char c, float val) {
	_vars[c] = val;
}

float VariableStore::get(char c) {
	return _vars[c];
}

std::unique_ptr<Token> TokenRegistry::makeSymbol(const std::string& s) {
	if (!symbols.contains(s)) {
		return {};
	}

	return symbols.at(s).get()->make();
}

float ParseNode::eval() const {
	return _token.get()->eval(_children);
}

bool ParseNode::hasTokenStr(const std::string& s) const {
	if (_token && _token.get()->str == s) {
		return true;
	}

	for (const auto& child : _children) {
		if (child.hasTokenStr(s)) return true;
	}

	return false;
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

ParseNode Token::nud(Parser& parser) {
	throw std::runtime_error(
		std::format("\"{}\": nud() not implemented", str)
	);
}

ParseNode Token::led(Parser& parser, ParseNode&& left) {
	throw std::runtime_error(
		std::format("\"{}\": led() not implemented", str)
	);
}

float Token::eval(const std::vector<ParseNode>& children) const {
	throw std::runtime_error(
		std::format("\"{}\": eval() not implemented", str)
	);
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

	std::string s;

	if (isAlpha(curChar)) {
		s = parseAlphaString();

		if (s.size() == 1) {
			return std::make_unique<VarToken>(*_vars, s[0]);
		}
	} else {
		s.push_back(curChar);
		_position++;
	}

	auto symbol = _registry->makeSymbol(s);
	if (!symbol) {
		throw std::runtime_error(std::format(
			"Could not tokenize expression, invalid symbol {}", s
		));
	} else {
		return std::move(symbol);
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

void Parser::expect(const std::string& s) {
	if (s != _cur.get()->str) {
		throw std::runtime_error(std::format("Expected \"{}\"", s));
	}
	nextToken();
}

ParseNode Parser::parse() {
	_cur = std::move(_tokenizer.next());
	return expression();
}

// SPECIAL TOKEN IMPLEMENTATIONS

ParseNode LiteralToken::nud(Parser& parser) {
	return { std::make_unique<LiteralToken>(*this) };
}

float LiteralToken::eval(const std::vector<ParseNode>& children) const {
	return value;
}

std::string LiteralToken::toString() {
	return std::format("LITERAL:{}", value);
}

ParseNode VarToken::nud(Parser& parser) {
	return { std::make_unique<VarToken>(*this) };
}

float VarToken::eval(const std::vector<ParseNode>& children) const {
	return _vars->get(str[0]);
}

std::string VarToken::toString() {
	return std::format("VAR:{}", str);
}

ParseNode StartParenToken::nud(Parser& parser) {
	ParseNode node = parser.expression();
	parser.expect({RPAREN});
	return node;
}

// TESTS

void tokenizerTest(const std::string& expression) {
	VariableStore vars {};
	auto registry = makeTokenRegistry();
	Tokenizer t { registry, vars, expression };

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
	VariableStore vars {};
	auto registry = makeTokenRegistry();
	Parser p { registry, vars, expression };

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
