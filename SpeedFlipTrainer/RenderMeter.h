#pragma once

// Make sure SpeedFlipTrainer.h (which defines CustomColor, Vector2) is included before this file,
// or provide forward declarations if only pointers/references are used.
// For simplicity, assuming SpeedFlipTrainer.h is included first in .cpp files that use RenderMeter.
// #include "SpeedFlipTrainer.h" // Or forward declare: struct CustomColor; struct Vector2; class CanvasWrapper;

#include <list> // For std::list

// Note: The original 'Color' struct here caused redefinition errors.
// We will use 'CustomColor' from SpeedFlipTrainer.h or pass LinearColor directly.
// For this correction, we assume CustomColor from SpeedFlipTrainer.h is intended.

struct LineStyle { // Renamed from Line to avoid conflict if there's a Line class elsewhere
    CustomColor color;
    int width;

    LineStyle(CustomColor c = CustomColor{ 255,255,255,1.0f }, int w = 2) : color(c), width(w) {}
};

struct MeterRange {
    CustomColor color;
    int low, high;

    MeterRange(CustomColor c = CustomColor{ 0,0,0,0.0f }, int l = 0, int h = 0) : color(c), low(l), high(h) {}
};

struct MeterMarking {
    LineStyle lineStyle; // Use LineStyle which includes color and width
    int value;

    MeterMarking(LineStyle ls = LineStyle{}, int val = 0) : lineStyle(ls), value(val) {}
    // Constructor for convenience if only color, width, and value are needed
    MeterMarking(CustomColor color, int width, int val) : lineStyle(LineStyle{ color, width }), value(val) {}
};


// The function signature needs to match how it's called and defined.
// Using CustomColor for base and LineStyle for border.
Vector2 RenderMeter(
    CanvasWrapper canvas,
    Vector2 startPos,
    Vector2 reqBoxSize,
    CustomColor baseColor,      // Base color of the meter background
    LineStyle borderStyle,      // Style for the meter border
    int totalUnits,
    const std::list<MeterRange>& ranges,
    const std::list<MeterMarking>& markings,
    bool vertical,
    float currentValue = -1.0f // Optional: current value to draw a marker for, if not in markings
);