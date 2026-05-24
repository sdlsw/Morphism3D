#pragma once

#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

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

class VariableRegistry {
private:
	std::unordered_map<char, float> _vars;

public:
	void set(char c, float val) {
		_vars[c] = val;
	}

	float get(char c) {
		return _vars[c];
	}
};

struct Token;

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
	std::string toString();
};

class Parser;

struct Token {
	// Left binding power
	unsigned int lbp = 0;
	char c = '\0';

	// Null denotation
	virtual ParseNode nud(Parser& parser) {
		throw std::runtime_error(
			std::format("\"{}\": nud() not implemented", c)
		);
	}

	// Left denotation
	virtual ParseNode led(Parser& parser, ParseNode&& left) {
		throw std::runtime_error(
			std::format("\"{}\": led() not implemented", c)
		);
	}

	virtual float eval(const std::vector<ParseNode>& children) const {
		throw std::runtime_error(
			std::format("\"{}\": eval() not implemented", c)
		);
	}

	// For debug
	virtual std::string toString() { return "Token"; };
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
	VariableRegistry* _vars;

public:
	Tokenizer(VariableRegistry& vars, const std::string& expression)
	: _vars { &vars}, _expression { expression } {}

	bool atEnd() { return _atEnd; }

	ParseIntResult parseInt();
	float parseFloat();
	std::string parseAlphaString();
	std::unique_ptr<Token> makeFunction(std::string&&);
	std::unique_ptr<Token> next();
};

class Parser {
private:
	Tokenizer _tokenizer;
	std::unique_ptr<Token> _cur;
	std::unique_ptr<Token> _prev;

	void nextToken();
public:
	Parser(VariableRegistry& vars, const std::string& expression)
	: _tokenizer { vars, expression } {}

	ParseNode expression(unsigned int rbp = 0);
	void expect(char c);
	ParseNode parse();
};

// LBP is zero.
struct EndToken : public Token {
	EndToken() { c = 'E'; }
	std::string toString() override { return "END"; }
};

struct StartParenToken : public Token {
	StartParenToken() { c = LPAREN; }

	ParseNode nud(Parser& parser) override {
		ParseNode node = parser.expression();
		parser.expect(RPAREN);
		return node;
	}
};

struct EndParenToken : public Token {
	EndParenToken() { c = RPAREN; }
};

struct LiteralToken : public Token {
	float value;

	LiteralToken(float value) : value { value } { lbp = 0; };

	ParseNode nud(Parser& parser) override {
		return { std::make_unique<LiteralToken>(*this) };
	}

	float eval(const std::vector<ParseNode>& children) const override {
		return value;
	}

	std::string toString() override { return std::format("LITERAL:{}", value); }
};

struct VarToken : public Token {
private:
	VariableRegistry* _vars;
public:
	VarToken(VariableRegistry& vars, char _c) : _vars { &vars } { c = _c; }

	ParseNode nud(Parser& parser) override {
		return { std::make_unique<VarToken>(*this) };
	}

	float eval(const std::vector<ParseNode>& children) const override {
		return _vars->get(c);
	}

	std::string toString() override { return std::format("VAR:{}", c); }
};

struct FuncSin {
	static inline float op(float x) { return std::sin(x); }
};

struct FuncCos {
	static inline float op(float x) { return std::cos(x); }
};

struct FuncExp {
	static inline float op(float x) { return std::exp(x); }
};

template<typename T>
struct FuncToken : public Token {
	std::string s;

	FuncToken(std::string&& _s) : s {std::move(_s) } { lbp = 110; }

	ParseNode nud(Parser& parser) override {
		return {
			std::make_unique<FuncToken>(*this),
			parser.expression(lbp-1)
		};
	}

	float eval(const std::vector<ParseNode>& children) const override {
		return T::op(children.at(0).eval());
	}

	std::string toString() override { return std::format("FUNC:{}", s); }
};

struct OpAdd {
	static constexpr char c = ADD_CHAR;
	static constexpr unsigned int lbp = 20;
	static constexpr unsigned int lbpSub = 0;
	static constexpr bool allowUnary = true;
	static constexpr unsigned int unaryLbp = 100;
	static constexpr char name[] = "ADD";

	static inline float opUnary(float x) { return x; }
	static inline float op(float a, float b) { return a + b; }
};

struct OpSub {
	static constexpr char c = SUB_CHAR;
	static constexpr unsigned int lbp = 20;
	static constexpr unsigned int lbpSub = 0;
	static constexpr bool allowUnary = true;
	static constexpr unsigned int unaryLbp = 100;
	static constexpr char name[] = "SUB";

	static inline float opUnary(float x) { return -x; }
	static inline float op(float a, float b) { return a - b; }
};

struct OpMul {
	static constexpr char c = MUL_CHAR;
	static constexpr unsigned int lbp = 30;
	static constexpr unsigned int lbpSub = 0;
	static constexpr bool allowUnary = false;
	static constexpr char name[] = "MUL";

	static inline float opUnary(float x) { return 0.0f; }
	static inline float op(float a, float b) { return a * b; }
};

struct OpDiv {
	static constexpr char c = DIV_CHAR;
	static constexpr unsigned int lbp = 30;
	static constexpr unsigned int lbpSub = 0;
	static constexpr bool allowUnary = false;
	static constexpr char name[] = "DIV";

	static inline float opUnary(float x) { return 0.0f; }
	static inline float op(float a, float b) { return a / b; }
};

struct OpExp {
	static constexpr char c = EXP_CHAR;
	static constexpr unsigned int lbp = 40;
	static constexpr unsigned int lbpSub = 1; // makes right-associative
	static constexpr bool allowUnary = false;
	static constexpr char name[] = "EXP";

	static inline float opUnary(float x) { return 0.0f; }
	static inline float op(float a, float b) { return std::pow(a, b); }
};

struct OpGt {
	static constexpr char c = GT_CHAR;
	static constexpr unsigned int lbp = 10;
	static constexpr unsigned int lbpSub = 0;
	static constexpr bool allowUnary = false;
	static constexpr char name[] = "GT";

	static inline float opUnary(float x) { return 0.0f; }
	static inline float op(float a, float b) { return static_cast<float>(a > b); }
};

struct OpLt {
	static constexpr char c = LT_CHAR;
	static constexpr unsigned int lbp = 10;
	static constexpr unsigned int lbpSub = 0;
	static constexpr bool allowUnary = false;
	static constexpr char name[] = "LT";

	static inline float opUnary(float x) { return 0.0f; }
	static inline float op(float a, float b) { return static_cast<float>(a < b); }
};

struct OpEq {
	static constexpr char c = EQ_CHAR;
	static constexpr unsigned int lbp = 10;
	static constexpr unsigned int lbpSub = 0;
	static constexpr bool allowUnary = false;
	static constexpr char name[] = "EQ";

	static inline float opUnary(float x) { return 0.0f; }
	static inline float op(float a, float b) { return static_cast<float>(a == b); }
};

template<typename T>
struct BinaryOpToken : public Token {
	BinaryOpToken() {
		lbp = T::lbp;
		c = T::c;
	}

	ParseNode nud(Parser& parser) override {
		if constexpr (T::allowUnary) {
			return {
				std::make_unique<BinaryOpToken<T>>(*this),
				parser.expression(T::unaryLbp)
			};
		} else {
			std::string msg = std::format(
				"Binary operator \"{}\" cannot be used as unary",
				T::c
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
		}

		if (children.size() == 1) {
			return T::opUnary(children.at(0).eval());
		} else {
			return T::op(children.at(0).eval(), children.at(1).eval());
		}
	}

	std::string toString() override { return T::name; }
};

/*
struct AddToken : public Token {
	AddToken() { lbp = 10; }

	ParseNode nud(Parser& parser) override {
		return {
			std::make_unique<AddToken>(*this),
			parser.expression(100)
		};
	}

	ParseNode led(Parser& parser, ParseNode&& left) override {
		return {
			std::make_unique<AddToken>(*this),
			std::move(left),
			parser.expression(lbp)
		};
	}

	float eval(const std::vector<ParseNode>& children) const override {
		if (children.size() == 1) {
			return children.at(0).eval();
		} else {
			return children.at(0).eval() + children.at(1).eval();
		}
	}

	std::string toString() override { return "ADD"; }
};
*/

/*
struct SubToken : public Token {
	SubToken() { lbp = 10; }

	float nud(Parser& parser) override {
		return -parser.expression(100);
	}

	float led(Parser& parser, float left) override {
		return left - parser.expression(lbp);
	}

	std::string toString() override { return "SUB"; }
};

struct MulToken : public Token {
	MulToken() { lbp = 20; }

	float led(Parser& parser, float left) override {
		return left * parser.expression(lbp);
	}

	std::string toString() override { return "MUL"; }
};

struct DivToken : public Token {
	DivToken() { lbp = 20; }

	float led(Parser& parser, float left) override {
		return left / parser.expression(lbp);
	}

	std::string toString() override { return "DIV"; }
};
*/

void tokenizerTest(const std::string& expression);
void parserTest(const std::string& expression);
}
