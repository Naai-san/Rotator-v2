#pragma once

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include "bakkesmod/plugin/bakkesmodplugin.h"

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <filesystem> // Added for std::filesystem

#include "imgui/imgui.h"

#include "fmt/core.h"
#include "fmt/ranges.h"

extern std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

template<typename S, typename... Args>
void LOG(const S& format_str, Args&&... args)
{
	if (_globalCvarManager) { // Added null check for safety
		_globalCvarManager->log(fmt::format(format_str, args...));
	}
}

// Forward declarations if needed by other headers included transitively by pch.h
// struct Vector; // Example if Vector is used by a header included here before its full definition
// struct Rotator;
// struct Matrix;
// struct Vector4;
// struct Color;
// class CanvasWrapper;
// class CameraWrapper;

//PCH_H #endif 