#include <iostream>
#include "Exception.h"
using namespace std;
Exception::Exception(const char * s) noexcept:str(s) {
#ifdef _DEBUG
	cout << "Exception: " << s;
#endif
}

Exception::Exception(std::string s) noexcept : msg(std::move(s)) {
#ifdef _DEBUG
	cout << "Exception: " << msg;
#endif
}
