#pragma once

#include "SpeedFlipTrainer.h" // For CustomColor, Vector2 (via SpeedFlipTrainer.h -> wrapperstructs.h), and CanvasWrapper (via SpeedFlipTrainer.h -> bakkesmodplugin.h)
#include <list>               // For std::list

// Note: The original 'Color' struct here caused redefinition errors.
// We will use 'CustomColor' from SpeedFlipTrainer.h or pass LinearColor directly.

struct LineStyle {
    CustomColor color;
    int width;

    LineStyle(CustomColor c = CustomColor{ 255, 255, 255, 1.0f }, int w = 2) : color(c), width(w) {}
};

struct MeterRange {
    CustomColor color;
    int low, high; // Representing units or ticks

    MeterRange(CustomColor c = CustomColor{ 0, 0, 0, 0.0f }, int l = 0, int h = 0) : color(c), low(l), high(h) {}
};

struct MeterMarking {
    LineStyle lineStyle;
    int value; // Representing a specific unit or tick to mark

    MeterMarking(LineStyle ls = LineStyle{}, int val = 0) : lineStyle(ls), value(val) {}
    MeterMarking(CustomColor color, int width, int val) : lineStyle(LineStyle{ color, width }), value(val) {}
};


// Renders a generic meter (horizontal or vertical)
// Returns the bottom-right Vector2 of the drawn meter box (can be used for positioning related elements)
Vector2 RenderMeter(
    CanvasWrapper canvas,
    Vector2 startPos,         // Top-left position of the meter
    Vector2 reqBoxSize,       // Requested size of the meter box
    CustomColor baseColor,    // Background color of the meter
    LineStyle borderStyle,    // Style for the meter's border
    int totalUnits,           // The total number of units the meter represents (e.g., 0 to 100 units)
    const std::list<MeterRange>& ranges,   // List of colored ranges to draw within the meter
    const std::list<MeterMarking>& markings, // List of lines/markings to draw on the meter
    bool vertical,            // True if the meter is vertical, false for horizontal
    float currentValue = -1.0f // Optional: a specific value to highlight with a special marker (e.g. current player value)
    // If >= 0, it will be drawn. Interpretation depends on implementation.
);