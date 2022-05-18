#pragma once
#include <glfw3_noinclude.h>


class IContextWrapper {
public:
	// provides abstract interface for RAII-enabled context creation
	// also implicitly disables copy and move operations
	virtual ~IContextWrapper() = 0;
};

inline IContextWrapper::~IContextWrapper() = default;
