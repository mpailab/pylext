#pragma once
#include <exception>
#include <string>

struct Exception :std::exception {
	std::string msg;
	const char *str = nullptr;
	Exception() noexcept = default;
	explicit Exception(const char* s) noexcept;
	explicit Exception(std::string s) noexcept;
	[[nodiscard]] const char* what()const noexcept override { return str ? str : msg.c_str(); }
	void prepend_msg(const std::string &s) {
	    msg = s+what();
	    str = nullptr;
	}
};

struct GrammarError : Exception {
	GrammarError()noexcept : Exception("Grammar Error") {}
	explicit GrammarError(std::string s) noexcept : Exception(std::move(s)) {}
};

struct SyntaxError : Exception {
	std::string stack_info;
	SyntaxError()noexcept : Exception("Syntax Error") {}
	explicit SyntaxError(std::string s) noexcept : Exception(std::move(s)) {}
	SyntaxError(std::string s, std::string stackinfo) noexcept : Exception(std::move(s)), stack_info(std::move(stackinfo)) {}
};

struct RRConflict: SyntaxError {
	RRConflict() noexcept:SyntaxError("Reduce-reduce conflict") {}
	explicit RRConflict(std::string s) noexcept : SyntaxError(std::move(s)) {}
	RRConflict(std::string s, std::string stackinfo) noexcept : SyntaxError(std::move(s),move(stackinfo)) {}
};

struct AssertionFailed : std::exception {
	const char* str = nullptr;
	explicit AssertionFailed(const char* s)noexcept :str(s) {}
	[[nodiscard]] const char* what()const noexcept override {	return str;	}
};

#define Assert(cond) if(!(cond))throw AssertionFailed(#cond " assertion failed ")
