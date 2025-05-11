#include "pch.h"
#include "SpeedFlipTrainer.h"
#include "RenderMeter.h" // Ensure this is after SpeedFlipTrainer.h if it uses its types
// #include <array> // Already in pch.h or SpeedFlipTrainer.h if needed by BotAttempt/Attempt
// #include "BotAttempt.h" // Already in SpeedFlipTrainer.h
// #include "bakkesmod/wrappers/wrapperstructs.h" // Already in SpeedFlipTrainer.h

#ifndef M_PI // Should be in SpeedFlipTrainer.h
#define M_PI 3.14159265358979323846
#endif
#define CONST_PI_F (static_cast<float>(M_PI)) // For clarity if used

BAKKESMOD_PLUGIN(SpeedFlipTrainer, "Speedflip Trainer", plugin_version, PLUGINTYPE_CUSTOM_TRAINING)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;


// Helper struct for clock time, can be local to where it's used or in the header if widely needed.
struct clock_time {
    int hour_hand;
    int min_hand;
};

// --- SpeedFlipTrainer Member Function Implementations ---

SpeedFlipTrainer::SpeedFlipTrainer() {
    // Initialize shared_ptr members for CVars
    showCarAxes = std::make_shared<bool>(true);
    axisLength = std::make_shared<float>(150.0f);
    enabled = std::make_shared<bool>(true);
    showAngleMeter = std::make_shared<bool>(true);
    showPositionMeter = std::make_shared<bool>(true);
    showFlipMeter = std::make_shared<bool>(true);
    showJumpMeter = std::make_shared<bool>(true);
    changeSpeed = std::make_shared<bool>(false);
    speed = std::make_shared<float>(1.0f);
    rememberSpeed = std::make_shared<bool>(true);
    numHitsChangedSpeed = std::make_shared<int>(3);
    speedIncrement = std::make_shared<float>(0.05f);
    optimalLeftAngle = std::make_shared<int>(-30);
    optimalRightAngle = std::make_shared<int>(30);
    flipCancelThreshold = std::make_shared<int>(13); // Original: 10, error list implies 13 for some tests
    jumpLow = std::make_shared<int>(40);
    jumpHigh = std::make_shared<int>(90);
    saveToFile = std::make_shared<bool>(false);
}


SpeedFlipTrainer::Orientation SpeedFlipTrainer::RotatorToOrientation(const Rotator& rotation) {
    Orientation result;

    float pitch = rotation.Pitch * CONST_PI_F / 32768.0f;
    float yaw = rotation.Yaw * CONST_PI_F / 32768.0f;
    float roll = rotation.Roll * CONST_PI_F / 32768.0f;

    float sp = sinf(pitch);
    float cp = cosf(pitch);
    float sy = sinf(yaw);
    float cy = cosf(yaw);
    float sr = sinf(roll);
    float cr = cosf(roll);

    result.forward.X = cp * cy;
    result.forward.Y = cp * sy;
    result.forward.Z = sp;

    result.right.X = cy * sr * sp + cr * sy;
    result.right.Y = sy * sr * sp - cr * cy;
    result.right.Z = -cp * sr;

    result.up.X = -cr * sp * cy + sr * sy;
    result.up.Y = -cr * sp * sy - sr * cy;
    result.up.Z = cp * cr;

    return result;
}


Matrix SpeedFlipTrainer::GetViewProjectionMatrix(CameraWrapper camera) {
    if (camera.IsNull()) return Matrix(); // Return identity or zero matrix

    Vector camLocation = camera.GetLocation();
    Rotator camRotation = camera.GetRotation();
    float FOV = camera.GetFOV();
    Vector2 screenSize = gameWrapper->GetScreenSize(); // Or pass canvas.GetSize()
    float aspect = (screenSize.Y > 0) ? screenSize.X / screenSize.Y : 16.0f / 9.0f;


    Orientation camOrientation = RotatorToOrientation(camRotation);
    Vector f = camOrientation.forward;
    Vector r = camOrientation.right;
    Vector u = camOrientation.up;

    Matrix viewMatrix;
    viewMatrix.M[0][0] = r.X; viewMatrix.M[0][1] = r.Y; viewMatrix.M[0][2] = r.Z; viewMatrix.M[0][3] = -Vector::dot(r, camLocation);
    viewMatrix.M[1][0] = u.X; viewMatrix.M[1][1] = u.Y; viewMatrix.M[1][2] = u.Z; viewMatrix.M[1][3] = -Vector::dot(u, camLocation);
    viewMatrix.M[2][0] = -f.X;viewMatrix.M[2][1] = -f.Y;viewMatrix.M[2][2] = -f.Z;viewMatrix.M[2][3] = Vector::dot(f, camLocation); // Standard view matrix looks along -Z
    viewMatrix.M[3][0] = 0;   viewMatrix.M[3][1] = 0;   viewMatrix.M[3][2] = 0;   viewMatrix.M[3][3] = 1;


    Matrix projMatrix;
    float nearPlane = 10.0f; // Typical near plane
    float farPlane = 30000.0f; // Typical far plane for Rocket League
    float fovRadians = FOV * (CONST_PI_F / 180.0f);
    float yScale = 1.0f / tanf(fovRadians / 2.0f);
    float xScale = yScale / aspect;

    projMatrix.M[0][0] = xScale; projMatrix.M[0][1] = 0;      projMatrix.M[0][2] = 0;                            projMatrix.M[0][3] = 0;
    projMatrix.M[1][0] = 0;      projMatrix.M[1][1] = yScale; projMatrix.M[1][2] = 0;                            projMatrix.M[1][3] = 0;
    projMatrix.M[2][0] = 0;      projMatrix.M[2][1] = 0;      projMatrix.M[2][2] = farPlane / (farPlane - nearPlane); projMatrix.M[2][3] = -(farPlane * nearPlane) / (farPlane - nearPlane);
    projMatrix.M[3][0] = 0;      projMatrix.M[3][1] = 0;      projMatrix.M[3][2] = 1;                            projMatrix.M[3][3] = 0;

    return projMatrix * viewMatrix; // Correct order: projection * view
}


Vector2 SpeedFlipTrainer::WorldToScreen(CanvasWrapper canvas, Vector location) {
    if (!gameWrapper) return Vector2{ 0,0 };
    CameraWrapper camera = gameWrapper->GetCamera();
    if (camera.IsNull()) return Vector2{ 0, 0 };

    Vector2 screenSize = canvas.GetSize();
    if (screenSize.X == 0 || screenSize.Y == 0) return Vector2{ 0,0 };

    Matrix viewProjMatrix = GetViewProjectionMatrix(camera);
    Vector4 locationVec4(location, 1.0f); // W=1 for positions
    Vector4 clipSpaceLocation = viewProjMatrix * locationVec4;

    if (clipSpaceLocation.W == 0.0f || clipSpaceLocation.W < 0.0f) { // Check for W <= 0 (behind camera or at infinity)
        return Vector2{ -1, -1 }; // Indicate point is not visible or problematic
    }

    // Perspective division
    float ndcX = clipSpaceLocation.X / clipSpaceLocation.W;
    float ndcY = clipSpaceLocation.Y / clipSpaceLocation.W;
    // float ndcZ = clipSpaceLocation.Z / clipSpaceLocation.W; // Z is not used for 2D screen coords

    // Convert NDC to screen coordinates
    // NDC range is [-1, 1]. Screen Y is often inverted in graphics APIs.
    int screenX = static_cast<int>((ndcX * 0.5f + 0.5f) * screenSize.X);
    int screenY = static_cast<int>((-ndcY * 0.5f + 0.5f) * screenSize.Y); // Invert Y for top-left origin

    return Vector2{ screenX, screenY };
}

bool SpeedFlipTrainer::IsPointOnScreen(const Vector2& point, float screenWidth, float screenHeight) {
    return point.X >= 0 && point.X <= screenWidth &&
        point.Y >= 0 && point.Y <= screenHeight;
}

void SpeedFlipTrainer::DrawArrow(CanvasWrapper& canvas, Vector2 start, Vector2 end, const CustomColor& color, int thickness) {
    canvas.SetColor(color.r, color.g, color.b, color.GetAlphaChar());
    canvas.DrawLine(start, end, thickness);

    float dx = end.X - start.X;
    float dy = end.Y - start.Y;
    float length = sqrtf(dx * dx + dy * dy);

    if (length < 1.0f) return;

    dx /= length; // Normalized direction vector
    dy /= length;

    float arrowSize = 10.0f;
    float arrowAngleDegrees = 30.0f; // Angle of arrowhead lines relative to the main line
    float arrowAngleRadians = arrowAngleDegrees * CONST_PI_F / 180.0f;

    // Point 1 of arrowhead
    // Rotated by +arrowAngleRadians
    float x1 = end.X - arrowSize * (dx * cosf(arrowAngleRadians) - dy * sinf(arrowAngleRadians));
    float y1 = end.Y - arrowSize * (dy * cosf(arrowAngleRadians) + dx * sinf(arrowAngleRadians));
    canvas.DrawLine(end, Vector2{ static_cast<int>(x1), static_cast<int>(y1) }, thickness);

    // Point 2 of arrowhead
    // Rotated by -arrowAngleRadians
    float x2 = end.X - arrowSize * (dx * cosf(-arrowAngleRadians) - dy * sinf(-arrowAngleRadians));
    float y2 = end.Y - arrowSize * (dy * cosf(-arrowAngleRadians) + dx * sinf(-arrowAngleRadians));
    canvas.DrawLine(end, Vector2{ static_cast<int>(x2), static_cast<int>(y2) }, thickness);
}

void SpeedFlipTrainer::RenderCarAxes(CanvasWrapper canvas) {
    if (!gameWrapper || !*enabled || !loaded || !*showCarAxes || !gameWrapper->IsInCustomTraining())
        return;

    CarWrapper car = gameWrapper->GetLocalCar();
    if (car.IsNull()) return;

    Vector carLocation = car.GetLocation();
    Rotator carRotation = car.GetRotation();
    Orientation orientation = RotatorToOrientation(carRotation);

    // Define colors using CustomColor
    CustomColor redColor(255, 0, 0);
    CustomColor greenColor(0, 255, 0);
    CustomColor blueColor(0, 0, 255);
    CustomColor orangeColor(255, 165, 0);
    CustomColor purpleColor(128, 0, 128);
    // CustomColor cyanColor(0, 255, 255); // Defined but not used in original logic for DrawArrow
    // CustomColor yellowColor(255, 255, 0); // Defined but not used
    CustomColor whiteColor(255, 255, 255);

    Vector2 screenSize = canvas.GetSize();
    Vector2 carPos2D = WorldToScreen(canvas, carLocation);

    if (!IsPointOnScreen(carPos2D, screenSize.X, screenSize.Y)) return; // Don't draw if car center is off-screen

    canvas.SetColor(whiteColor.r, whiteColor.g, whiteColor.b, whiteColor.GetAlphaChar());
    canvas.SetPosition(carPos2D - Vector2{ 2,2 }); // Center the box
    canvas.FillBox(Vector2{ 5, 5 });

    // End points of axes
    Vector forwardEnd = carLocation + orientation.forward * (*axisLength);
    Vector rightEnd = carLocation + orientation.right * (*axisLength);
    Vector upEnd = carLocation + orientation.up * (*axisLength);

    Vector2 forwardEnd2D = WorldToScreen(canvas, forwardEnd);
    Vector2 rightEnd2D = WorldToScreen(canvas, rightEnd);
    Vector2 upEnd2D = WorldToScreen(canvas, upEnd);

    if (IsPointOnScreen(forwardEnd2D, screenSize.X, screenSize.Y)) DrawArrow(canvas, carPos2D, forwardEnd2D, greenColor);
    if (IsPointOnScreen(rightEnd2D, screenSize.X, screenSize.Y)) DrawArrow(canvas, carPos2D, rightEnd2D, redColor);
    if (IsPointOnScreen(upEnd2D, screenSize.X, screenSize.Y)) DrawArrow(canvas, carPos2D, upEnd2D, blueColor);

    // Diagonal Axes
    float diagonalLength = (*axisLength) * 0.6f;
    Vector frontLeftDir = (orientation.forward - orientation.right).normalize();
    Vector frontRightDir = (orientation.forward + orientation.right).normalize();

    Vector2 frontLeftEnd2D = WorldToScreen(canvas, carLocation + frontLeftDir * diagonalLength);
    Vector2 frontRightEnd2D = WorldToScreen(canvas, carLocation + frontRightDir * diagonalLength);

    if (IsPointOnScreen(frontLeftEnd2D, screenSize.X, screenSize.Y)) DrawArrow(canvas, carPos2D, frontLeftEnd2D, orangeColor);
    if (IsPointOnScreen(frontRightEnd2D, screenSize.X, screenSize.Y)) DrawArrow(canvas, carPos2D, frontRightEnd2D, purpleColor);

    // Textual info
    canvas.SetColor(whiteColor.r, whiteColor.g, whiteColor.b, whiteColor.GetAlphaChar());
    int textY = screenSize.Y - 60;
    canvas.SetPosition(Vector2{ 10, textY });
    canvas.DrawString("Rouge (X): Droite | Vert (Y): Avant | Bleu (Z): Haut");
    textY += 20;
    canvas.SetPosition(Vector2{ 10, textY });
    canvas.DrawString("Orange: Avant-Gauche | Violet: Avant-Droit");
}


void SpeedFlipTrainer::RenderMeters(CanvasWrapper canvas) {
    if (!gameWrapper || !*enabled || !loaded || !gameWrapper->IsInCustomTraining()) return;

    Vector2 screenSize = canvas.GetSize(); // Get once

    if (*showAngleMeter) RenderAngleMeter(canvas, screenSize.X, screenSize.Y);
    if (*showPositionMeter) RenderPositionMeter(canvas, screenSize.X, screenSize.Y);
    if (*showFlipMeter) RenderFlipCancelMeter(canvas, screenSize.X, screenSize.Y);
    if (*showJumpMeter) RenderFirstJumpMeter(canvas, screenSize.X, screenSize.Y);
    if (*showCarAxes) RenderCarAxes(canvas); // Called from here
}

int ComputeDodgeAngle(DodgeComponentWrapper dodge) {
    if (dodge.IsNull()) return 0;
    Vector dd = dodge.GetDodgeDirection();
    if (dd.X == 0 && dd.Y == 0) return 0;
    return static_cast<int>(atan2f(dd.Y, dd.X) * (180.0f / CONST_PI_F));
}

clock_time ComputeClockTime(int angle) {
    if (angle < 0) angle += 360;
    clock_time time;
    time.hour_hand = static_cast<int>(angle * (12.0 / 360.0));
    if (time.hour_hand == 0) time.hour_hand = 12; // 0 should be 12 o'clock
    time.min_hand = (angle % (360 / 12)) * (60 / (360 / 12)); // Correct calculation for minutes on a clock face segment
    return time;
}

float distance(Vector a, Vector b) {
    return sqrt(pow(a.X - b.X, 2) + pow(a.Y - b.Y, 2) + pow(a.Z - b.Z, 2)); // Added Z component
}

void SpeedFlipTrainer::Measure(CarWrapper car, PriWrapper pri) { // Assuming PriWrapper holds input
    if (!gameWrapper || car.IsNull() || pri.IsNull()) return;

    int currentPhysicsFrame = gameWrapper->GetEngine().GetPhysicsFrame();
    int currentTick = currentPhysicsFrame - startingPhysicsFrame;
    if (currentTick < 0) return; // Not started yet

    ControllerInput input = car.GetInput(); // Or pri.GetInput() if that's where live input is
    attempt.Record(currentTick, input);

    Vector loc = car.GetLocation();
    // attempt.traveledY += abs(loc.Y - attempt.positionY); // This logic is for 2D top-down, might need re-evaluation for 3D path
    // For now, let's track displacement along the car's initial forward vector or just total distance from start
    if (attempt.pathPoints.empty()) {
        attempt.pathPoints.push_back(loc);
        attempt.totalDistanceTraveled = 0;
    }
    else {
        attempt.totalDistanceTraveled += distance(attempt.pathPoints.back(), loc);
        attempt.pathPoints.push_back(loc);
    }
    attempt.currentPosition = loc; // Store current full position

    if (!attempt.jumped && car.GetbJumped()) {
        attempt.jumped = true;
        attempt.jumpTick = currentTick;
        LOG("First jump: {} ticks", currentTick);
    }

    DodgeComponentWrapper dodge = car.GetDodgeComponent();
    if (!attempt.dodged && !dodge.IsNull() && dodge.GetDodgeTorque().X != 0) { // Check if dodge is active
        attempt.dodged = true;
        attempt.dodgedTick = currentTick;
        attempt.dodgeAngle = ComputeDodgeAngle(dodge);
        clock_time time = ComputeClockTime(attempt.dodgeAngle);
        LOG("Dodge Angle: {:03d} deg or {:02d}:{:02d}", attempt.dodgeAngle, time.hour_hand, time.min_hand);
    }

    if (input.Throttle < 0.9f) attempt.ticksNotPressingThrottle++; // Use a threshold for analog input
    if (!input.ActivateBoost) attempt.ticksNotPressingBoost++;

    if (attempt.dodged && !attempt.flipCanceled && input.Pitch > 0.8f) { // Still using Pitch for cancel
        attempt.flipCanceled = true;
        attempt.flipCancelTick = currentTick;
        LOG("Flip Cancel: {} ticks after dodge", attempt.flipCancelTick - attempt.dodgedTick);
    }
}


void SpeedFlipTrainer::Hook() {
    if (loaded) return;
    loaded = true;
    LOG("Hooking events");
    gameWrapper->RegisterDrawable(std::bind(&SpeedFlipTrainer::RenderMeters, this, std::placeholders::_1));

    if (*rememberSpeed) {
        CVarWrapper speedCvar = _globalCvarManager->getCvar("sv_soccar_gamespeed");
        if (speedCvar) speedCvar.setValue(*speed);
    }

    gameWrapper->HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.SetVehicleInput",
        [this](CarWrapper car, void* params, std::string eventname) {
            if (!gameWrapper || !*enabled || !loaded || !gameWrapper->IsInCustomTraining() || car.IsNull()) return;

            // Update speed from CVar if rememberSpeed is true
            if (*rememberSpeed) {
                CVarWrapper speedCvarRead = _globalCvarManager->getCvar("sv_soccar_gamespeed");
                if (speedCvarRead) *this->speed = speedCvarRead.getFloatValue();
            }

            ControllerInput* ci = (ControllerInput*)params;
            PriWrapper pri = car.GetPRI(); // Get PRI for the car
            if (pri.IsNull()) return;


            if (mode == SpeedFlipTrainerMode::Bot) {
                PlayBot(ci);
            }
            else if (mode == SpeedFlipTrainerMode::Replay) {
                PlayAttempt(&replayAttempt, ci);
            }


            ServerWrapper server = gameWrapper->GetCurrentGameState();
            if (server.IsNull()) return;
            float timeLeft = server.GetGameTimeRemaining();
            int currentFrame = gameWrapper->GetEngine().GetPhysicsFrame();

            if (initialTime <= 0 || timeLeft >= initialTime) { // Handles countdown not started or reset
                // If time is reset (e.g. goal scored and new kickoff), re-capture initialTime
                if (timeLeft > 0 && (initialTime <= 0 || timeLeft > initialTime + 0.1f /*debounce*/)) {
                    initialTime = timeLeft;
                    LOG("Initial time set to: {}", initialTime);
                }
                return;
            }

            if (startingPhysicsFrame < 0 && timeLeft < initialTime && timeLeft > 0) { // Countdown has started
                startingPhysicsFrame = currentFrame;
                LOG("Attempt started at physics frame: {}", startingPhysicsFrame);
                attempt = Attempt(); // Reset attempt data
                attempt.initialCarLocation = car.GetLocation(); // Store initial location

                if (!car.IsOnGround()) attempt.startedInAir = true;
                if (!ci->ActivateBoost) attempt.startedNoBoost = true; // Check current input at start
            }

            // Only measure if the attempt has started (startingPhysicsFrame is set)
            if (startingPhysicsFrame >= 0 && !attempt.exploded && !attempt.hit) {
                Measure(car, pri);
            }
        });

    gameWrapper->HookEvent("Function TAGame.Ball_TA.RecordCarHit",
        [this](std::string eventname) {
            if (!gameWrapper || !*enabled || !loaded || !gameWrapper->IsInCustomTraining() || attempt.hit || attempt.exploded || startingPhysicsFrame < 0) return;

            BallWrapper ball = gameWrapper->GetGameEventAsServer().GetBall();
            CarWrapper car = gameWrapper->GetLocalCar();
            if (ball.IsNull() || car.IsNull()) return;

            // Check if this car actually hit the ball
            // This might require checking the last touch or a more robust hit detection
            // For now, we assume any RecordCarHit during an active attempt is by the player.

            // Impact frame might not be perfectly synced, use current frame relative to start
            attempt.ticksToBall = gameWrapper->GetEngine().GetPhysicsFrame() - startingPhysicsFrame;
            attempt.timeToBall = initialTime - gameWrapper->GetCurrentGameState().GetGameTimeRemaining(); // Time elapsed
            attempt.hit = true;
            LOG("Ball hit: {:.3f}s after {} ticks", attempt.timeToBall, attempt.ticksToBall);
        });

    gameWrapper->HookEvent("Function TAGame.Ball_TA.Explode",
        [this](std::string eventName) {
            if (!gameWrapper || !*enabled || !loaded || !gameWrapper->IsInCustomTraining() || startingPhysicsFrame < 0) return;

            ServerWrapper server = gameWrapper->GetGameEventAsServer();
            if (server.IsNull()) return;
            BallWrapper ball = server.GetBall();
            CarWrapper car = server.GetLocalPrimaryPlayer().GetCar(); // Get car from PRI
            if (ball.IsNull() || car.IsNull()) return;

            float distToBall = distance(ball.GetLocation(), car.GetLocation());
            // float carRadius = 45; // Approximate radius of car collision cylinder
            // float ballRadius = 46; // Approximate radius of ball collision sphere
            // float meters = (distToBall - carRadius - ballRadius) / 100.0f; // Distance between surfaces
            float meters = distToBall / 100.0f; // Center to center distance in meters
            LOG("Ball exploded. Distance to ball center = {:.1f}m", meters);

            attempt.exploded = true; // Means a miss
            attempt.hit = false; // Can't hit if it exploded before contact
        });

    gameWrapper->HookEventPost("Function Engine.Controller.Restart", // Or "Function TAGame.GameEvent_Soccar_TA.ResetRound"
        [this](std::string eventName) {
            if (!gameWrapper || !*enabled || !loaded || !gameWrapper->IsInCustomTraining()) return;

            LOG("Round restarted (Controller.Restart)");

            ServerWrapper server = gameWrapper->GetCurrentGameState();
            if (!server.IsNull()) initialTime = server.GetGameTimeRemaining();
            else initialTime = 0; // Fallback

            startingPhysicsFrame = -1; // Reset for next attempt

            if (attempt.hit && !attempt.exploded) {
                consecutiveHits++;
                consecutiveMiss = 0;
            }
            else if (attempt.inputs.size() > 0) { // Only count as miss if an attempt was made
                consecutiveHits = 0;
                consecutiveMiss++;
            }

            if (*saveToFile && attempt.inputs.size() > 0) {
                if (!std::filesystem::exists(dataDir)) { // Ensure directory exists
                    std::filesystem::create_directories(dataDir);
                }
                auto path = attempt.GetFilename(dataDir / "attempts"); // Save in subdirectory
                if (!std::filesystem::exists(dataDir / "attempts")) {
                    std::filesystem::create_directories(dataDir / "attempts");
                }
                attempt.WriteInputsToFile(path);
                LOG("Saved attempt to: {}", path.string());
            }

            CVarWrapper speedCvar = _globalCvarManager->getCvar("sv_soccar_gamespeed");
            if (!speedCvar) return;
            float currentSpeed = speedCvar.getFloatValue();

            if (*changeSpeed) {
                bool speedChanged = false;
                if (consecutiveHits > 0 && consecutiveHits % (*numHitsChangedSpeed) == 0) {
                    gameWrapper->LogToChatbox(std::to_string(consecutiveHits) + (consecutiveHits > 1 ? " hits" : " hit") + " in a row!");
                    currentSpeed += *speedIncrement;
                    speedChanged = true;
                }
                else if (consecutiveMiss > 0 && consecutiveMiss % (*numHitsChangedSpeed) == 0) {
                    gameWrapper->LogToChatbox(std::to_string(consecutiveMiss) + (consecutiveMiss > 1 ? " misses" : " miss") + " in a row.");
                    currentSpeed -= *speedIncrement;
                    if (currentSpeed < 0.1f) currentSpeed = 0.1f; // Minimum speed
                    speedChanged = true;
                }

                if (speedChanged) {
                    speedCvar.setValue(currentSpeed);
                    *this->speed = currentSpeed; // Update plugin's speed variable
                    LOG("Game speed changed to: {:.3f}", currentSpeed);
                    gameWrapper->LogToChatbox(fmt::format("Game speed set to: {:.0f}%", currentSpeed * 100));
                }
            }
        });
}

void SpeedFlipTrainer::onLoad() {
    _globalCvarManager = cvarManager;
    LOG("SpeedFlipTrainer onLoad start");

    // Bind CVars to shared_ptr members
    cvarManager->registerCvar("sf_show_axes", "1", "Show car orientation axes.", true, true, 0, true, 1)
        .bindTo(showCarAxes);
    cvarManager->registerCvar("sf_axis_length", "150.0", "Length of car orientation axes.", true, true, 50.0f, true, 500.0f)
        .bindTo(axisLength);
    cvarManager->registerCvar("sf_enabled", "1", "Enable Speedflip trainer plugin.", true, true, 0, true, 1)
        .bindTo(enabled);
    cvarManager->getCvar("sf_enabled").addOnValueChanged([this](const std::string& oldVal, CVarWrapper cvar) {
        if (*enabled) Hook(); else onUnload(); // Hook/unhook based on new value
        });

    cvarManager->registerCvar("sf_show_angle", "1", "Show dodge angle meter.").bindTo(showAngleMeter);
    cvarManager->registerCvar("sf_show_position", "1", "Show horizontal position meter.").bindTo(showPositionMeter);
    cvarManager->registerCvar("sf_show_jump", "1", "Show first jump timing meter.").bindTo(showJumpMeter);
    cvarManager->registerCvar("sf_show_flip", "1", "Show flip cancel timing meter.").bindTo(showFlipMeter);

    cvarManager->registerCvar("sf_save_attempts", "0", "Save attempts to a file.").bindTo(saveToFile);
    cvarManager->registerCvar("sf_change_speed", "0", "Change game speed on consecutive hits/misses.").bindTo(changeSpeed);
    cvarManager->registerCvar("sf_speed", "1.0", "Current game speed multiplier for training.").bindTo(speed); // Should be float
    cvarManager->registerCvar("sf_remember_speed", "1", "Remember last set speed.").bindTo(rememberSpeed);
    cvarManager->registerCvar("sf_num_hits", "3", "Number of hits/misses for speed change.").bindTo(numHitsChangedSpeed);
    cvarManager->registerCvar("sf_speed_increment", "0.05", "Speed increment/decrement value.").bindTo(speedIncrement);

    cvarManager->registerCvar("sf_left_angle", "-30", "Optimal left dodge angle (degrees).").bindTo(optimalLeftAngle);
    cvarManager->registerCvar("sf_right_angle", "30", "Optimal right dodge angle (degrees).").bindTo(optimalRightAngle);
    cvarManager->registerCvar("sf_cancel_threshold", "13", "Optimal flip cancel threshold (ticks).").bindTo(flipCancelThreshold); // Defaulting to 13 as per analysis

    // CVars for jump timing (used in RenderFirstJumpMeter)
    cvarManager->registerCvar("sf_jump_low", "40", "Low threshold for first jump (ticks).").bindTo(jumpLow);
    cvarManager->registerCvar("sf_jump_high", "90", "High threshold for first jump (ticks).").bindTo(jumpHigh);


    if (gameWrapper) { // Ensure gameWrapper is available
        dataDir = gameWrapper->GetDataFolder() / "SpeedFlipTrainer"; // Plugin specific folder
        if (!std::filesystem::exists(dataDir)) {
            std::filesystem::create_directories(dataDir);
        }
        if (!std::filesystem::exists(dataDir / "attempts")) {
            std::filesystem::create_directories(dataDir / "attempts");
        }
        if (!std::filesystem::exists(dataDir / "bots")) {
            std::filesystem::create_directories(dataDir / "bots");
        }

        // Setup ImGuiFileDialog
        // Attempt File Dialog
        attemptFileDialog.SetTitle("Select Replay Attempt");
        attemptFileDialog.SetTypeFilters({ ".txt" });
        attemptFileDialog.SetPwd(dataDir / "attempts");

        // Bot File Dialog
        botFileDialog.SetTitle("Select Bot File");
        botFileDialog.SetTypeFilters({ ".txt" });
        botFileDialog.SetPwd(dataDir / "bots");


        // Hook into training pack loading if plugin is enabled
        // Check if already in Musty's pack when plugin loads
        if (*enabled) {
            if (gameWrapper->IsInCustomTraining()) {
                TrainingEditorWrapper trainingEditor = gameWrapper->GetTrainingEditor();
                if (IsMustysPack(trainingEditor)) {
                    Hook();
                }
            }
            // Hook for when training is loaded/changed
            gameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.GameEvent_TrainingEditor_TA.LoadRound",
                [this](ActorWrapper cw, void* params, std::string eventName) {
                    if (*enabled && IsMustysPack(TrainingEditorWrapper(cw.memory_address))) {
                        Hook();
                    }
                    else {
                        onUnload(); // Unhook if not Musty's pack or plugin disabled
                    }
                });

            gameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.GameEvent_TrainingEditor_TA.Destroyed",
                [this](ActorWrapper cw, void* params, std::string eventName) {
                    if (loaded) onUnload(); // Unhook when leaving training
                });
        }
    }
    else {
        LOG("gameWrapper is null during onLoad, cannot set up data directory or hooks correctly.");
    }
    LOG("SpeedFlipTrainer onLoad finished.");
}


void SpeedFlipTrainer::onUnload() {
    if (!loaded) return;
    loaded = false;
    LOG("Unhooking events and unregistering drawables");
    gameWrapper->UnhookEvent("Function TAGame.Car_TA.SetVehicleInput");
    gameWrapper->UnhookEvent("Function TAGame.Ball_TA.RecordCarHit");
    gameWrapper->UnhookEvent("Function TAGame.Ball_TA.Explode");
    gameWrapper->UnhookEventPost("Function Engine.Controller.Restart");
    gameWrapper->UnregisterDrawables();
}

bool SpeedFlipTrainer::IsMustysPack(TrainingEditorWrapper tw) {
    if (tw.IsNull()) return false;
    GameEditorSaveDataWrapper data = tw.GetTrainingData();
    if (data.IsNull()) return false;
    TrainingEditorSaveDataWrapper td = data.GetTrainingData();
    if (td.IsNull() || td.GetCode().IsNull()) return false;
    return td.GetCode().ToString() == "A503-264C-A7EB-D282";
}

// --- Meter Rendering Functions ---
// Using CustomColor where appropriate, and ensuring correct types for list elements.

void SpeedFlipTrainer::RenderPositionMeter(CanvasWrapper canvas, float screenWidth, float screenHeight) {
    // This meter seems to track deviation from a Y-centerline.
    // The original logic for `relLocation` and `range` implies a specific setup.
    // For now, focusing on fixing type errors.

    // Original logic:
    // float mid = -1.1; // Unused
    // int range = 200; // Half-width of the meter in abstract units
    // int relLocation = (-1 * attempt.currentPosition.Y) + range; // currentPosition.Y is world units
    // int totalUnits = range * 2;

    // Let's assume the meter shows deviation from the initial Y position.
    // Max deviation to show: +/- 2000 world units (e.g.)
    float maxDeviation = 2000.0f;
    float currentYDeviation = attempt.currentPosition.Y - attempt.initialCarLocation.Y;

    // Map currentYDeviation to meter range [0, totalUnits]
    // Meter center (0 deviation) should be at totalUnits / 2
    int totalMeterUnits = 400; // Arbitrary number of "slots" in the meter display
    int centerMark = totalMeterUnits / 2;
    // Scale: how many world units per meter unit
    float scale = (maxDeviation * 2) / totalMeterUnits;
    int meterValue = centerMark + static_cast<int>(currentYDeviation / scale);
    meterValue = std::max(0, std::min(totalMeterUnits, meterValue));


    float opacity = 1.0f;
    Vector2 reqSize = { static_cast<int>(screenWidth * 0.7f), static_cast<int>(screenHeight * 0.04f) };
    Vector2 startPos = { static_cast<int>((screenWidth / 2) - (reqSize.X / 2)), static_cast<int>(screenHeight * 0.1f) };

    CustomColor baseC(255, 255, 255, opacity);
    LineStyle borderS(CustomColor(255, 255, 255, opacity), 2);

    std::list<MeterRange> ranges;
    // Example ranges: green for small deviation, yellow for medium, red for large
    int greenZone = static_cast<int>(totalMeterUnits * 0.1); // +/- 10% deviation
    int yellowZone = static_cast<int>(totalMeterUnits * 0.3); // +/- 30% deviation

    // Green zone
    ranges.push_back({ CustomColor(50, 255, 50, 0.7f), centerMark - greenZone, centerMark + greenZone });
    // Yellow zones
    ranges.push_back({ CustomColor(255, 255, 50, 0.7f), centerMark - yellowZone, centerMark - greenZone });
    ranges.push_back({ CustomColor(255, 255, 50, 0.7f), centerMark + greenZone, centerMark + yellowZone });
    // Red zones (outer)
    ranges.push_back({ CustomColor(255, 50, 50, 0.7f), 0, centerMark - yellowZone });
    ranges.push_back({ CustomColor(255, 50, 50, 0.7f), centerMark + yellowZone, totalMeterUnits });


    std::list<MeterMarking> markings;
    markings.push_back({ CustomColor(255,255,255,opacity), 1, centerMark - greenZone });
    markings.push_back({ CustomColor(255,255,255,opacity), 1, centerMark + greenZone });
    markings.push_back({ CustomColor(255,255,255,opacity), 1, centerMark - yellowZone });
    markings.push_back({ CustomColor(255,255,255,opacity), 1, centerMark + yellowZone });
    markings.push_back({ CustomColor(0,0,0,0.6f), 2, meterValue }); // Current position marker

    RenderMeter(canvas, startPos, reqSize, baseC, borderS, totalMeterUnits, ranges, markings, false);

    // Game speed display
    CVarWrapper speedCvar = _globalCvarManager->getCvar("sv_soccar_gamespeed");
    if (speedCvar) {
        float gameSpeed = speedCvar.getFloatValue();
        std::string speedMsg = fmt::format("Game Speed: {:.0f}%", gameSpeed * 100);
        Vector2 textSize = canvas.GetStringSize(speedMsg);
        canvas.SetColor(255, 255, 255, static_cast<unsigned char>(255 * opacity));
        canvas.SetPosition(Vector2{ startPos.X + reqSize.X - textSize.X - 5, startPos.Y - textSize.Y - 2 });
        canvas.DrawString(speedMsg);
    }

    // Boost/Throttle warnings
    int textYOffset = reqSize.Y + 5;
    if (attempt.ticksNotPressingBoost > 0) {
        std::string boostMsg = fmt::format("No Boost: {}ms", static_cast<int>(attempt.ticksNotPressingBoost / 120.0f * 1000.0f));
        canvas.SetColor(255, 255, 50, static_cast<unsigned char>(255 * opacity));
        canvas.SetPosition(Vector2{ startPos.X, startPos.Y + textYOffset });
        canvas.DrawString(boostMsg);
        textYOffset += 15;
    }
    if (attempt.ticksNotPressingThrottle > 0) {
        std::string throttleMsg = fmt::format("No Throttle: {}ms", static_cast<int>(attempt.ticksNotPressingThrottle / 120.0f * 1000.0f));
        canvas.SetColor(255, 255, 50, static_cast<unsigned char>(255 * opacity));
        canvas.SetPosition(Vector2{ startPos.X, startPos.Y + textYOffset });
        canvas.DrawString(throttleMsg);
    }
}


void SpeedFlipTrainer::RenderFirstJumpMeter(CanvasWrapper canvas, float screenWidth, float screenHeight) {
    int minTicks = *jumpLow;  // e.g., 40 ticks
    int maxTicks = *jumpHigh; // e.g., 90 ticks
    int totalMeterUnits = maxTicks - minTicks;
    if (totalMeterUnits <= 0) return;

    // Define "good" zone, e.g. 50-60 ticks for a speedflip first jump
    int optimalLow = 50 - minTicks; // Optimal low relative to meter start
    int optimalHigh = 60 - minTicks; // Optimal high relative to meter start
    optimalLow = std::max(0, std::min(totalMeterUnits, optimalLow));
    optimalHigh = std::max(0, std::min(totalMeterUnits, optimalHigh));


    float opacity = 1.0f;
    Vector2 reqSize = { static_cast<int>(screenWidth * 0.02f), static_cast<int>(screenHeight * 0.56f) };
    Vector2 startPos = { static_cast<int>((screenWidth * 0.75f) + 2.5f * reqSize.X), static_cast<int>((screenHeight * 0.8f) - reqSize.Y) };

    CustomColor baseC(255, 255, 255, opacity);
    LineStyle borderS(CustomColor(255, 255, 255, opacity), 2);

    std::list<MeterMarking> markings;
    markings.push_back({ CustomColor(200,200,200,opacity), 1, optimalLow });
    markings.push_back({ CustomColor(200,200,200,opacity), 1, optimalHigh });


    std::list<MeterRange> ranges;
    // Green zone (optimal)
    ranges.push_back({ CustomColor(50, 255, 50, 0.7f), optimalLow, optimalHigh });
    // Yellow zones (acceptable but not perfect)
    int yellowBuffer = 5; // 5 ticks buffer for yellow
    ranges.push_back({ CustomColor(255, 255, 50, 0.7f), std::max(0, optimalLow - yellowBuffer), optimalLow });
    ranges.push_back({ CustomColor(255, 255, 50, 0.7f), optimalHigh, std::min(totalMeterUnits, optimalHigh + yellowBuffer) });
    // Red zones (too early/late)
    ranges.push_back({ CustomColor(255, 50, 50, 0.7f), 0, std::max(0, optimalLow - yellowBuffer) });
    ranges.push_back({ CustomColor(255, 50, 50, 0.7f), std::min(totalMeterUnits, optimalHigh + yellowBuffer), totalMeterUnits });


    if (attempt.jumped) {
        int jumpTickRelative = attempt.jumpTick - minTicks;
        jumpTickRelative = std::max(0, std::min(totalMeterUnits, jumpTickRelative));
        markings.push_back({ CustomColor(0,0,0,0.8f), 2, jumpTickRelative }); // Current jump tick marker
    }

    RenderMeter(canvas, startPos, reqSize, baseC, borderS, totalMeterUnits, ranges, markings, true);

    std::string label = "First Jump";
    Vector2 labelSize = canvas.GetStringSize(label);
    canvas.SetColor(255, 255, 255, static_cast<unsigned char>(255 * opacity));
    canvas.SetPosition(Vector2{ startPos.X + reqSize.X / 2 - labelSize.X / 2, startPos.Y + reqSize.Y + 8 });
    canvas.DrawString(label);

    if (attempt.jumped) {
        std::string msLabel = fmt::format("{}ms", static_cast<int>(attempt.jumpTick / 120.0f * 1000.0f));
        Vector2 msLabelSize = canvas.GetStringSize(msLabel);
        canvas.SetPosition(Vector2{ startPos.X + reqSize.X / 2 - msLabelSize.X / 2, startPos.Y + reqSize.Y + 8 + 15 });
        canvas.DrawString(msLabel);
    }
}

void SpeedFlipTrainer::RenderFlipCancelMeter(CanvasWrapper canvas, float screenWidth, float screenHeight) {
    float opacity = 1.0f;
    int totalMeterUnits = *flipCancelThreshold * 2; // Give some room for late cancels, up to 2x threshold
    int idealCancelPoint = *flipCancelThreshold; // Target is the threshold itself or slightly less

    Vector2 reqSize = { static_cast<int>(screenWidth * 0.02f), static_cast<int>(screenHeight * 0.55f) };
    Vector2 startPos = { static_cast<int>(screenWidth * 0.75f), static_cast<int>((screenHeight * 0.8f) - reqSize.Y) };

    CustomColor baseC(255, 255, 255, opacity);
    LineStyle borderS(CustomColor(255, 255, 255, opacity), 2);

    std::list<MeterRange> ranges;
    // Green: 0 to idealCancelPoint
    ranges.push_back({ CustomColor(50, 255, 50, 0.7f), 0, idealCancelPoint });
    // Yellow: idealCancelPoint to idealCancelPoint + 50% of threshold
    ranges.push_back({ CustomColor(255, 255, 50, 0.7f), idealCancelPoint, static_cast<int>(idealCancelPoint * 1.5f) });
    // Red: Beyond that
    ranges.push_back({ CustomColor(255, 50, 50, 0.7f), static_cast<int>(idealCancelPoint * 1.5f), totalMeterUnits });

    std::list<MeterMarking> markings;
    markings.push_back({ CustomColor(200,200,200,opacity), 1, idealCancelPoint });
    markings.push_back({ CustomColor(200,200,200,opacity), 1, static_cast<int>(idealCancelPoint * 1.5f) });


    if (attempt.flipCanceled) {
        int ticksAfterDodge = attempt.flipCancelTick - attempt.dodgedTick;
        ticksAfterDodge = std::max(0, std::min(totalMeterUnits, ticksAfterDodge));
        markings.push_back({ CustomColor(0,0,0,0.8f), 2, ticksAfterDodge });
    }

    RenderMeter(canvas, startPos, reqSize, baseC, borderS, totalMeterUnits, ranges, markings, true);

    std::string label = "Flip Cancel";
    Vector2 labelSize = canvas.GetStringSize(label);
    canvas.SetColor(255, 255, 255, static_cast<unsigned char>(255 * opacity));
    canvas.SetPosition(Vector2{ startPos.X + reqSize.X / 2 - labelSize.X / 2, startPos.Y + reqSize.Y + 8 });
    canvas.DrawString(label);

    if (attempt.flipCanceled) {
        int ticksForCancel = attempt.flipCancelTick - attempt.dodgedTick;
        std::string msLabel = fmt::format("{}ms", static_cast<int>(ticksForCancel / 120.0f * 1000.0f));
        Vector2 msLabelSize = canvas.GetStringSize(msLabel);
        canvas.SetPosition(Vector2{ startPos.X + reqSize.X / 2 - msLabelSize.X / 2, startPos.Y + reqSize.Y + 8 + 15 });
        canvas.DrawString(msLabel);
    }
}

void SpeedFlipTrainer::RenderAngleMeter(CanvasWrapper canvas, float screenWidth, float screenHeight) {
    int totalMeterUnits = 180; // Represents -90 to +90 degrees
    int centerAngle = 90;  // 0 degrees will be at mark 90

    float opacity = 1.0f;
    Vector2 reqSize = { static_cast<int>(screenWidth * 0.66f), static_cast<int>(screenHeight * 0.04f) };
    Vector2 startPos = { static_cast<int>((screenWidth / 2) - (reqSize.X / 2)), static_cast<int>(screenHeight * 0.9f) };

    CustomColor baseC(255, 255, 255, opacity);
    LineStyle borderS(CustomColor(255, 255, 255, opacity), 2);

    std::list<MeterRange> ranges;
    std::list<MeterMarking> markings;

    int greenRangeWidth = 8; // Degrees for green zone around optimal
    int yellowRangeWidth = 15; // Degrees for yellow zone

    // Left optimal angle markings/ranges
    int lTargetMeter = *optimalLeftAngle + centerAngle;
    markings.push_back({ CustomColor(200,200,200,opacity), 1, lTargetMeter - greenRangeWidth });
    markings.push_back({ CustomColor(200,200,200,opacity), 1, lTargetMeter + greenRangeWidth });
    ranges.push_back({ CustomColor(50,255,50,0.7f), lTargetMeter - greenRangeWidth, lTargetMeter + greenRangeWidth });
    ranges.push_back({ CustomColor(255,255,50,0.7f), lTargetMeter - yellowRangeWidth, lTargetMeter - greenRangeWidth });
    ranges.push_back({ CustomColor(255,255,50,0.7f), lTargetMeter + greenRangeWidth, lTargetMeter + yellowRangeWidth });

    // Right optimal angle markings/ranges
    int rTargetMeter = *optimalRightAngle + centerAngle;
    markings.push_back({ CustomColor(200,200,200,opacity), 1, rTargetMeter - greenRangeWidth });
    markings.push_back({ CustomColor(200,200,200,opacity), 1, rTargetMeter + greenRangeWidth });
    ranges.push_back({ CustomColor(50,255,50,0.7f), rTargetMeter - greenRangeWidth, rTargetMeter + greenRangeWidth });
    ranges.push_back({ CustomColor(255,255,50,0.7f), rTargetMeter - yellowRangeWidth, rTargetMeter - greenRangeWidth });
    ranges.push_back({ CustomColor(255,255,50,0.7f), rTargetMeter + greenRangeWidth, rTargetMeter + yellowRangeWidth });

    // Add red ranges for areas outside yellow
    ranges.push_back({ CustomColor(255,50,50,0.7f), 0, std::min(lTargetMeter - yellowRangeWidth, rTargetMeter - yellowRangeWidth) });
    ranges.push_back({ CustomColor(255,50,50,0.7f), std::max(lTargetMeter + yellowRangeWidth, rTargetMeter + yellowRangeWidth), totalMeterUnits });
    // Red zone between the two yellow zones if they don't meet
    if (lTargetMeter + yellowRangeWidth < rTargetMeter - yellowRangeWidth) {
        ranges.push_back({ CustomColor(255,50,50,0.7f), lTargetMeter + yellowRangeWidth, rTargetMeter - yellowRangeWidth });
    }


    if (attempt.dodged) {
        int angleMeterValue = attempt.dodgeAngle + centerAngle;
        angleMeterValue = std::max(0, std::min(totalMeterUnits, angleMeterValue));
        markings.push_back({ CustomColor(0,0,0,0.8f), 2, angleMeterValue });
    }

    RenderMeter(canvas, startPos, reqSize, baseC, borderS, totalMeterUnits, ranges, markings, false);

    std::string angleText = "Dodge Angle: " + (attempt.dodged ? std::to_string(attempt.dodgeAngle) : "N/A") + " DEG";
    canvas.SetColor(255, 255, 255, static_cast<unsigned char>(255 * opacity));
    canvas.SetPosition(Vector2{ startPos.X, startPos.Y - 20 });
    canvas.DrawString(angleText);

    if (attempt.hit && attempt.ticksToBall > 0) {
        std::string timeToBallMsg = fmt::format("Time to Ball: {:.3f}s", attempt.timeToBall);
        Vector2 ttbSize = canvas.GetStringSize(timeToBallMsg);
        canvas.SetPosition(Vector2{ startPos.X + reqSize.X / 2 - ttbSize.X / 2, startPos.Y - 20 });
        canvas.DrawString(timeToBallMsg);
    }

    // Display total distance traveled (more relevant than Y-only for 3D)
    std::string distMsg = fmt::format("Path Length: {:.0f}uu", attempt.totalDistanceTraveled);
    Vector2 distMsgSize = canvas.GetStringSize(distMsg);
    CustomColor distColor = (attempt.totalDistanceTraveled < 2500) ? CustomColor(50, 255, 50, opacity) :
        (attempt.totalDistanceTraveled < 3500) ? CustomColor(255, 255, 50, opacity) :
        CustomColor(255, 50, 50, opacity);
    canvas.SetColor(distColor.r, distColor.g, distColor.b, distColor.GetAlphaChar());
    canvas.SetPosition(Vector2{ startPos.X + reqSize.X - distMsgSize.X - 5, startPos.Y - 20 });
    canvas.DrawString(distMsg);


    int warningYOffset = reqSize.Y + 5;
    if (attempt.startedInAir) {
        std::string airMsg = "WARNING: Started in air!";
        canvas.SetColor(255, 10, 10, static_cast<unsigned char>(255 * opacity));
        canvas.SetPosition(Vector2{ startPos.X, startPos.Y + warningYOffset });
        canvas.DrawString(airMsg);
        warningYOffset += 15;
    }
    if (attempt.startedNoBoost) {
        std::string noBoostMsg = "WARNING: Started without boost!";
        canvas.SetColor(255, 10, 10, static_cast<unsigned char>(255 * opacity));
        canvas.SetPosition(Vector2{ startPos.X, startPos.Y + warningYOffset });
        canvas.DrawString(noBoostMsg);
    }
}


// --- Bot/Replay Playback ---
void SpeedFlipTrainer::PlayAttempt(Attempt* att, ControllerInput* ci) {
    if (!gameWrapper || att->inputs.empty() || startingPhysicsFrame < 0) return;
    int tick = gameWrapper->GetEngine().GetPhysicsFrame() - startingPhysicsFrame;
    att->Play(ci, tick);
    gameWrapper->OverrideParams(ci, sizeof(ControllerInput));
}

void SpeedFlipTrainer::PlayBot(ControllerInput* ci) {
    if (!gameWrapper || bot.inputs.empty() || startingPhysicsFrame < 0) return;
    int tick = gameWrapper->GetEngine().GetPhysicsFrame() - startingPhysicsFrame;
    bot.Play(ci, tick);
    gameWrapper->OverrideParams(ci, sizeof(ControllerInput));
}


// Definition of RenderMeter (could be in RenderMeter.cpp if it gets complex)
// For now, keeping it here for self-containment of this file's fixes.
Vector2 RenderMeter(CanvasWrapper canvas, Vector2 startPos, Vector2 reqBoxSize, CustomColor baseColor,
    LineStyle borderStyle, int totalUnits, const std::list<MeterRange>& ranges,
    const std::list<MeterMarking>& markings, bool vertical, float currentValue) {

    // Calculate actual box size based on totalUnits and requested size (aspect ratio might change)
    Vector2 actualBoxSize = reqBoxSize;
    float unitPixelSize;

    if (vertical) {
        unitPixelSize = (totalUnits > 0) ? actualBoxSize.Y / static_cast<float>(totalUnits) : 0;
        // actualBoxSize.Y = unitPixelSize * totalUnits; // Optional: adjust box Y to perfectly fit units
    }
    else {
        unitPixelSize = (totalUnits > 0) ? actualBoxSize.X / static_cast<float>(totalUnits) : 0;
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
            rangePos.Y += (totalUnits - rHigh) * unitPixelSize; // Ranges drawn from top (high value) down
            rangeSize.Y = (rHigh - rLow) * unitPixelSize;
        }
        else {
            rangePos.X += rLow * unitPixelSize;
            rangeSize.X = (rHigh - rLow) * unitPixelSize;
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
            float yPos = startPos.Y + (totalUnits - mVal) * unitPixelSize;
            markStart = { startPos.X, yPos };
            markEnd = { startPos.X + actualBoxSize.X, yPos };
        }
        else {
            float xPos = startPos.X + mVal * unitPixelSize;
            markStart = { xPos, startPos.Y };
            markEnd = { xPos, startPos.Y + actualBoxSize.Y };
        }
        canvas.DrawLine(markStart, markEnd, mark.lineStyle.width);
    }

    // Draw current value if provided and not covered by markings
    if (currentValue >= 0) {
        // Similar logic to markings for drawing a line for currentValue
        // (This part was not in the original call, so it's an optional extension)
    }


    // Draw border
    canvas.SetColor(borderStyle.color.r, borderStyle.color.g, borderStyle.color.b, borderStyle.color.GetAlphaChar());
    canvas.SetPosition(startPos);
    canvas.DrawBox(actualBoxSize, borderStyle.width); // DrawBox usually takes size and thickness

    return startPos + actualBoxSize; // Return bottom-right corner or similar
}