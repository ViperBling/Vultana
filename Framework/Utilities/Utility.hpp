#pragma once

#include <iostream>
#include <string>
#include <functional>

#define NOCOPY(ClassName) \
    ClassName(ClassName&) = delete; \
    ClassName& operator=(ClassName&) = delete;\
    ClassName(const ClassName&) = delete;\
    ClassName& operator=(const ClassName&) = delete;


inline void GDebugInfoCallbackFunc(const std::string& message, const std::string& type)
{
    std::cout << "[INFO " << type <<"] : " << message << std::endl;
}

static std::function<void(const std::string&, const std::string&)> GDebugInfoCallback = GDebugInfoCallbackFunc;