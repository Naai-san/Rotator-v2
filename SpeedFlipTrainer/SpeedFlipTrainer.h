#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "ImGuiFileDialog.h"
#include "BotAttempt.h"

#include "version.h"
#include "Attempt.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

using namespace std;

enum class SpeedFlipTrainerMode
{
	Replay,
	Bot,
	Manual
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

class SpeedFlipTrainer {
public:
	Matrix GetViewProjectionMatrix(CameraWrapper camera);
};

class SpeedFlipTrainer: public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginSettingsWindow, public BakkesMod::Plugin::PluginWindow
{
public: 
	// DEBUT


	// SpeedFlipTrainer.h - Ajoutez ces définitions dans la partie publique ou privée de la classe
	// Structure pour stocker les vecteurs d'orientation
	struct Orientation {
		Vector forward;
		Vector right;
		Vector up;
	};

	// Variables pour les axes de la voiture
	shared_ptr<bool> showCarAxes = make_shared<bool>(true);

	// Nouvelle fonction pour le rendu des axes
	void RenderCarAxes(CanvasWrapper canvas);

	// Fonction utilitaire pour convertir Rotator en vecteurs d'orientation
	Orientation RotatorToMatrix(const Rotator& rotation);

	// Fonction utilitaire pour convertir un point 3D en 2D pour l'affichage
	Vector2 WorldToScreen(CanvasWrapper canvas, Vector location);

	// Fonction utilitaire pour vérifier si un point est visible à l'écran
	bool IsPointOnScreen(const Vector2& point, float screenWidth, float screenHeight);

	// Fonction pour dessiner une ligne avec une flèche à la fin
	void DrawArrow(CanvasWrapper& canvas, Vector2 start, Vector2 end, const struct Color& color);

	// Fonction pour obtenir la matrice de projection de la caméra
	Matrix GetViewProjectionMatrix(CameraWrapper camera);

	// Dans la déclaration de la classe SpeedFlipTrainer
	std::shared_ptr<float> axisLength = std::make_shared<float>(150.0f);

	// FIN

private:

	SpeedFlipTrainerMode mode = SpeedFlipTrainerMode::Manual;

	// Whether plugin is enabled
	shared_ptr<bool> enabled = make_shared<bool>(true);

	// Whether to show various meters
	shared_ptr<bool> showAngleMeter = make_shared<bool>(true);
	shared_ptr<bool> showPositionMeter = make_shared<bool>(true);
	shared_ptr<bool> showFlipMeter = make_shared<bool>(true);
	shared_ptr<bool> showJumpMeter = make_shared<bool>(true);

	// Whether the speed should be changed on consecutive hits and misses
	shared_ptr<bool> changeSpeed = make_shared<bool>(false);

	// What the last speed was
	shared_ptr<float> speed = make_shared<float>(1.0);

	// Whether we should remember the game speed between plugin loads
	shared_ptr<bool> rememberSpeed = make_shared<bool>(true);

	// Number of hits/misses before the game speed is changed
	shared_ptr<int> numHitsChangedSpeed = make_shared<int>(3);

	// Amount to change the game speed by
	shared_ptr<float> speedIncrement = make_shared<float>(0.1);
	
	// Optimal left angle for dodge
	shared_ptr<int> optimalLeftAngle = make_shared<int>(-30);

	// Optimal right angle for dodge
	shared_ptr<int> optimalRightAngle = make_shared<int>(30);

	// Physics ticks the flip canceled should be performed under
	shared_ptr<int> flipCancelThreshold = make_shared<int>(13);

	// Physics ticks range during which first jump should be performed
	shared_ptr<int> jumpLow = make_shared<int>(40);
	shared_ptr<int> jumpHigh = make_shared<int>(90);

	// Save attempts to file?
	shared_ptr<bool> saveToFile = make_shared<bool>(false);

	// Whether plugin is loaded
	bool loaded = false;

	// Assists in determining when time started counting down
	float initialTime = 0;

	int startingPhysicsFrame = -1;
	int ticksBeforeTimeExpired = (int)(initialTime * 120);

	Attempt attempt;
	Attempt replayAttempt;

	BotAttempt bot;

	// Consecutive hits and misses
	int consecutiveHits = 0;
	int consecutiveMiss = 0;

	//Boilerplate
	virtual void onLoad();
	virtual void onUnload();

	// Inherited via PluginSettingsWindow
	void RenderSettings() override;
	std::string GetPluginName() override;

	// Hooks the necessary game functions to operate the plugin
	void Hook();

	// Determines whether Musty's training pack is loaded
	bool IsMustysPack(TrainingEditorWrapper tw);

	// Measures the speedflip being performed
	void Measure(CarWrapper car, shared_ptr<GameWrapper> gameWrapper);

	// Programmed bot to perform the speed flip
	void PlayBot(shared_ptr<GameWrapper> gameWrapper, ControllerInput* ci);

	// Replays saved speedflip attempt
	void PlayAttempt(Attempt* a, shared_ptr<GameWrapper> gameWrapper, ControllerInput* ci);

	// Render functions to render various meters and measured values on screen
	void RenderMeters(CanvasWrapper canvas);
	void RenderAngleMeter(CanvasWrapper canvas, float screenWidth, float screenHeight);
	void RenderFlipCancelMeter(CanvasWrapper canvas, float screenWidth, float screenHeight);
	void RenderFirstJumpMeter(CanvasWrapper canvas, float screenWidth, float screenHeight);
	void RenderPositionMeter(CanvasWrapper canvas, float screenWidth, float screenHeight);

	// Inherited via PluginWindow
	bool isWindowOpen_ = false;
	bool isMinimized_ = false;
	std::string menuTitle_ = "Speedflip Trainer";

	virtual void Render() override;
	virtual std::string GetMenuName() override;
	virtual std::string GetMenuTitle() override;
	virtual void SetImGuiContext(uintptr_t ctx) override;
	virtual bool ShouldBlockInput() override;
	virtual bool IsActiveOverlay() override;
	virtual void OnOpen() override;
	virtual void OnClose() override;

	filesystem::path dataDir;
	ImGui::FileDialog attemptFileDialog;
	ImGui::FileDialog botFileDialog;
};

