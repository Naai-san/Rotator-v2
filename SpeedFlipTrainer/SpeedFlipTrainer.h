#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "bakkesmod/wrappers/wrapperstructs.h" // For Vector, Rotator, CameraWrapper, LinearColor etc.
#include "bakkesmod/wrappers/GuiManagerWrapper.h" // For ImGui context
#include "bakkesmod/wrappers/GameObject/CarWrapper.h"
#include "bakkesmod/wrappers/GameObject/BallWrapper.h"
#include "bakkesmod/wrappers/GameEvent/ServerWrapper.h"
#include "bakkesmod/wrappers/GameEvent/GameEventWrapper.h"
#include "bakkesmod/wrappers/training/TrainingEditorWrapper.h"


#include "ImGuiFileDialog.h"
#include "BotAttempt.h" // Assuming this includes its own necessary headers like <vector>
#include "Attempt.h"    // Assuming this includes its own necessary headers

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

#include <memory>       // For std::shared_ptr
#include <string>       // For std::string
#include <vector>       // For std::vector
#include <list>         // For std::list
#include <filesystem>   // For std::filesystem
#include <cmath>        // For M_PI, tanf, cosf, sinf, sqrtf, atan2f, abs
#include <algorithm>    // For std::min/max if needed

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- Consolidated Structure Definitions ---

// Definition for Vector4, necessary for Matrix
// Ensure 'Vector' is defined (via wrapperstructs.h)
struct Vector4 {
    float X, Y, Z, W;

    Vector4() : X(0), Y(0), Z(0), W(1.0f) {}
    Vector4(float x, float y, float z, float w) : X(x), Y(y), Z(z), W(w) {}
    Vector4(const Vector& v, float w = 1.0f) : X(v.X), Y(v.Y), Z(v.Z), W(w) {} // Added w parameter
};

struct Matrix {
    float M[4][4];

    Matrix() { // Initialize to identity or zero matrix if preferred
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                M[i][j] = (i == j) ? 1.0f : 0.0f;
    }

    // Opérateur permettant de multiplier une matrice par un Vector4
    Vector4 operator*(const Vector4& vec) const {
        Vector4 result;
        result.X = M[0][0] * vec.X + M[0][1] * vec.Y + M[0][2] * vec.Z + M[0][3] * vec.W;
        result.Y = M[1][0] * vec.X + M[1][1] * vec.Y + M[1][2] * vec.Z + M[1][3] * vec.W;
        result.Z = M[2][0] * vec.X + M[2][1] * vec.Y + M[2][2] * vec.Z + M[2][3] * vec.W;
        result.W = M[3][0] * vec.X + M[3][1] * vec.Y + M[3][2] * vec.Z + M[3][3] * vec.W;
        return result;
    }
    // Matrix multiplication
    Matrix operator*(const Matrix& other) const {
        Matrix result;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                result.M[i][j] = 0;
                for (int k = 0; k < 4; ++k) {
                    result.M[i][j] += M[i][k] * other.M[k][j];
                }
            }
        }
        return result;
    }
};

// Structure for custom colors (if LinearColor is not sufficient for all cases)
// Note: CanvasWrapper typically uses LinearColor or char r,g,b,a (0-255)
struct CustomColor {
    unsigned char r, g, b;
    float a; // Opacity 0.0f to 1.0f

    CustomColor(unsigned char red = 0, unsigned char green = 0, unsigned char blue = 0, float alpha = 1.0f)
        : r(red), g(green), b(blue), a(alpha) {
    }

    // Conversion to LinearColor for BakkesMod drawing functions
    LinearColor ToLinearColor() const {
        return LinearColor{ static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f, static_cast<float>(b) / 255.0f, a };
    }
    // For CanvasWrapper::SetColor(char, char, char, char)
    unsigned char GetAlphaChar() const {
        return static_cast<unsigned char>(a * 255.0f);
    }
};


// --- Enumérations ---
enum class SpeedFlipTrainerMode
{
    Replay,
    Bot,
    Manual
};

// --- Définition principale de la classe ---
class SpeedFlipTrainer : public BakkesMod::Plugin::BakkesModPlugin,
    public BakkesMod::Plugin::PluginSettingsWindow,
    public BakkesMod::Plugin::PluginWindow
{
public:
    struct Orientation {
        Vector forward;
        Vector right;
        Vector up;
    };

    std::shared_ptr<bool> showCarAxes;
    std::shared_ptr<float> axisLength;
    std::shared_ptr<bool> enabled;
    std::shared_ptr<bool> showAngleMeter;
    std::shared_ptr<bool> showPositionMeter;
    std::shared_ptr<bool> showFlipMeter;
    std::shared_ptr<bool> showJumpMeter;
    std::shared_ptr<bool> changeSpeed;
    std::shared_ptr<float> speed;
    std::shared_ptr<bool> rememberSpeed;
    std::shared_ptr<int> numHitsChangedSpeed;
    std::shared_ptr<float> speedIncrement;
    std::shared_ptr<int> optimalLeftAngle;
    std::shared_ptr<int> optimalRightAngle;
    std::shared_ptr<int> flipCancelThreshold;
    std::shared_ptr<int> jumpLow;  // Make sure these are registered if used
    std::shared_ptr<int> jumpHigh; // Make sure these are registered if used
    std::shared_ptr<bool> saveToFile;

    SpeedFlipTrainer(); // Constructor declaration

    void RenderCarAxes(CanvasWrapper canvas);
    Orientation RotatorToOrientation(const Rotator& rotation); // Renamed for clarity
    Vector2 WorldToScreen(CanvasWrapper canvas, Vector location);
    bool IsPointOnScreen(const Vector2& point, float screenWidth, float screenHeight);
    void DrawArrow(CanvasWrapper& canvas, Vector2 start, Vector2 end, const CustomColor& color, int thickness = 2); // Using CustomColor
    Matrix GetViewProjectionMatrix(CameraWrapper camera);

    virtual void onLoad() override;
    virtual void onUnload() override;

    void RenderSettings() override;
    std::string GetPluginName() override;

    virtual void Render() override;
    virtual std::string GetMenuName() override;
    virtual std::string GetMenuTitle() override;
    virtual void SetImGuiContext(uintptr_t ctx) override;
    virtual bool ShouldBlockInput() override;
    virtual bool IsActiveOverlay() override;
    virtual void OnOpen() override;
    virtual void OnClose() override;

private:
    SpeedFlipTrainerMode mode = SpeedFlipTrainerMode::Manual;
    bool loaded = false;
    float initialTime = 0;
    int startingPhysicsFrame = -1;
    // int ticksBeforeTimeExpired = 0; // This seems unused, consider removing

    Attempt attempt;
    Attempt replayAttempt;
    BotAttempt bot;

    int consecutiveHits = 0;
    int consecutiveMiss = 0;

    void Hook();
    bool IsMustysPack(TrainingEditorWrapper tw);
    void Measure(CarWrapper car, PriWrapper pri); // Added PriWrapper for input
    void PlayBot(ControllerInput* ci); // Removed GameWrapper, use member gameWrapper
    void PlayAttempt(Attempt* a, ControllerInput* ci); // Removed GameWrapper

    void RenderMeters(CanvasWrapper canvas);
    void RenderAngleMeter(CanvasWrapper canvas, float screenWidth, float screenHeight);
    void RenderFlipCancelMeter(CanvasWrapper canvas, float screenWidth, float screenHeight);
    void RenderFirstJumpMeter(CanvasWrapper canvas, float screenWidth, float screenHeight);
    void RenderPositionMeter(CanvasWrapper canvas, float screenWidth, float screenHeight);

    bool isWindowOpen_ = false;
    // bool isMinimized_ = false; // PluginWindow doesn't typically manage this state itself
    std::string menuTitle_ = "Speedflip Trainer";

    std::filesystem::path dataDir;
    ImGui::FileDialog attemptFileDialog;
    ImGui::FileDialog botFileDialog;
};