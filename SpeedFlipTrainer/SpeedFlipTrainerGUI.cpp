#include "pch.h"
#include "SpeedFlipTrainer.h"
// ImGuiFileDialog.h is already included via pch.h or SpeedFlipTrainer.h
// BotAttempt.h is already included via SpeedFlipTrainer.h

// Plugin Settings Window code here
std::string SpeedFlipTrainer::GetPluginName() {
	return "Speedflip Trainer"; // Corrected name
}

// Render the plugin settings here
void SpeedFlipTrainer::RenderSettings() {
	ImGui::TextUnformatted("Plugin to train speedflips in Musty's training pack: A503-264C-A7EB-D282");

	CVarWrapper enableCvar = cvarManager->getCvar("sf_enabled");
	if (!enableCvar) { ImGui::TextUnformatted("Error: sf_enabled CVar not found!"); return; }

	bool currentPluginEnabled = *enabled; // Use the bound variable
	if (ImGui::Checkbox("Enable plugin", &currentPluginEnabled)) {
		enableCvar.setValue(currentPluginEnabled);
		*enabled = currentPluginEnabled; // Update bound variable
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Enable/Disable Speedflip trainer plugin");

	ImGui::Separator();
	ImGui::TextUnformatted("Car Axes Settings");

	bool currentShowCarAxes = *showCarAxes;
	if (ImGui::Checkbox("Show Car Axes", &currentShowCarAxes)) {
		cvarManager->getCvar("sf_show_axes").setValue(currentShowCarAxes);
		*showCarAxes = currentShowCarAxes;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Display colored axes to help with orientation");
	}

	if (*showCarAxes) {
		// Use .get() to pass float* to ImGui::SliderFloat
		if (ImGui::SliderFloat("Axes Length", axisLength.get(), 50.0f, 300.0f, "%.0f")) {
			cvarManager->getCvar("sf_axis_length").setValue(*axisLength);
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Length of the orientation axes");
		}
	}

	ImGui::Separator();
	ImGui::TextUnformatted("Meter Display Settings");

	bool currentShowAngleMeter = *showAngleMeter;
	if (ImGui::Checkbox("Show Dodge Angle Meter", &currentShowAngleMeter)) {
		cvarManager->getCvar("sf_show_angle").setValue(currentShowAngleMeter);
		*showAngleMeter = currentShowAngleMeter;
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show meter for the dodge angle.");

	bool currentShowFlipMeter = *showFlipMeter;
	if (ImGui::Checkbox("Show Flip Cancel Meter", &currentShowFlipMeter)) {
		cvarManager->getCvar("sf_show_flip").setValue(currentShowFlipMeter);
		*showFlipMeter = currentShowFlipMeter;
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show meter for the time to flip cancel.");

	bool currentShowJumpMeter = *showJumpMeter;
	if (ImGui::Checkbox("Show First Jump Meter", &currentShowJumpMeter)) {
		cvarManager->getCvar("sf_show_jump").setValue(currentShowJumpMeter);
		*showJumpMeter = currentShowJumpMeter;
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show meter for time to first jump.");

	bool currentShowPosMeter = *showPositionMeter;
	if (ImGui::Checkbox("Show Horizontal Position Meter", &currentShowPosMeter)) {
		cvarManager->getCvar("sf_show_position").setValue(currentShowPosMeter);
		*showPositionMeter = currentShowPosMeter;
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show meter for the horizontal position.");


	ImGui::Separator();
	ImGui::TextUnformatted("Gameplay Settings");

	if (ImGui::SliderInt("Optimal Left Angle", optimalLeftAngle.get(), -70, -15)) {
		cvarManager->getCvar("sf_left_angle").setValue(*optimalLeftAngle);
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("The optimal angle (degrees) to dodge left.");

	if (ImGui::SliderInt("Optimal Right Angle", optimalRightAngle.get(), 15, 70)) {
		cvarManager->getCvar("sf_right_angle").setValue(*optimalRightAngle);
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("The optimal angle (degrees) to dodge right.");

	if (ImGui::SliderInt("Flip Cancel Threshold (ticks)", flipCancelThreshold.get(), 1, 15)) {
		cvarManager->getCvar("sf_cancel_threshold").setValue(*flipCancelThreshold);
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Max physics ticks after dodge to perform flip cancel.");

	// Commented out jumpLow/jumpHigh as they were in original GUI but cvars were commented in onLoad
	// CVarWrapper jumpLowCVar = cvarManager->getCvar("sf_jump_low");
	// CVarWrapper jumpHighCVar = cvarManager->getCvar("sf_jump_high");
	// if (jumpLowCVar && jumpHighCVar) {
	// 	int jumpPos[2] = {*jumpLow, *jumpHigh};
	// 	if (ImGui::SliderInt2("First Jump Time (ticks)", jumpPos, 10, 120)) {
	// 		if (jumpPos[0] < jumpPos[1]) {
	// 			jumpLowCVar.setValue(jumpPos[0]); *jumpLow = jumpPos[0];
	// 			jumpHighCVar.setValue(jumpPos[1]); *jumpHigh = jumpPos[1];
	// 		} else {
	// 			jumpLowCVar.setValue(jumpPos[1]); *jumpLow = jumpPos[1];
	// 			jumpHighCVar.setValue(jumpPos[0]); *jumpHigh = jumpPos[0];
	// 		}
	// 	}
	// 	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Optimal tick range for the first jump.");
	// }


	ImGui::Separator();
	ImGui::TextUnformatted("Game Speed Adjustment");

	bool currentChangeSpeed = *changeSpeed;
	if (ImGui::Checkbox("Change Game Speed on Hit/Miss", &currentChangeSpeed)) {
		cvarManager->getCvar("sf_change_speed").setValue(currentChangeSpeed);
		*changeSpeed = currentChangeSpeed;
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Alter game speed on consecutive hits/misses.");

	bool currentRememberSpeed = *rememberSpeed;
	if (ImGui::Checkbox("Remember Last Game Speed", &currentRememberSpeed)) {
		cvarManager->getCvar("sf_remember_speed").setValue(currentRememberSpeed);
		*rememberSpeed = currentRememberSpeed;
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Restore last game speed on next training load.");

	if (ImGui::SliderInt("Num Hits/Misses for Speed Change", numHitsChangedSpeed.get(), 1, 30)) {
		cvarManager->getCvar("sf_num_hits").setValue(*numHitsChangedSpeed);
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Consecutive hits/misses before speed changes.");

	if (ImGui::SliderFloat("Game Speed Increment", speedIncrement.get(), 0.001f, 0.5f, "%.3f")) { // Max to 0.5, original was 0.999
		cvarManager->getCvar("sf_speed_increment").setValue(*speedIncrement);
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Amount to change game speed by.");


	ImGui::Separator();
	ImGui::TextUnformatted("Attempt Management");
	bool currentSaveToFile = *saveToFile;
	if (ImGui::Checkbox("Save Attempts to File", &currentSaveToFile)) {
		cvarManager->getCvar("sf_save_attempts").setValue(currentSaveToFile);
		*saveToFile = currentSaveToFile;
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Automatically save each attempt's inputs to a file.");

}


// Render the plugin GUI window
void SpeedFlipTrainer::Render()
{
	if (!ImGui::Begin(menuTitle_.c_str(), &isWindowOpen_, ImGuiWindowFlags_None))
	{
		ImGui::End();
		return;
	}

	if (ImGui::Button("Enable Manual Mode"))
	{
		mode = SpeedFlipTrainerMode::Manual;
		LOG("MODE = Manual");
	}
	ImGui::SameLine();
	if (ImGui::Button("Save Last Attempt"))
	{
		if (!attempt.inputs.empty()) {
			auto path = attempt.GetFilename(dataDir); // Ensure dataDir is initialized
			attempt.WriteInputsToFile(path);
			LOG("Saved attempt to: {}", path.string());
		}
		else {
			LOG("No inputs in last attempt to save.");
		}
	}

	if (ImGui::Button("Replay Last Attempt"))
	{
		if (!attempt.inputs.empty()) {
			mode = SpeedFlipTrainerMode::Replay;
			LOG("MODE = Replay (Last Attempt)");
			replayAttempt = attempt;
		}
		else {
			LOG("No inputs in last attempt to replay.");
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Load Replay Attempt from File"))
	{
		attemptFileDialog.Open(); // Use Open() instead of .open = true
	}
	// Use Display() and IsOpened() for ImGuiFileDialog v0.6+
	if (attemptFileDialog.IsOpened()) {
		attemptFileDialog.Display();
		if (attemptFileDialog.HasSelected()) {
			try {
				Attempt loadedAttempt; // Create new attempt object
				loadedAttempt.ReadInputsFromFile(attemptFileDialog.GetSelected().string()); // Use GetSelected().string()
				if (!loadedAttempt.inputs.empty()) {
					mode = SpeedFlipTrainerMode::Replay;
					LOG("MODE = Replay (File)");
					replayAttempt = loadedAttempt; // Assign to replayAttempt
					LOG("Loaded attempt from file: {}", attemptFileDialog.GetSelected().string());
				}
				else {
					LOG("Attempt file was empty or failed to load: {}", attemptFileDialog.GetSelected().string());
				}
			}
			catch (const std::exception& e) {
				LOG("Failed to read attempt from file: {}. Exception: {}", attemptFileDialog.GetSelected().string(), e.what());
			}
			catch (...) {
				LOG("Failed to read attempt from file: {} (Unknown exception)", attemptFileDialog.GetSelected().string());
			}
			attemptFileDialog.ClearSelected();
			attemptFileDialog.Close();
		}
	}


	if (ImGui::Button("Load Bot: Diagonal Flip (-26 deg)")) // More descriptive
	{
		bot.Become26Bot();
		mode = SpeedFlipTrainerMode::Bot;
		LOG("MODE = Bot (-26 Degree)");
	}
	ImGui::SameLine();
	if (ImGui::Button("Load Bot: Forward Diagonal Flip (-45 deg)")) // More descriptive
	{
		bot.Become45Bot();
		mode = SpeedFlipTrainerMode::Bot;
		LOG("MODE = Bot (-45 Degree)");
	}
	ImGui::SameLine();
	if (ImGui::Button("Load Bot from File"))
	{
		botFileDialog.Open();
	}
	if (botFileDialog.IsOpened()) {
		botFileDialog.Display();
		if (botFileDialog.HasSelected()) {
			try {
				bot.ReadInputsFromFile(botFileDialog.GetSelected().string()); // Directly use bot member
				if (!bot.inputs.empty()) {
					LOG("Loaded bot from file: {}", botFileDialog.GetSelected().string());
					mode = SpeedFlipTrainerMode::Bot;
					LOG("MODE = Bot (File)");
				}
				else {
					LOG("Bot file was empty or failed to load: {}", botFileDialog.GetSelected().string());
				}
			}
			catch (const std::exception& e) {
				LOG("Failed to read bot from file: {}. Exception: {}", botFileDialog.GetSelected().string(), e.what());
			}
			catch (...) {
				LOG("Failed to read bot from file: {} (Unknown exception)", botFileDialog.GetSelected().string());
			}
			botFileDialog.ClearSelected();
			botFileDialog.Close();
		}
	}

	ImGui::End();

	if (!isWindowOpen_)
	{
		cvarManager->executeCommand("togglemenu " + GetMenuName());
	}
}

std::string SpeedFlipTrainer::GetMenuName()
{
	return "speedfliptrainer"; // Conventionally lowercase and one word
}

std::string SpeedFlipTrainer::GetMenuTitle()
{
	return menuTitle_;
}

void SpeedFlipTrainer::SetImGuiContext(uintptr_t ctx)
{
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
	// Optional: Setup ImGuiFileDialog if it needs the context explicitly
	// ImGui::FileDialog::Instance().SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

bool SpeedFlipTrainer::ShouldBlockInput()
{
	return ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
}

bool SpeedFlipTrainer::IsActiveOverlay()
{
	return true; // Window is interactive
}

void SpeedFlipTrainer::OnOpen()
{
	isWindowOpen_ = true;
}

void SpeedFlipTrainer::OnClose()
{
	isWindowOpen_ = false;
}