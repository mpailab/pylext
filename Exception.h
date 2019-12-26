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
	SyntaxError()noexcept : Exception("Syntax Error") {}
	SyntaxError(std::string s) noexcept : Exception(std::move(s)) {}
};

struct RRConflict :Exception {
	RRConflict() noexcept:Exception("Reduce-reduce conflict") {}
	RRConflict(std::string s) noexcept : Exception(std::move(s)) {}
};
