#include "pch.h"
#include "SpeedFlipTrainer.h"
#include "RenderMeter.h" // Ensure this is after SpeedFlipTrainer.h

BAKKESMOD_PLUGIN(SpeedFlipTrainer, "Speedflip Trainer", plugin_version, PLUGINTYPE_CUSTOM_TRAINING)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;


// Helper struct for clock time
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
    flipCancelThreshold = std::make_shared<int>(13);
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
    if (camera.IsNull()) return Matrix();

    Vector camLocation = camera.GetLocation();
    Rotator camRotation = camera.GetRotation();
    float FOV = camera.GetFOV();
    Vector2 screenSize = gameWrapper->GetScreenSize();
    float aspect = (screenSize.Y > 0) ? static_cast<float>(screenSize.X) / screenSize.Y : 16.0f / 9.0f;


    Orientation camOrientation = RotatorToOrientation(camRotation);
    Vector f = camOrientation.forward;
    Vector r = camOrientation.right;
    Vector u = camOrientation.up;

    Matrix viewMatrix;
    viewMatrix.M[0][0] = r.X; viewMatrix.M[0][1] = r.Y; viewMatrix.M[0][2] = r.Z; viewMatrix.M[0][3] = -Vector::dot(r, camLocation);
    viewMatrix.M[1][0] = u.X; viewMatrix.M[1][1] = u.Y; viewMatrix.M[1][2] = u.Z; viewMatrix.M[1][3] = -Vector::dot(u, camLocation);
    viewMatrix.M[2][0] = -f.X;viewMatrix.M[2][1] = -f.Y;viewMatrix.M[2][2] = -f.Z;viewMatrix.M[2][3] = Vector::dot(f, camLocation);
    viewMatrix.M[3][0] = 0;   viewMatrix.M[3][1] = 0;   viewMatrix.M[3][2] = 0;   viewMatrix.M[3][3] = 1;


    Matrix projMatrix;
    float nearPlane = 10.0f;
    float farPlane = 30000.0f;
    float fovRadians = FOV * (CONST_PI_F / 180.0f);
    float yScale = 1.0f / tanf(fovRadians / 2.0f);
    float xScale = yScale / aspect;

    projMatrix.M[0][0] = xScale; projMatrix.M[0][1] = 0;      projMatrix.M[0][2] = 0;                            projMatrix.M[0][3] = 0;
    projMatrix.M[1][0] = 0;      projMatrix.M[1][1] = yScale; projMatrix.M[1][2] = 0;                            projMatrix.M[1][3] = 0;
    projMatrix.M[2][0] = 0;      projMatrix.M[2][1] = 0;      projMatrix.M[2][2] = farPlane / (farPlane - nearPlane); projMatrix.M[2][3] = -(farPlane * nearPlane) / (farPlane - nearPlane);
    projMatrix.M[3][0] = 0;      projMatrix.M[3][1] = 0;      projMatrix.M[3][2] = 1;                            projMatrix.M[3][3] = 0;

    return projMatrix * viewMatrix;
}


Vector2 SpeedFlipTrainer::WorldToScreen(CanvasWrapper canvas, Vector location) {
    if (!gameWrapper) return Vector2{ 0,0 };
    CameraWrapper camera = gameWrapper->GetCamera();
    if (camera.IsNull()) return Vector2{ 0, 0 };

    Vector2 screenSize = canvas.GetSize();
    if (screenSize.X == 0 || screenSize.Y == 0) return Vector2{ 0,0 };

    Matrix viewProjMatrix = GetViewProjectionMatrix(camera);
    Vector4 locationVec4(location, 1.0f);
    Vector4 clipSpaceLocation = viewProjMatrix * locationVec4;

    if (clipSpaceLocation.W == 0.0f || clipSpaceLocation.W < 0.0f) {
        return Vector2{ -1, -1 };
    }

    float ndcX = clipSpaceLocation.X / clipSpaceLocation.W;
    float ndcY = clipSpaceLocation.Y / clipSpaceLocation.W;

    int screenX = static_cast<int>((ndcX * 0.5f + 0.5f) * screenSize.X);
    int screenY = static_cast<int>((-ndcY * 0.5f + 0.5f) * screenSize.Y);

    return Vector2{ screenX, screenY };
}

bool SpeedFlipTrainer::IsPointOnScreen(const Vector2& point, float screenWidth, float screenHeight) {
    return point.X >= 0 && point.X <= screenWidth &&
        point.Y >= 0 && point.Y <= screenHeight;
}

void SpeedFlipTrainer::DrawArrow(CanvasWrapper& canvas, Vector2 start, Vector2 end, const CustomColor& color, int thickness) {
    canvas.SetColor(color.r, color.g, color.b, color.GetAlphaChar());
    canvas.DrawLine(start, end, thickness);

    float dx = static_cast<float>(end.X - start.X);
    float dy = static_cast<float>(end.Y - start.Y);
    float length = sqrtf(dx * dx + dy * dy);

    if (length < 1.0f) return;

    dx /= length;
    dy /= length;

    float arrowSize = 10.0f;
    float arrowAngleDegrees = 30.0f;
    float arrowAngleRadians = arrowAngleDegrees * CONST_PI_F / 180.0f;

    float x1 = end.X - arrowSize * (dx * cosf(arrowAngleRadians) - dy * sinf(arrowAngleRadians));
    float y1 = end.Y - arrowSize * (dy * cosf(arrowAngleRadians) + dx * sinf(arrowAngleRadians));
    canvas.DrawLine(end, Vector2{ static_cast<int>(x1), static_cast<int>(y1) }, thickness);

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

    CustomColor redColor(255, 0, 0);
    CustomColor greenColor(0, 255, 0);
    CustomColor blueColor(0, 0, 255);
    CustomColor orangeColor(255, 165, 0);
    CustomColor purpleColor(128, 0, 128);
    CustomColor whiteColor(255, 255, 255);

    Vector2 screenSize = canvas.GetSize();
    Vector2 carPos2D = WorldToScreen(canvas, carLocation);

    if (!IsPointOnScreen(carPos2D, static_cast<float>(screenSize.X), static_cast<float>(screenSize.Y))) return;

    canvas.SetColor(whiteColor.r, whiteColor.g, whiteColor.b, whiteColor.GetAlphaChar());
    canvas.SetPosition(carPos2D - Vector2{ 2,2 });
    canvas.FillBox(Vector2{ 5, 5 });

    Vector forwardEnd = carLocation + orientation.forward * (*axisLength);
    Vector rightEnd = carLocation + orientation.right * (*axisLength);
    Vector upEnd = carLocation + orientation.up * (*axisLength);

    Vector2 forwardEnd2D = WorldToScreen(canvas, forwardEnd);
    Vector2 rightEnd2D = WorldToScreen(canvas, rightEnd);
    Vector2 upEnd2D = WorldToScreen(canvas, upEnd);

    if (IsPointOnScreen(forwardEnd2D, static_cast<float>(screenSize.X), static_cast<float>(screenSize.Y))) DrawArrow(canvas, carPos2D, forwardEnd2D, greenColor);
    if (IsPointOnScreen(rightEnd2D, static_cast<float>(screenSize.X), static_cast<float>(screenSize.Y))) DrawArrow(canvas, carPos2D, rightEnd2D, redColor);
    if (IsPointOnScreen(upEnd2D, static_cast<float>(screenSize.X), static_cast<float>(screenSize.Y))) DrawArrow(canvas, carPos2D, upEnd2D, blueColor);

    float diagonalLength = (*axisLength) * 0.6f;
    Vector frontLeftDir_temp = orientation.forward - orientation.right;
    frontLeftDir_temp.normalize();
    Vector frontLeftDir = frontLeftDir_temp;

    Vector frontRightDir_temp = orientation.forward + orientation.right;
    frontRightDir_temp.normalize();
    Vector frontRightDir = frontRightDir_temp;


    Vector2 frontLeftEnd2D = WorldToScreen(canvas, carLocation + frontLeftDir * diagonalLength);
    Vector2 frontRightEnd2D = WorldToScreen(canvas, carLocation + frontRightDir * diagonalLength);

    if (IsPointOnScreen(frontLeftEnd2D, static_cast<float>(screenSize.X), static_cast<float>(screenSize.Y))) DrawArrow(canvas, carPos2D, frontLeftEnd2D, orangeColor);
    if (IsPointOnScreen(frontRightEnd2D, static_cast<float>(screenSize.X), static_cast<float>(screenSize.Y))) DrawArrow(canvas, carPos2D, frontRightEnd2D, purpleColor);

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

    Vector2 screenSize = canvas.GetSize();

    if (*showAngleMeter) RenderAngleMeter(canvas, static_cast<float>(screenSize.X), static_cast<float>(screenSize.Y));
    if (*showPositionMeter) RenderPositionMeter(canvas, static_cast<float>(screenSize.X), static_cast<float>(screenSize.Y));
    if (*showFlipMeter) RenderFlipCancelMeter(canvas, static_cast<float>(screenSize.X), static_cast<float>(screenSize.Y));
    if (*showJumpMeter) RenderFirstJumpMeter(canvas, static_cast<float>(screenSize.X), static_cast<float>(screenSize.Y));
    if (*showCarAxes) RenderCarAxes(canvas);
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
    if (time.hour_hand == 0) time.hour_hand = 12;
    time.min_hand = (angle % (360 / 12)) * (60 / (360 / 12));
    return time;
}

float distance(Vector a, Vector b) {
    return sqrt(pow(a.X - b.X, 2) + pow(a.Y - b.Y, 2) + pow(a.Z - b.Z, 2));
}

void SpeedFlipTrainer::Measure(CarWrapper car, PriWrapper pri) {
    if (!gameWrapper || car.IsNull() || pri.IsNull()) return;

    int currentPhysicsFrame = gameWrapper->GetEngine().GetPhysicsFrame();
    int currentTick = currentPhysicsFrame - startingPhysicsFrame;
    if (currentTick < 0) return;

    ControllerInput input = car.GetInput();
    attempt.Record(currentTick, input);

    Vector loc = car.GetLocation();
    // Ensure Attempt class has 'pathPoints', 'totalDistanceTraveled', and 'currentPosition' members
    if (attempt.pathPoints.empty()) {
        attempt.pathPoints.push_back(loc);
        attempt.totalDistanceTraveled = 0;
    }
    else {
        attempt.totalDistanceTraveled += distance(attempt.pathPoints.back(), loc);
        attempt.pathPoints.push_back(loc);
    }
    attempt.currentPosition = loc;

    if (!attempt.jumped && car.GetbJumped()) {
        attempt.jumped = true;
        attempt.jumpTick = currentTick;
        LOG("First jump: {} ticks", currentTick);
    }

    DodgeComponentWrapper dodge = car.GetDodgeComponent();
    if (!attempt.dodged && !dodge.IsNull() && dodge.GetDodgeTorque().X != 0) {
        attempt.dodged = true;
        attempt.dodgedTick = currentTick;
        attempt.dodgeAngle = ComputeDodgeAngle(dodge);
        clock_time time = ComputeClockTime(attempt.dodgeAngle);
        LOG("Dodge Angle: {:03d} deg or {:02d}:{:02d}", attempt.dodgeAngle, time.hour_hand, time.min_hand);
    }

    if (input.Throttle < 0.9f) attempt.ticksNotPressingThrottle++;
    if (!input.ActivateBoost) attempt.ticksNotPressingBoost++;

    if (attempt.dodged && !attempt.flipCanceled && input.Pitch > 0.8f) {
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

            if (*rememberSpeed) {
                CVarWrapper speedCvarRead = _globalCvarManager->getCvar("sv_soccar_gamespeed");
                if (speedCvarRead) *this->speed = speedCvarRead.getFloatValue();
            }

            ControllerInput* ci = (ControllerInput*)params;
            PriWrapper pri = car.GetPRI();
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

            if (initialTime <= 0 || timeLeft >= initialTime) {
                if (timeLeft > 0 && (initialTime <= 0 || timeLeft > initialTime + 0.1f)) {
                    initialTime = timeLeft;
                    LOG("Initial time set to: {}", initialTime);
                }
                return;
            }

            if (startingPhysicsFrame < 0 && timeLeft < initialTime && timeLeft > 0) {
                startingPhysicsFrame = currentFrame;
                LOG("Attempt started at physics frame: {}", startingPhysicsFrame);
                attempt = Attempt();
                // Ensure Attempt class has 'initialCarLocation' member
                attempt.initialCarLocation = car.GetLocation();

                if (!car.IsOnGround()) attempt.startedInAir = true;
                if (!ci->ActivateBoost) attempt.startedNoBoost = true;
            }

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

            attempt.ticksToBall = gameWrapper->GetEngine().GetPhysicsFrame() - startingPhysicsFrame;
            attempt.timeToBall = initialTime - gameWrapper->GetCurrentGameState().GetGameTimeRemaining();
            attempt.hit = true;
            LOG("Ball hit: {:.3f}s after {} ticks", attempt.timeToBall, attempt.ticksToBall);
        });

    gameWrapper->HookEvent("Function TAGame.Ball_TA.Explode",
        [this](std::string eventName) {
            if (!gameWrapper || !*enabled || !loaded || !gameWrapper->IsInCustomTraining() || startingPhysicsFrame < 0) return;

            ServerWrapper server = gameWrapper->GetGameEventAsServer();
            if (server.IsNull()) return;
            BallWrapper ball = server.GetBall();
            CarWrapper car = server.GetLocalPrimaryPlayer().GetCar();
            if (ball.IsNull() || car.IsNull()) return;

            float distToBall = distance(ball.GetLocation(), car.GetLocation());
            float meters = distToBall / 100.0f;
            LOG("Ball exploded. Distance to ball center = {:.1f}m", meters);

            attempt.exploded = true;
            attempt.hit = false;
        });

    gameWrapper->HookEventPost("Function Engine.Controller.Restart",
        [this](std::string eventName) {
            if (!gameWrapper || !*enabled || !loaded || !gameWrapper->IsInCustomTraining()) return;

            LOG("Round restarted (Controller.Restart)");

            ServerWrapper server = gameWrapper->GetCurrentGameState();
            if (!server.IsNull()) initialTime = server.GetGameTimeRemaining();
            else initialTime = 0;

            startingPhysicsFrame = -1;

            if (attempt.hit && !attempt.exploded) {
                consecutiveHits++;
                consecutiveMiss = 0;
            }
            // Ensure Attempt class has 'inputs' member (std::vector or similar with .size())
            else if (attempt.inputs.size() > 0) {
                consecutiveHits = 0;
                consecutiveMiss++;
            }

            if (*saveToFile && attempt.inputs.size() > 0) {
                if (!std::filesystem::exists(dataDir)) {
                    std::filesystem::create_directories(dataDir);
                }
                auto attemptSubDir = dataDir / "attempts";
                if (!std::filesystem::exists(attemptSubDir)) {
                    std::filesystem::create_directories(attemptSubDir);
                }
                auto path = attempt.GetFilename(attemptSubDir);
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
                    if (currentSpeed < 0.1f) currentSpeed = 0.1f;
                    speedChanged = true;
                }

                if (speedChanged) {
                    speedCvar.setValue(currentSpeed);
                    *this->speed = currentSpeed;
                    LOG("Game speed changed to: {:.3f}", currentSpeed);
                    gameWrapper->LogToChatbox(fmt::format("Game speed set to: {:.0f}%", currentSpeed * 100));
                }
            }
        });
}

void SpeedFlipTrainer::onLoad() {
    _globalCvarManager = cvarManager;
    LOG("SpeedFlipTrainer onLoad start");

    cvarManager->registerCvar("sf_show_axes", "1", "Show car orientation axes.", true, true, 0, true, 1)
        .bindTo(showCarAxes);
    cvarManager->registerCvar("sf_axis_length", "150.0", "Length of car orientation axes.", true, true, 50.0f, true, 500.0f)
        .bindTo(axisLength);
    cvarManager->registerCvar("sf_enabled", "1", "Enable Speedflip trainer plugin.", true, true, 0, true, 1)
        .bindTo(enabled);
    cvarManager->getCvar("sf_enabled").addOnValueChanged([this](const std::string& oldVal, CVarWrapper cvar) {
        if (*enabled) Hook(); else onUnload();
        });

    cvarManager->registerCvar("sf_show_angle", "1", "Show dodge angle meter.").bindTo(showAngleMeter);
    cvarManager->registerCvar("sf_show_position", "1", "Show horizontal position meter.").bindTo(showPositionMeter);
    cvarManager->registerCvar("sf_show_jump", "1", "Show first jump timing meter.").bindTo(showJumpMeter);
    cvarManager->registerCvar("sf_show_flip", "1", "Show flip cancel timing meter.").bindTo(showFlipMeter);

    cvarManager->registerCvar("sf_save_attempts", "0", "Save attempts to a file.").bindTo(saveToFile);
    cvarManager->registerCvar("sf_change_speed", "0", "Change game speed on consecutive hits/misses.").bindTo(changeSpeed);
    cvarManager->registerCvar("sf_speed", "1.0", "Current game speed multiplier for training.", true, true, 0.1f, true, 2.0f).bindTo(speed);
    cvarManager->registerCvar("sf_remember_speed", "1", "Remember last set speed.").bindTo(rememberSpeed);
    cvarManager->registerCvar("sf_num_hits", "3", "Number of hits/misses for speed change.").bindTo(numHitsChangedSpeed);
    cvarManager->registerCvar("sf_speed_increment", "0.05", "Speed increment/decrement value.").bindTo(speedIncrement);

    cvarManager->registerCvar("sf_left_angle", "-30", "Optimal left dodge angle (degrees).").bindTo(optimalLeftAngle);
    cvarManager->registerCvar("sf_right_angle", "30", "Optimal right dodge angle (degrees).").bindTo(optimalRightAngle);
    cvarManager->registerCvar("sf_cancel_threshold", "13", "Optimal flip cancel threshold (ticks).").bindTo(flipCancelThreshold);

    cvarManager->registerCvar("sf_jump_low", "40", "Low threshold for first jump (ticks).").bindTo(jumpLow);
    cvarManager->registerCvar("sf_jump_high", "90", "High threshold for first jump (ticks).").bindTo(jumpHigh);


    if (gameWrapper) {
        dataDir = gameWrapper->GetDataFolder() / "SpeedFlipTrainer";
        if (!std::filesystem::exists(dataDir)) {
            std::filesystem::create_directories(dataDir);
        }
        std::filesystem::path attemptsPath = dataDir / "attempts";
        if (!std::filesystem::exists(attemptsPath)) {
            std::filesystem::create_directories(attemptsPath);
        }
        std::filesystem::path botsPath = dataDir / "bots";
        if (!std::filesystem::exists(botsPath)) {
            std::filesystem::create_directories(botsPath);
        }

        // Setup ImGuiFileDialog instances
        attemptFileDialog.SetTitle("Select Replay Attempt");
        attemptFileDialog.SetTypeFilters({ ".txt" });
        if (std::filesystem::exists(attemptsPath)) attemptFileDialog.SetPwd(attemptsPath); else attemptFileDialog.SetPwd(dataDir);


        botFileDialog.SetTitle("Select Bot File");
        botFileDialog.SetTypeFilters({ ".txt" });
        if (std::filesystem::exists(botsPath)) botFileDialog.SetPwd(botsPath); else botFileDialog.SetPwd(dataDir);


        if (*enabled) {
            if (gameWrapper->IsInCustomTraining()) {
                // The following line might be the source of E0135 if GetTrainingEditor() is not a member of GameWrapper in your SDK version
                TrainingEditorWrapper trainingEditor = gameWrapper->GetTrainingEditor();
                if (IsMustysPack(trainingEditor)) {
                    Hook();
                }
            }
            gameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.GameEvent_TrainingEditor_TA.LoadRound",
                [this](ActorWrapper cw, void* params, std::string eventName) {
                    if (*enabled && IsMustysPack(TrainingEditorWrapper(cw.memory_address))) {
                        Hook();
                    }
                    else {
                        onUnload();
                    }
                });

            gameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.GameEvent_TrainingEditor_TA.Destroyed",
                [this](ActorWrapper cw, void* params, std::string eventName) {
                    if (loaded) onUnload();
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

void SpeedFlipTrainer::RenderPositionMeter(CanvasWrapper canvas, float screenWidth, float screenHeight) {
    float maxDeviation = 2000.0f;
    // Ensure Attempt class has 'currentPosition' and 'initialCarLocation' members
    float currentYDeviation = attempt.currentPosition.Y - attempt.initialCarLocation.Y;

    int totalMeterUnits = 400;
    int centerMark = totalMeterUnits / 2;
    float scale = (maxDeviation * 2) / totalMeterUnits;
    int meterValue = centerMark + static_cast<int>(currentYDeviation / scale);
    meterValue = std::max(0, std::min(totalMeterUnits, meterValue));


    float opacity = 1.0f;
    Vector2 reqSize = { static_cast<int>(screenWidth * 0.7f), static_cast<int>(screenHeight * 0.04f) };
    Vector2 startPos = { static_cast<int>((screenWidth / 2) - (reqSize.X / 2)), static_cast<int>(screenHeight * 0.1f) };

    CustomColor baseC(255, 255, 255, opacity);
    LineStyle borderS(CustomColor(255, 255, 255, opacity), 2);

    std::list<MeterRange> ranges;
    int greenZone = static_cast<int>(totalMeterUnits * 0.1);
    int yellowZone = static_cast<int>(totalMeterUnits * 0.3);

    ranges.push_back({ CustomColor(50, 255, 50, 0.7f), centerMark - greenZone, centerMark + greenZone });
    ranges.push_back({ CustomColor(255, 255, 50, 0.7f), centerMark - yellowZone, centerMark - greenZone });
    ranges.push_back({ CustomColor(255, 255, 50, 0.7f), centerMark + greenZone, centerMark + yellowZone });
    ranges.push_back({ CustomColor(255, 50, 50, 0.7f), 0, centerMark - yellowZone });
    ranges.push_back({ CustomColor(255, 50, 50, 0.7f), centerMark + yellowZone, totalMeterUnits });


    std::list<MeterMarking> markings;
    markings.push_back({ CustomColor(255,255,255,opacity), 1, centerMark - greenZone });
    markings.push_back({ CustomColor(255,255,255,opacity), 1, centerMark + greenZone });
    markings.push_back({ CustomColor(255,255,255,opacity), 1, centerMark - yellowZone });
    markings.push_back({ CustomColor(255,255,255,opacity), 1, centerMark + yellowZone });
    // markings.push_back({ CustomColor(0,0,0,0.6f), 2, meterValue }); // Current position marker, now handled by RenderMeter's currentValue

    RenderMeter(canvas, startPos, reqSize, baseC, borderS, totalMeterUnits, ranges, markings, false, static_cast<float>(meterValue));

    CVarWrapper speedCvar = _globalCvarManager->getCvar("sv_soccar_gamespeed");
    if (speedCvar) {
        float gameSpeed = speedCvar.getFloatValue();
        std::string speedMsg = fmt::format("Game Speed: {:.0f}%", gameSpeed * 100);
        Vector2F textSizeF = canvas.GetStringSize(speedMsg); // Returns Vector2F
        Vector2 textSize = { static_cast<int>(textSizeF.X), static_cast<int>(textSizeF.Y) };
        canvas.SetColor(255, 255, 255, static_cast<unsigned char>(255 * opacity));
        canvas.SetPosition(Vector2{ startPos.X + reqSize.X - textSize.X - 5, startPos.Y - textSize.Y - 2 });
        canvas.DrawString(speedMsg);
    }

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
    int minTicks = *jumpLow;
    int maxTicks = *jumpHigh;
    int totalMeterUnits = maxTicks - minTicks;
    if (totalMeterUnits <= 0) return;

    int optimalLow = 50 - minTicks;
    int optimalHigh = 60 - minTicks;
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
    ranges.push_back({ CustomColor(50, 255, 50, 0.7f), optimalLow, optimalHigh });
    int yellowBuffer = 5;
    ranges.push_back({ CustomColor(255, 255, 50, 0.7f), std::max(0, optimalLow - yellowBuffer), optimalLow });
    ranges.push_back({ CustomColor(255, 255, 50, 0.7f), optimalHigh, std::min(totalMeterUnits, optimalHigh + yellowBuffer) });
    ranges.push_back({ CustomColor(255, 50, 50, 0.7f), 0, std::max(0, optimalLow - yellowBuffer) });
    ranges.push_back({ CustomColor(255, 50, 50, 0.7f), std::min(totalMeterUnits, optimalHigh + yellowBuffer), totalMeterUnits });

    float currentJumpTickRelative = -1.0f;
    if (attempt.jumped) {
        currentJumpTickRelative = static_cast<float>(attempt.jumpTick - minTicks);
        currentJumpTickRelative = std::max(0.0f, std::min(static_cast<float>(totalMeterUnits), currentJumpTickRelative));
        // Markings list already handles distinct lines, currentValue in RenderMeter handles the main indicator
    }

    RenderMeter(canvas, startPos, reqSize, baseC, borderS, totalMeterUnits, ranges, markings, true, currentJumpTickRelative);

    std::string label = "First Jump";
    Vector2F labelSizeF = canvas.GetStringSize(label);
    Vector2 labelSize = { static_cast<int>(labelSizeF.X), static_cast<int>(labelSizeF.Y) };
    canvas.SetColor(255, 255, 255, static_cast<unsigned char>(255 * opacity));
    canvas.SetPosition(Vector2{ startPos.X + reqSize.X / 2 - labelSize.X / 2, startPos.Y + reqSize.Y + 8 });
    canvas.DrawString(label);

    if (attempt.jumped) {
        std::string msLabel = fmt::format("{}ms", static_cast<int>(attempt.jumpTick / 120.0f * 1000.0f));
        Vector2F msLabelSizeF = canvas.GetStringSize(msLabel);
        Vector2 msLabelSize = { static_cast<int>(msLabelSizeF.X), static_cast<int>(msLabelSizeF.Y) };
        canvas.SetPosition(Vector2{ startPos.X + reqSize.X / 2 - msLabelSize.X / 2, startPos.Y + reqSize.Y + 8 + 15 });
        canvas.DrawString(msLabel);
    }
}

void SpeedFlipTrainer::RenderFlipCancelMeter(CanvasWrapper canvas, float screenWidth, float screenHeight) {
    float opacity = 1.0f;
    int totalMeterUnits = *flipCancelThreshold * 2;
    int idealCancelPoint = *flipCancelThreshold;

    Vector2 reqSize = { static_cast<int>(screenWidth * 0.02f), static_cast<int>(screenHeight * 0.55f) };
    Vector2 startPos = { static_cast<int>(screenWidth * 0.75f), static_cast<int>((screenHeight * 0.8f) - reqSize.Y) };

    CustomColor baseC(255, 255, 255, opacity);
    LineStyle borderS(CustomColor(255, 255, 255, opacity), 2);

    std::list<MeterRange> ranges;
    ranges.push_back({ CustomColor(50, 255, 50, 0.7f), 0, idealCancelPoint });
    ranges.push_back({ CustomColor(255, 255, 50, 0.7f), idealCancelPoint, static_cast<int>(idealCancelPoint * 1.5f) });
    ranges.push_back({ CustomColor(255, 50, 50, 0.7f), static_cast<int>(idealCancelPoint * 1.5f), totalMeterUnits });

    std::list<MeterMarking> markings;
    markings.push_back({ CustomColor(200,200,200,opacity), 1, idealCancelPoint });
    markings.push_back({ CustomColor(200,200,200,opacity), 1, static_cast<int>(idealCancelPoint * 1.5f) });

    float currentTicksAfterDodge = -1.0f;
    if (attempt.flipCanceled) {
        currentTicksAfterDodge = static_cast<float>(attempt.flipCancelTick - attempt.dodgedTick);
        currentTicksAfterDodge = std::max(0.0f, std::min(static_cast<float>(totalMeterUnits), currentTicksAfterDodge));
    }

    RenderMeter(canvas, startPos, reqSize, baseC, borderS, totalMeterUnits, ranges, markings, true, currentTicksAfterDodge);

    std::string label = "Flip Cancel";
    Vector2F labelSizeF = canvas.GetStringSize(label);
    Vector2 labelSize = { static_cast<int>(labelSizeF.X), static_cast<int>(labelSizeF.Y) };
    canvas.SetColor(255, 255, 255, static_cast<unsigned char>(255 * opacity));
    canvas.SetPosition(Vector2{ startPos.X + reqSize.X / 2 - labelSize.X / 2, startPos.Y + reqSize.Y + 8 });
    canvas.DrawString(label);

    if (attempt.flipCanceled) {
        int ticksForCancel = attempt.flipCancelTick - attempt.dodgedTick;
        std::string msLabel = fmt::format("{}ms", static_cast<int>(ticksForCancel / 120.0f * 1000.0f));
        Vector2F msLabelSizeF = canvas.GetStringSize(msLabel);
        Vector2 msLabelSize = { static_cast<int>(msLabelSizeF.X), static_cast<int>(msLabelSizeF.Y) };
        canvas.SetPosition(Vector2{ startPos.X + reqSize.X / 2 - msLabelSize.X / 2, startPos.Y + reqSize.Y + 8 + 15 });
        canvas.DrawString(msLabel);
    }
}

void SpeedFlipTrainer::RenderAngleMeter(CanvasWrapper canvas, float screenWidth, float screenHeight) {
    int totalMeterUnits = 180;
    int centerAngle = 90;

    float opacity = 1.0f;
    Vector2 reqSize = { static_cast<int>(screenWidth * 0.66f), static_cast<int>(screenHeight * 0.04f) };
    Vector2 startPos = { static_cast<int>((screenWidth / 2) - (reqSize.X / 2)), static_cast<int>(screenHeight * 0.9f) };

    CustomColor baseC(255, 255, 255, opacity);
    LineStyle borderS(CustomColor(255, 255, 255, opacity), 2);

    std::list<MeterRange> ranges;
    std::list<MeterMarking> markings;

    int greenRangeWidth = 8;
    int yellowRangeWidth = 15;

    int lTargetMeter = *optimalLeftAngle + centerAngle;
    markings.push_back({ CustomColor(200,200,200,opacity), 1, lTargetMeter - greenRangeWidth });
    markings.push_back({ CustomColor(200,200,200,opacity), 1, lTargetMeter + greenRangeWidth });
    ranges.push_back({ CustomColor(50,255,50,0.7f), lTargetMeter - greenRangeWidth, lTargetMeter + greenRangeWidth });
    ranges.push_back({ CustomColor(255,255,50,0.7f), lTargetMeter - yellowRangeWidth, lTargetMeter - greenRangeWidth });
    ranges.push_back({ CustomColor(255,255,50,0.7f), lTargetMeter + greenRangeWidth, lTargetMeter + yellowRangeWidth });

    int rTargetMeter = *optimalRightAngle + centerAngle;
    markings.push_back({ CustomColor(200,200,200,opacity), 1, rTargetMeter - greenRangeWidth });
    markings.push_back({ CustomColor(200,200,200,opacity), 1, rTargetMeter + greenRangeWidth });
    ranges.push_back({ CustomColor(50,255,50,0.7f), rTargetMeter - greenRangeWidth, rTargetMeter + greenRangeWidth });
    ranges.push_back({ CustomColor(255,255,50,0.7f), rTargetMeter - yellowRangeWidth, rTargetMeter - greenRangeWidth });
    ranges.push_back({ CustomColor(255,255,50,0.7f), rTargetMeter + greenRangeWidth, rTargetMeter + yellowRangeWidth });

    ranges.push_back({ CustomColor(255,50,50,0.7f), 0, std::min(lTargetMeter - yellowRangeWidth, rTargetMeter - yellowRangeWidth) });
    ranges.push_back({ CustomColor(255,50,50,0.7f), std::max(lTargetMeter + yellowRangeWidth, rTargetMeter + yellowRangeWidth), totalMeterUnits });
    if (lTargetMeter + yellowRangeWidth < rTargetMeter - yellowRangeWidth) {
        ranges.push_back({ CustomColor(255,50,50,0.7f), lTargetMeter + yellowRangeWidth, rTargetMeter - yellowRangeWidth });
    }

    float currentAngleMeterValue = -1.0f;
    if (attempt.dodged) {
        currentAngleMeterValue = static_cast<float>(attempt.dodgeAngle + centerAngle);
        currentAngleMeterValue = std::max(0.0f, std::min(static_cast<float>(totalMeterUnits), currentAngleMeterValue));
    }

    RenderMeter(canvas, startPos, reqSize, baseC, borderS, totalMeterUnits, ranges, markings, false, currentAngleMeterValue);

    std::string angleText = "Dodge Angle: " + (attempt.dodged ? std::to_string(attempt.dodgeAngle) : "N/A") + " DEG";
    canvas.SetColor(255, 255, 255, static_cast<unsigned char>(255 * opacity));
    canvas.SetPosition(Vector2{ startPos.X, startPos.Y - 20 });
    canvas.DrawString(angleText);

    if (attempt.hit && attempt.ticksToBall > 0) {
        std::string timeToBallMsg = fmt::format("Time to Ball: {:.3f}s", attempt.timeToBall);
        Vector2F ttbSizeF = canvas.GetStringSize(timeToBallMsg);
        Vector2 ttbSize = { static_cast<int>(ttbSizeF.X), static_cast<int>(ttbSizeF.Y) };
        canvas.SetPosition(Vector2{ startPos.X + reqSize.X / 2 - ttbSize.X / 2, startPos.Y - 20 });
        canvas.DrawString(timeToBallMsg);
    }

    // Ensure Attempt class has 'totalDistanceTraveled' member
    std::string distMsg = fmt::format("Path Length: {:.0f}uu", attempt.totalDistanceTraveled);
    Vector2F distMsgSizeF = canvas.GetStringSize(distMsg);
    Vector2 distMsgSize = { static_cast<int>(distMsgSizeF.X), static_cast<int>(distMsgSizeF.Y) };
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
    // Ensure Attempt class has 'inputs' member (std::vector or similar with .empty())
    if (!gameWrapper || att->inputs.empty() || startingPhysicsFrame < 0) return;
    int tick = gameWrapper->GetEngine().GetPhysicsFrame() - startingPhysicsFrame;
    att->Play(ci, tick);
    gameWrapper->OverrideParams(ci, sizeof(ControllerInput));
}

void SpeedFlipTrainer::PlayBot(ControllerInput* ci) {
    // Ensure BotAttempt class has 'inputs' member
    if (!gameWrapper || bot.inputs.empty() || startingPhysicsFrame < 0) return;
    int tick = gameWrapper->GetEngine().GetPhysicsFrame() - startingPhysicsFrame;
    bot.Play(ci, tick);
    gameWrapper->OverrideParams(ci, sizeof(ControllerInput));
}