#pragma once
#include <exception>
#include <string>

struct Exception :std::exception {
	std::string msg;
	const char *str = 0;
	Exception() noexcept {}
	Exception(const char* s) noexcept;
	Exception(std::string s) noexcept;
	const char* what()const noexcept override { return str ? str : msg.c_str(); }
};

struct SyntaxError : Exception {
	std::string stack_info;
	SyntaxError()noexcept : Exception("Syntax Error") {}
	SyntaxError(std::string s) noexcept : Exception(std::move(s)) {}
	SyntaxError(std::string s, std::string stackinfo) noexcept : Exception(std::move(s)), stack_info(std::move(stackinfo)) {}
};

struct RRConflict: SyntaxError {
	RRConflict() noexcept:SyntaxError("Reduce-reduce conflict") {}
	RRConflict(std::string s) noexcept : SyntaxError(std::move(s)) {}
	RRConflict(std::string s, std::string stackinfo) noexcept : SyntaxError(std::move(s),move(stackinfo)) {}
};
