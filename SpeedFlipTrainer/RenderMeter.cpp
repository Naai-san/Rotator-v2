#include "pch.h"
#include "RenderMeter.h" // Includes SpeedFlipTrainer.h for CustomColor, Vector2, CanvasWrapper

// Implementation of RenderMeter function
Vector2 RenderMeter(CanvasWrapper canvas, Vector2 startPos, Vector2 reqBoxSize, CustomColor baseColor,
    LineStyle borderStyle, int totalUnits, const std::list<MeterRange>& ranges,
    const std::list<MeterMarking>& markings, bool vertical, float currentValue) {

    // Calculate actual box size based on totalUnits and requested size (aspect ratio might change)
    Vector2 actualBoxSize = reqBoxSize;
    float unitPixelSize;

    if (vertical) {
        unitPixelSize = (totalUnits > 0) ? static_cast<float>(actualBoxSize.Y) / static_cast<float>(totalUnits) : 0;
        // actualBoxSize.Y = unitPixelSize * totalUnits; // Optional: adjust box Y to perfectly fit units
    }
    else {
        unitPixelSize = (totalUnits > 0) ? static_cast<float>(actualBoxSize.X) / static_cast<float>(totalUnits) : 0;
        // actualBoxSize.X = unitPixelSize * totalUnits; // Optional: adjust box X
    }
    if (unitPixelSize <= 0.001f && totalUnits > 0) return startPos + actualBoxSize; // Avoid division by zero or tiny units


    // Draw background
    canvas.SetColor(baseColor.r, baseColor.g, baseColor.b, baseColor.GetAlphaChar());
    canvas.SetPosition(startPos);
    canvas.FillBox(actualBoxSize);

    // Draw ranges
    for (const auto& range : ranges) {
        canvas.SetColor(range.color.r, range.color.g, range.color.b, range.color.GetAlphaChar());
        Vector2 rangePos = startPos;
        Vector2 rangeSize = actualBoxSize;
        int rLow = std::max(0, std::min(totalUnits, range.low));
        int rHigh = std::max(0, std::min(totalUnits, range.high));
        if (rLow >= rHigh) continue;


        if (vertical) {
            // In vertical, Y increases downwards. Low tick value is at bottom, high tick value at top.
            // So range.low (e.g. 10 ticks) should be below range.high (e.g. 20 ticks) on screen.
            // Meter is drawn from top (startPos.Y).
            // (totalUnits - rHigh) gives offset from top for the *top* of the range.
            rangePos.Y += static_cast<int>((totalUnits - rHigh) * unitPixelSize);
            rangeSize.Y = static_cast<int>((rHigh - rLow) * unitPixelSize);
        }
        else {
            rangePos.X += static_cast<int>(rLow * unitPixelSize);
            rangeSize.X = static_cast<int>((rHigh - rLow) * unitPixelSize);
        }
        canvas.SetPosition(rangePos);
        canvas.FillBox(rangeSize);
    }

    // Draw markings
    for (const auto& mark : markings) {
        canvas.SetColor(mark.lineStyle.color.r, mark.lineStyle.color.g, mark.lineStyle.color.b, mark.lineStyle.color.GetAlphaChar());
        int mVal = std::max(0, std::min(totalUnits, mark.value));
        Vector2 markStart, markEnd;

        if (vertical) {
            // Similar to ranges, (totalUnits - mVal) gives offset from top.
            float yPos = startPos.Y + static_cast<int>((totalUnits - mVal) * unitPixelSize);
            markStart = { startPos.X, static_cast<int>(yPos) };
            markEnd = { startPos.X + actualBoxSize.X, static_cast<int>(yPos) };
        }
        else {
            float xPos = startPos.X + static_cast<int>(mVal * unitPixelSize);
            markStart = { static_cast<int>(xPos), startPos.Y };
            markEnd = { static_cast<int>(xPos), startPos.Y + actualBoxSize.Y };
        }
        canvas.DrawLine(markStart, markEnd, mark.lineStyle.width);
    }

    // Draw current value if provided (currentValue is in terms of units from 0 to totalUnits)
    if (currentValue >= 0.0f) {
        int currentValUnit = static_cast<int>(currentValue);
        currentValUnit = std::max(0, std::min(totalUnits, currentValUnit));
        // Example: Draw a thicker, darker line for currentValue
        CustomColor currentValueColor = CustomColor(10, 10, 10, 0.8f); // Darker marker
        int currentValueMarkWidth = borderStyle.width > 0 ? borderStyle.width : 2; // Use border width or default
        canvas.SetColor(currentValueColor.r, currentValueColor.g, currentValueColor.b, currentValueColor.GetAlphaChar());

        Vector2 cvMarkStart, cvMarkEnd;
        if (vertical) {
            float yPos = startPos.Y + static_cast<int>((totalUnits - currentValUnit) * unitPixelSize);
            cvMarkStart = { startPos.X, static_cast<int>(yPos) };
            cvMarkEnd = { startPos.X + actualBoxSize.X, static_cast<int>(yPos) };
        }
        else {
            float xPos = startPos.X + static_cast<int>(currentValUnit * unitPixelSize);
            cvMarkStart = { static_cast<int>(xPos), startPos.Y };
            cvMarkEnd = { static_cast<int>(xPos), startPos.Y + actualBoxSize.Y };
        }
        canvas.DrawLine(cvMarkStart, cvMarkEnd, currentValueMarkWidth + 1); // Slightly thicker
    }


    // Draw border
    if (borderStyle.width > 0) {
        canvas.SetColor(borderStyle.color.r, borderStyle.color.g, borderStyle.color.b, borderStyle.color.GetAlphaChar());
        canvas.SetPosition(startPos);
        // Assuming DrawBox(Vector2 size, int thickness) exists or DrawBox(Vector2 size) draws a 1px border
        // If DrawBox only takes size, borderStyle.width might be conceptually used or needs manual line drawing for thickness
        // For now, trying to use it as a thickness parameter.
        // If your BakkesMod version DrawBox(Vector2) is a 1px outline, and you want thicker,
        // you'd need to draw 4 lines or filled rects.
        // Let's assume an overload canvas.DrawBox(size, thickness) might exist or be intended.
        // If not, change to canvas.DrawBox(actualBoxSize); and the thickness is 1px.
        canvas.DrawBox(actualBoxSize); // Fallback to 1px border if thickness param is not supported
        // Or if canvas.DrawBox(actualBoxSize, borderStyle.width) is the correct API:
        // canvas.DrawBox(actualBoxSize, borderStyle.width);
    }

    return startPos + actualBoxSize; // Return bottom-right corner or similar
}