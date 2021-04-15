#include <iostream>
#include "Exception.h"
using namespace std;
Exception::Exception(const char * s) noexcept:str(s) {
#ifndef NDEBUG
	cout << "Exception: " << s;
#endif
}

Exception::Exception(std::string s) noexcept : msg(std::move(s)) {
#ifndef NDEBUG
	cout << "Exception: " << msg;
#endif
}
