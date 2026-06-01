#pragma once
#include "global_defines.h"

#include <format>
#include <memory>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/trigonometric.hpp>

namespace g3d {
constexpr char ADD_CHAR = '+';
constexpr char SUB_CHAR = '-';
constexpr char DIV_CHAR = '/';
constexpr char MUL_CHAR = '*';
constexpr char EXP_CHAR = '^';
constexpr char GT_CHAR = '>';
constexpr char LT_CHAR = '<';
constexpr char EQ_CHAR = '=';
constexpr char LPAREN = '(';
constexpr char RPAREN = ')';
constexpr char DEC_POINT = '.';

bool isAlpha(char c);

class VariableStore {
private:
	std::unordered_map<char, float> _vars;

public:
	void set(char c, float val);
	float get(char c);
};

struct Token;

template<typename... Args>
class AbstractTokenFactory {
public:
	virtual std::unique_ptr<Token> make(Args&&... args) const = 0;
};

template<typename T, typename... Args>
class TokenFactory : public AbstractTokenFactory<Args...> {
public:
	std::unique_ptr<Token> make(Args&&... args) const override {
		return std::make_unique<T>(std::forward<Args>(args)...);
	}
};

class TokenRegistry {
private:
	std::unordered_map<std::string, std::unique_ptr<AbstractTokenFactory<>>> symbols;

public:
	template<typename T>
	void registerSymbol() {
		std::string s { T::s };
		std::cerr << "TokenRegistry: register symbol \"" << s << '"' << std::endl;
		auto factory = std::make_unique<TokenFactory<T>>();
		symbols.emplace(std::move(s), std::move(factory));
	}

	std::unique_ptr<Token> makeSymbol(const std::string& s);
};

class ParseNode {
private:
	std::vector<ParseNode> _children;
	std::unique_ptr<Token> _token;

public:
	ParseNode() = delete;
	ParseNode(const ParseNode&) = delete;
	ParseNode(ParseNode&&) = default;
	ParseNode(std::unique_ptr<Token>&& token) : _token { std::move(token) } {}
	ParseNode(
		std::unique_ptr<Token>&& token,
		ParseNode&& child
	) : _token { std::move(token) } {
		_children.push_back(std::move(child));
	}
	ParseNode(
		std::unique_ptr<Token>&& token,
		ParseNode&& child1,
		ParseNode&& child2
	) : _token { std::move(token) } {
		_children.push_back(std::move(child1));
		_children.push_back(std::move(child2));
	}

	ParseNode& operator=(const ParseNode&) = delete;
	ParseNode& operator=(ParseNode&& other) = default;

	float eval() const;
	bool hasTokenStr(const std::string& s) const;
	std::string toString();
};

class Parser;

struct Token {
	// Left binding power
	unsigned int lbp = 0;

	// String value. Not all tokens have one.
	std::string str;

	Token() = default;
	Token(const std::string& s) : str { s } {}
	Token(char c) : str { c } {}

	// Null denotation
	virtual ParseNode nud(Parser& parser);

	// Left denotation
	virtual ParseNode led(Parser& parser, ParseNode&& left);

	// Used for evaluating parse trees
	virtual float eval(const std::vector<ParseNode>& children) const;

	virtual std::string toString() { return str; }
};

struct ParseIntResult {
	unsigned int val = 0;
	unsigned int digits = 0;
};

class Tokenizer {
private:
	std::string _expression;
	size_t _position = 0;
	bool _atEnd = false;
	VariableStore* _vars;
	TokenRegistry* _registry;
public:
	Tokenizer(
		TokenRegistry& registry,
		VariableStore& vars,
		const std::string& expression
	)
	: _registry { &registry },
	  _vars { &vars},
	  _expression { expression }
	{}

	bool atEnd() { return _atEnd; }

	ParseIntResult parseInt();
	float parseFloat();
	std::string parseAlphaString();
	std::unique_ptr<Token> next();
};

class Parser {
private:
	Tokenizer _tokenizer;
	std::unique_ptr<Token> _cur;
	std::unique_ptr<Token> _prev;

	void nextToken();
public:
	Parser(
		TokenRegistry& registry,
		VariableStore& vars,
		const std::string& expression
	)
	: _tokenizer { registry, vars, expression } {}

	ParseNode expression(unsigned int rbp = 0);
	void expect(const std::string& c);
	ParseNode parse();
};

struct EndToken : public Token {
	EndToken() : Token("{{ END }}") {}
};

struct LiteralToken : public Token {
	float value;

	LiteralToken(float value) : value { value } { lbp = 0; };

	ParseNode nud(Parser& parser) override;
	float eval(const std::vector<ParseNode>& children) const override;
	std::string toString() override;
};

struct VarToken : public Token {
private:
	VariableStore* _vars;
public:
	VarToken(VariableStore& vars, char _c) : Token(_c),  _vars { &vars } { lbp = 0; }

	ParseNode nud(Parser& parser) override;
	float eval(const std::vector<ParseNode>& children) const override;
	std::string toString() override;
};

struct StartParenToken : public Token {
	static constexpr char s[] { "(" };
	StartParenToken() : Token(s) {}

	ParseNode nud(Parser& parser) override;
};

struct EndParenToken : public Token {
	static constexpr char s[] = { ")" };
	EndParenToken() : Token(s) {}
};

struct FuncSin {
	static constexpr char s[] { "sin" };
	static inline float op(float x) { return glm::sin(x); }
};

struct FuncCos {
	static constexpr char s[] { "cos" };
	static inline float op(float x) { return glm::cos(x); }
};

struct FuncTan {
	static constexpr char s[] { "tan" };
	static inline float op(float x) { return glm::tan(x); }
};

struct FuncExp {
	static constexpr char s[] { "exp" };
	static inline float op(float x) { return glm::exp(x); }
};

struct FuncLog {
	static constexpr char s[] { "log" };
	static inline float op(float x) { return glm::log(x); }
};

struct FuncAbs {
	static constexpr char s[] { "abs" };
	static inline float op(float x) { return glm::abs(x); }
};

template<typename T>
struct BuiltinFuncToken : public Token {
	static constexpr auto s { T::s };
	BuiltinFuncToken() : Token(s) { lbp = 110; }

	ParseNode nud(Parser& parser) override {
		return {
			std::make_unique<BuiltinFuncToken<T>>(*this),
			parser.expression(lbp-1)
		};
	}

	float eval(const std::vector<ParseNode>& children) const override {
		return T::op(children.at(0).eval());
	}

	std::string toString() override { return std::format("FUNC:{}", str); }
};

struct OpAdd {
	static constexpr char s[] { "+" };
	static constexpr unsigned int lbp = 20;
	static constexpr unsigned int lbpSub = 0;
	static constexpr bool allowUnary = true;
	static constexpr unsigned int unaryLbp = 100;

	static inline float opUnary(float x) { return x; }
	static inline float op(float a, float b) { return a + b; }
};

struct OpSub {
	static constexpr char s[] { "-" };
	static constexpr unsigned int lbp = 20;
	static constexpr unsigned int lbpSub = 0;
	static constexpr bool allowUnary = true;
	static constexpr unsigned int unaryLbp = 100;

	static inline float opUnary(float x) { return -x; }
	static inline float op(float a, float b) { return a - b; }
};

struct OpMul {
	static constexpr char s[] { "*" };
	static constexpr unsigned int lbp = 30;
	static constexpr unsigned int lbpSub = 0;
	static constexpr bool allowUnary = false;

	static inline float op(float a, float b) { return a * b; }
};

struct OpDiv {
	static constexpr char s[] { "/" };
	static constexpr unsigned int lbp = 30;
	static constexpr unsigned int lbpSub = 0;
	static constexpr bool allowUnary = false;

	static inline float op(float a, float b) { return a / b; }
};

struct OpMod {
	static constexpr char s[] { "%" };
	static constexpr unsigned int lbp = 30;
	static constexpr unsigned int lbpSub = 0;
	static constexpr bool allowUnary = false;

	static inline float op(float a, float b) { return glm::mod(a, b); }
};

struct OpExp {
	static constexpr char s[] { "^" };
	static constexpr unsigned int lbp = 40;
	static constexpr unsigned int lbpSub = 1; // makes right-associative
	static constexpr bool allowUnary = false;

	static inline float op(float a, float b) { return glm::pow(a, b); }
};

struct OpGt {
	static constexpr char s[] { ">" };
	static constexpr unsigned int lbp = 10;
	static constexpr unsigned int lbpSub = 0;
	static constexpr bool allowUnary = false;

	static inline float op(float a, float b) { return static_cast<float>(a > b); }
};

struct OpLt {
	static constexpr char s[] { "<" };
	static constexpr unsigned int lbp = 10;
	static constexpr unsigned int lbpSub = 0;
	static constexpr bool allowUnary = false;

	static inline float op(float a, float b) { return static_cast<float>(a < b); }
};

struct OpEq {
	static constexpr char s[] { "=" };
	static constexpr unsigned int lbp = 10;
	static constexpr unsigned int lbpSub = 0;
	static constexpr bool allowUnary = false;

	static inline float op(float a, float b) { return static_cast<float>(a == b); }
};

template<typename T>
struct BinaryOpToken : public Token {
	static constexpr auto s { T::s };
	BinaryOpToken() : Token(s) { lbp = T::lbp; }

	ParseNode nud(Parser& parser) override {
		if constexpr (T::allowUnary) {
			return {
				std::make_unique<BinaryOpToken<T>>(*this),
				parser.expression(T::unaryLbp)
			};
		} else {
			std::string msg = std::format(
				"Binary operator \"{}\" cannot be used as unary",
				T::s
			);
			throw std::runtime_error(msg);
		}
	}

	ParseNode led(Parser& parser, ParseNode&& left) override {
		return {
			std::make_unique<BinaryOpToken<T>>(*this),
			std::move(left),
			parser.expression(lbp-T::lbpSub)
		};
	}

	float eval(const std::vector<ParseNode>& children) const override {
		if constexpr (!T::allowUnary) {
			return T::op(children.at(0).eval(), children.at(1).eval());
		} else {
			if (children.size() == 1) {
				return T::opUnary(children.at(0).eval());
			} else {
				return T::op(children.at(0).eval(), children.at(1).eval());
			}
		}
	}
};

TokenRegistry makeTokenRegistry();

void tokenizerTest(const std::string& expression);
void parserTest(const std::string& expression);
}
