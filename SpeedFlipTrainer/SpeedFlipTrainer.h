#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "bakkesmod/wrappers/wrapperstructs.h" // Pour Vector, Rotator, CameraWrapper etc.
#include "ImGuiFileDialog.h"
#include "BotAttempt.h"
#include "Attempt.h"

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

#include <memory>       // Pour std::shared_ptr
#include <string>       // Pour std::string
#include <vector>       // Pour std::vector (si nécessaire pour d'autres parties)
#include <list>         // Pour std::list (utilisé dans RenderMeter.h, peut-être ici aussi)
#include <filesystem>   // Pour std::filesystem

// --- Structures nécessaires AVANT leur utilisation ---

// Définition pour Vector4, nécessaire pour Matrix
// Assurez-vous que 'Vector' est défini (via wrapperstructs.h)
struct Vector4 {
    float X, Y, Z, W;

    Vector4() : X(0), Y(0), Z(0), W(1.0f) {}
    Vector4(float x, float y, float z, float w) : X(x), Y(y), Z(z), W(w) {}
    Vector4(const Vector& v) : X(v.X), Y(v.Y), Z(v.Z), W(1.0f) {}
};

struct Matrix {
    float M[4][4];

    // Opérateur permettant de multiplier une matrice par un Vector4
    Vector4 operator*(const Vector4& vec) const {
        Vector4 result;
        result.X = M[0][0] * vec.X + M[0][1] * vec.Y + M[0][2] * vec.Z + M[0][3] * vec.W;
        result.Y = M[1][0] * vec.X + M[1][1] * vec.Y + M[1][2] * vec.Z + M[1][3] * vec.W;
        result.Z = M[2][0] * vec.X + M[2][1] * vec.Y + M[2][2] * vec.Z + M[2][3] * vec.W;
        result.W = M[3][0] * vec.X + M[3][1] * vec.Y + M[3][2] * vec.Z + M[3][3] * vec.W;
        return result;
    }
};

// Structure pour les couleurs utilisée par DrawArrow et RenderCarAxes
struct Color {
    unsigned char red, green, blue;
    float opacity;
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
    // Structure pour stocker les vecteurs d'orientation (déjà dans votre code original)
    struct Orientation {
        Vector forward;
        Vector right;
        Vector up;
    };

    // CVars (initialisées directement comme dans votre code original)
    std::shared_ptr<bool> showCarAxes = std::make_shared<bool>(true);
    std::shared_ptr<float> axisLength = std::make_shared<float>(150.0f);
    std::shared_ptr<bool> enabled = std::make_shared<bool>(true);
    std::shared_ptr<bool> showAngleMeter = std::make_shared<bool>(true);
    std::shared_ptr<bool> showPositionMeter = std::make_shared<bool>(true);
    std::shared_ptr<bool> showFlipMeter = std::make_shared<bool>(true);
    std::shared_ptr<bool> showJumpMeter = std::make_shared<bool>(true);
    std::shared_ptr<bool> changeSpeed = std::make_shared<bool>(false);
    std::shared_ptr<float> speed = std::make_shared<float>(1.0f);
    std::shared_ptr<bool> rememberSpeed = std::make_shared<bool>(true);
    std::shared_ptr<int> numHitsChangedSpeed = std::make_shared<int>(3);
    std::shared_ptr<float> speedIncrement = std::make_shared<float>(0.05f); // Noté 0.05 dans votre liste d'erreur, original 0.1
    std::shared_ptr<int> optimalLeftAngle = std::make_shared<int>(-30);
    std::shared_ptr<int> optimalRightAngle = std::make_shared<int>(30);
    std::shared_ptr<int> flipCancelThreshold = std::make_shared<int>(13);
    std::shared_ptr<int> jumpLow = std::make_shared<int>(40);
    std::shared_ptr<int> jumpHigh = std::make_shared<int>(90);
    std::shared_ptr<bool> saveToFile = std::make_shared<bool>(false);

    // Constructeur (peut être vide si l'initialisation ci-dessus suffit)
    SpeedFlipTrainer() {
        // dataDir, attemptFileDialog, botFileDialog peuvent nécessiter une initialisation ici ou dans onLoad()
        // Par exemple, pour les FileDialogs si elles ont des constructeurs spécifiques
        // menuTitle_ est déjà initialisé ci-dessous.
    }

    // Fonctions publiques
    void RenderCarAxes(CanvasWrapper canvas);
    Orientation RotatorToMatrix(const Rotator& rotation);
    Vector2 WorldToScreen(CanvasWrapper canvas, Vector location);
    bool IsPointOnScreen(const Vector2& point, float screenWidth, float screenHeight);
    void DrawArrow(CanvasWrapper& canvas, Vector2 start, Vector2 end, const Color& color);
    Matrix GetViewProjectionMatrix(CameraWrapper camera); // Déclaration unique ici

    // Méthodes héritées et virtuelles
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
    int ticksBeforeTimeExpired = 0;

    Attempt attempt;
    Attempt replayAttempt;
    BotAttempt bot;

    int consecutiveHits = 0;
    int consecutiveMiss = 0;

    void Hook();
    bool IsMustysPack(TrainingEditorWrapper tw);
    void Measure(CarWrapper car, std::shared_ptr<GameWrapper> gameWrapper);
    void PlayBot(std::shared_ptr<GameWrapper> gameWrapper, ControllerInput* ci);
    void PlayAttempt(Attempt* a, std::shared_ptr<GameWrapper> gameWrapper, ControllerInput* ci);

    void RenderMeters(CanvasWrapper canvas);
    void RenderAngleMeter(CanvasWrapper canvas, float screenWidth, float screenHeight);
    void RenderFlipCancelMeter(CanvasWrapper canvas, float screenWidth, float screenHeight);
    void RenderFirstJumpMeter(CanvasWrapper canvas, float screenWidth, float screenHeight);
    void RenderPositionMeter(CanvasWrapper canvas, float screenWidth, float screenHeight);

    // Membres pour PluginWindow
    bool isWindowOpen_ = false;
    bool isMinimized_ = false;
    std::string menuTitle_ = "Speedflip Trainer";

    std::filesystem::path dataDir;
    ImGui::FileDialog attemptFileDialog;
    ImGui::FileDialog botFileDialog;
};