#include <iostream>

#if defined(_WIN32)
#define RTYPE_LUA_API __declspec(dllexport)
#else
#define RTYPE_LUA_API
#endif

extern "C" RTYPE_LUA_API void lua_hello() { std::cout << "hello world!" << std::endl; }
