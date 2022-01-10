#pragma once
#include <GLFW/glfw3.h>


class IContextWrapper {
public:
	// provides abstract interface for RAII-enabled context creation
	// also implicitly disables copy and move operations
	virtual ~IContextWrapper() = 0;
};

inline IContextWrapper::~IContextWrapper() = default;
