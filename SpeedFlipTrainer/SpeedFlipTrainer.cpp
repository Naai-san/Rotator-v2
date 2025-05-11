#include "pch.h"	
#include "SpeedFlipTrainer.h"
#include "RenderMeter.h"
#include <array>
#include "BotAttempt.h"
#include "bakkesmod/wrappers/wrapperstructs.h" // Ensure this includes the definition of Matrix


#define _USE_MATH_DEFINES
#include <math.h>

BAKKESMOD_PLUGIN(SpeedFlipTrainer, "Speedflip trainer", plugin_version, PLUGINTYPE_CUSTOM_TRAINING)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

struct clock_time {
	int hour_hand;
	int min_hand;
};

// SpeedFlipTrainer.cpp - Implémentation de la fonction
/*void SpeedFlipTrainer::RenderCarAxes(CanvasWrapper canvas)
{
	if (!*enabled || !loaded || !*showCarAxes || !gameWrapper->IsInCustomTraining())
		return;

	CarWrapper car = gameWrapper->GetLocalCar();
	if (car.IsNull())
		return;

	// Récupérer la position et la rotation de la voiture
	Vector carLocation = car.GetLocation();
	Rotator carRotation = car.GetRotation();

	// Convertir la rotation en vecteurs directionnels
	auto orientation = RotatorToMatrix(carRotation);
	Vector forward = orientation.forward;
	Vector right = orientation.right;
	Vector up = orientation.up;

	// Couleurs pour les axes principaux (RGB)
	LinearColor redColor = { 1.0f, 0.0f, 0.0f, 1.0f };    // Rouge pour l'axe X (droite)
	LinearColor greenColor = { 0.0f, 1.0f, 0.0f, 1.0f };  // Vert pour l'axe Y (avant)
	LinearColor blueColor = { 0.0f, 0.0f, 1.0f, 1.0f };   // Bleu pour l'axe Z (haut)

	// Couleurs pour les marqueurs diagonaux
	LinearColor orangeColor = { 1.0f, 0.5f, 0.0f, 1.0f }; // Orange pour avant-gauche/arrière-droit
	LinearColor purpleColor = { 0.5f, 0.0f, 1.0f, 1.0f }; // Violet pour avant-droit/arrière-gauche
	LinearColor cyanColor = { 0.0f, 1.0f, 1.0f, 1.0f };   // Cyan pour haut-avant
	LinearColor yellowColor = { 1.0f, 1.0f, 0.0f, 1.0f }; // Jaune pour haut-arrière

	// Dessiner les axes principaux (X, Y, Z)
	// Axe X (droite) - Rouge
	canvas.DrawLine3D(carLocation, carLocation + (right * axisLength), redColor);

	// Axe Y (avant) - Vert
	canvas.DrawLine3D(carLocation, carLocation + (forward * axisLength), greenColor);

	// Axe Z (haut) - Bleu
	canvas.DrawLine3D(carLocation, carLocation + (up * axisLength), blueColor);

	// Point d'origine
	canvas.Draw3DBox(carLocation, Vector{ 5, 5, 5 }, LinearColor{ 1, 1, 1, 1 });

	// Point sur les axes (pour mieux visualiser la direction)
	canvas.Draw3DBox(carLocation + (right * axisLength), Vector{ 5, 5, 5 }, redColor);
	canvas.Draw3DBox(carLocation + (forward * axisLength), Vector{ 5, 5, 5 }, greenColor);
	canvas.Draw3DBox(carLocation + (up * axisLength), Vector{ 5, 5, 5 }, blueColor);

	// Dessiner les marqueurs diagonaux
	float diagonalLength = axisLength * 0.7f;

	// Avant-gauche/arrière-droit (orange)
	Vector frontLeft = forward - right;
	frontLeft.normalize();
	Vector backRight = -frontLeft;
	canvas.DrawLine3D(carLocation, carLocation + (frontLeft * diagonalLength), orangeColor);
	canvas.DrawLine3D(carLocation, carLocation + (backRight * diagonalLength), orangeColor);
	canvas.Draw3DBox(carLocation + (frontLeft * diagonalLength), Vector{ 5, 5, 5 }, orangeColor);
	canvas.Draw3DBox(carLocation + (backRight * diagonalLength), Vector{ 5, 5, 5 }, orangeColor);

	// Avant-droit/arrière-gauche (violet)
	Vector frontRight = forward + right;
	frontRight.normalize();
	Vector backLeft = -frontRight;
	canvas.DrawLine3D(carLocation, carLocation + (frontRight * diagonalLength), purpleColor);
	canvas.DrawLine3D(carLocation, carLocation + (backLeft * diagonalLength), purpleColor);
	canvas.Draw3DBox(carLocation + (frontRight * diagonalLength), Vector{ 5, 5, 5 }, purpleColor);
	canvas.Draw3DBox(carLocation + (backLeft * diagonalLength), Vector{ 5, 5, 5 }, purpleColor);

	// Haut-avant/bas-arrière (cyan)
	Vector topFront = up + forward;
	topFront.normalize();
	Vector bottomBack = -topFront;
	canvas.DrawLine3D(carLocation, carLocation + (topFront * diagonalLength), cyanColor);
	canvas.DrawLine3D(carLocation, carLocation + (bottomBack * diagonalLength), cyanColor);
	canvas.Draw3DBox(carLocation + (topFront * diagonalLength), Vector{ 5, 5, 5 }, cyanColor);
	canvas.Draw3DBox(carLocation + (bottomBack * diagonalLength), Vector{ 5, 5, 5 }, cyanColor);

	// Haut-arrière/bas-avant (jaune)
	Vector topBack = up - forward;
	topBack.normalize();
	Vector bottomFront = -topBack;
	canvas.DrawLine3D(carLocation, carLocation + (topBack * diagonalLength), yellowColor);
	canvas.DrawLine3D(carLocation, carLocation + (bottomFront * diagonalLength), yellowColor);
	canvas.Draw3DBox(carLocation + (topBack * diagonalLength), Vector{ 5, 5, 5 }, yellowColor);
	canvas.Draw3DBox(carLocation + (bottomFront * diagonalLength), Vector{ 5, 5, 5 }, yellowColor);

	// Couleurs des côtés de la voiture
	LinearColor leftSideColor = { 0.0f, 0.7f, 1.0f, 0.5f };  // Bleu clair pour côté gauche
	LinearColor rightSideColor = { 1.0f, 0.7f, 0.0f, 0.5f }; // Orange clair pour côté droit

	// Dessiner des marqueurs sur les côtés de la voiture
	float carWidth = 40.0f; // Approximation de la largeur de la voiture
	float carHeight = 20.0f; // Approximation de la hauteur de la voiture

	// Côté gauche (bleu clair)
	Vector leftSide = carLocation - (right * carWidth / 2) + (up * carHeight / 2);
	canvas.Draw3DBox(leftSide, Vector{ 5, 30, 20 }, leftSideColor);

	// Côté droit (orange clair)
	Vector rightSide = carLocation + (right * carWidth / 2) + (up * carHeight / 2);
	canvas.Draw3DBox(rightSide, Vector{ 5, 30, 20 }, rightSideColor);
}*/

// Fonction d'aide pour convertir une Rotator en vecteurs directionnels
struct Orientation {
	Vector forward;
	Vector right;
	Vector up;
};

// SpeedFlipTrainer.cpp - Ajoutez ces fonctions NEWVER

// Fonction utilitaire pour convertir Rotator en vecteurs d'orientation
SpeedFlipTrainer::Orientation SpeedFlipTrainer::RotatorToMatrix(const Rotator& rotation) {
	Orientation result;

	// Conversion de rotations en radians
	float pitch = rotation.Pitch * M_PI / 32768.0f;
	float yaw = rotation.Yaw * M_PI / 32768.0f;
	float roll = rotation.Roll * M_PI / 32768.0f;

	// Calcul des sinus et cosinus
	float sp = sinf(pitch);
	float cp = cosf(pitch);
	float sy = sinf(yaw);
	float cy = cosf(yaw);
	float sr = sinf(roll);
	float cr = cosf(roll);

	// Calcul des vecteurs directionnels
	// Forward vector (avant)
	result.forward.X = cp * cy;
	result.forward.Y = cp * sy;
	result.forward.Z = sp;

	// Right vector (droite)
	result.right.X = cy * sr * sp + cr * sy;
	result.right.Y = sy * sr * sp - cr * cy;
	result.right.Z = -cp * sr;

	// Up vector (haut)
	result.up.X = -cr * sp * cy + sr * sy;
	result.up.Y = -cr * sp * sy - sr * cy;
	result.up.Z = cp * cr;

	return result;
}

// Définition d'une structure Vector4 pour les calculs de projection
struct Vector4 {
	float X, Y, Z, W;

	Vector4() : X(0), Y(0), Z(0), W(1.0f) {}
	Vector4(float x, float y, float z, float w) : X(x), Y(y), Z(z), W(w) {}
	Vector4(const Vector& v) : X(v.X), Y(v.Y), Z(v.Z), W(1.0f) {}
};


// Fonction pour obtenir la matrice de projection de la caméra
Matrix SpeedFlipTrainer::GetViewProjectionMatrix(CameraWrapper camera) {
	Matrix result = {};

	// Obtenir la position et la rotation de la caméra
	Vector camLocation = camera.GetLocation();
	Rotator camRotation = camera.GetRotation();
	float FOV = camera.GetFOV();

	// Conversion de la rotation en vecteurs de base
	Orientation camOrientation = this->RotatorToMatrix(camRotation);
	Vector forward = camOrientation.forward;
	Vector right = camOrientation.right;
	Vector up = camOrientation.up;

	// Calculer la matrice de vue
	// La matrice de vue transforme les coordonnées du monde en coordonnées relatives à la caméra
	Matrix viewMatrix = {};

	// Première rangée (axe X de la caméra)
	viewMatrix.M[0][0] = right.X;
	viewMatrix.M[0][1] = right.Y;
	viewMatrix.M[0][2] = right.Z;
	viewMatrix.M[0][3] = -Vector::dot(right, camLocation);

	// Deuxième rangée (axe Y de la caméra)
	viewMatrix.M[1][0] = up.X;
	viewMatrix.M[1][1] = up.Y;
	viewMatrix.M[1][2] = up.Z;
	viewMatrix.M[1][3] = -Vector::dot(up, camLocation);

	// Troisième rangée (axe Z de la caméra)
	viewMatrix.M[2][0] = -forward.X; // Inverser l'axe Z pour la convention à droite
	viewMatrix.M[2][1] = -forward.Y;
	viewMatrix.M[2][2] = -forward.Z;
	viewMatrix.M[2][3] = Vector::dot(forward, camLocation);

	// Quatrième rangée (pour les coordonnées homogènes)
	viewMatrix.M[3][0] = 0.0f;
	viewMatrix.M[3][1] = 0.0f;
	viewMatrix.M[3][2] = 0.0f;
	viewMatrix.M[3][3] = 1.0f;

	// Calculer la matrice de projection
	float aspect = 16.0f / 9.0f; // Ratio par défaut si non disponible
	float nearPlane = 10.0f;
	float farPlane = 100000.0f;
	float tanHalfFOV = tanf((FOV * CONST_PI_F / 180.0f) / 2.0f);

	Matrix projMatrix = {};
	projMatrix.M[0][0] = 1.0f / (aspect * tanHalfFOV);
	projMatrix.M[1][1] = 1.0f / tanHalfFOV;
	projMatrix.M[2][2] = farPlane / (farPlane - nearPlane);
	projMatrix.M[2][3] = 1.0f;
	projMatrix.M[3][2] = -(farPlane * nearPlane) / (farPlane - nearPlane);
	projMatrix.M[3][3] = 0.0f;

	// Multiplier les matrices pour obtenir la matrice de projection finale
	// Note: Cette multiplication simplifiée ne gère pas les cas complexes
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result.M[i][j] = 0;
			for (int k = 0; k < 4; k++) {
				result.M[i][j] += viewMatrix.M[i][k] * projMatrix.M[k][j];
			}
		}
	}

	return result;
}

// Fonction pour convertir un point 3D en 2D pour l'affichage
Vector2 SpeedFlipTrainer::WorldToScreen(CanvasWrapper canvas, Vector location) {
	auto screenSize = canvas.GetSize();
	if (auto currentCamera = gameWrapper->GetCamera()) {
		// Obtenir la matrice de projection à partir de la caméra
		Matrix viewProjMatrix = GetViewProjectionMatrix(currentCamera);

		// Transformer la position 3D en espace de coordonnées homogènes
		Vector4 locationVec4(location);
		Vector4 clipSpaceLocation = viewProjMatrix * locationVec4;

		// Effectuer la division perspective pour obtenir les coordonnées normalisées
		if (clipSpaceLocation.W != 0.0f) {
			clipSpaceLocation.X /= clipSpaceLocation.W;
			clipSpaceLocation.Y /= clipSpaceLocation.W;
			clipSpaceLocation.Z /= clipSpaceLocation.W;
		}

		// Convertir les coordonnées normalisées en coordonnées d'écran
		int screenX = static_cast<int>((clipSpaceLocation.X * 0.5f + 0.5f) * screenSize.X);
		int screenY = static_cast<int>((-clipSpaceLocation.Y * 0.5f + 0.5f) * screenSize.Y);

		return Vector2{ screenX, screenY };
	}
	return Vector2{ 0, 0 }; // Fallback si la caméra n'est pas disponible
}



// Fonction pour vérifier si un point est visible à l'écran
bool SpeedFlipTrainer::IsPointOnScreen(const Vector2& point, float screenWidth, float screenHeight) {
	return point.X >= 0 && point.X <= screenWidth &&
		point.Y >= 0 && point.Y <= screenHeight;
}

// Fonction pour dessiner une ligne avec une flèche à la fin
void SpeedFlipTrainer::DrawArrow(CanvasWrapper& canvas, Vector2 start, Vector2 end, const struct Color& color) {
	// Dessiner la ligne principale
	canvas.SetColor(color.red, color.green, color.blue, (char)(255 * color.opacity));
	canvas.DrawLine(start, end, 2);

	// Calculer la direction et la normaliser
	float dx = end.X - start.X;
	float dy = end.Y - start.Y;
	float length = sqrtf(dx * dx + dy * dy);

	if (length < 1.0f) return; // Éviter la division par zéro

	dx /= length;
	dy /= length;

	// Créer les points de la flèche
	float arrowSize = 10.0f;
	float arrowAngle = 0.5f; // environ 30 degrés

	Vector2 arrow1;
	arrow1.X = end.X - arrowSize * (dx * cosf(arrowAngle) + dy * sinf(arrowAngle));
	arrow1.Y = end.Y - arrowSize * (dy * cosf(arrowAngle) - dx * sinf(arrowAngle));

	Vector2 arrow2;
	arrow2.X = end.X - arrowSize * (dx * cosf(arrowAngle) - dy * sinf(arrowAngle));
	arrow2.Y = end.Y - arrowSize * (dy * cosf(arrowAngle) + dx * sinf(arrowAngle));

	// Dessiner les lignes de la flèche
	canvas.DrawLine(end, arrow1, 2);
	canvas.DrawLine(end, arrow2, 2);
}

// Implémentation principale du rendu des axes
void SpeedFlipTrainer::RenderCarAxes(CanvasWrapper canvas) {
	if (!*enabled || !loaded || !*showCarAxes || !gameWrapper->IsInCustomTraining())
		return;

	CarWrapper car = gameWrapper->GetLocalCar();
	if (car.IsNull())
		return;

	// Récupérer la position et la rotation de la voiture
	Vector carLocation = car.GetLocation();
	Rotator carRotation = car.GetRotation();

	// Convertir la rotation en vecteurs directionnels
	auto orientation = RotatorToMatrix(carRotation);
	Vector forward = orientation.forward;
	Vector right = orientation.right;
	Vector up = orientation.up;

	// Calculer les points finaux des axes
	Vector forwardEnd = carLocation + forward * (*axisLength);
	Vector rightEnd = carLocation + right * (*axisLength);
	Vector upEnd = carLocation + up * (*axisLength);

	// Couleurs pour les axes
	struct Color redColor = { (char)255, (char)0, (char)0, 1.0f };
	struct Color greenColor = { (char)0, (char)255, (char)0, 1.0f };
	struct Color blueColor = { (char)0, (char)0, (char)255, 1.0f };
	struct Color orangeColor = { (char)255, (char)165, (char)0, 1.0f };
	struct Color purpleColor = { (char)128, (char)0, (char)128, 1.0f };
	struct Color cyanColor = { (char)0, (char)255, (char)255, 1.0f };
	struct Color yellowColor = { (char)255, (char)255, (char)0, 1.0f };
	struct Color whiteColor = { (char)255, (char)255, (char)255, 1.0f };

	// Taille de l'écran
	float screenWidth = canvas.GetSize().X;
	float screenHeight = canvas.GetSize().Y;

	// Convertir les positions 3D en positions 2D à l'écran
	Vector2 carPos2D = WorldToScreen(canvas, carLocation);
	Vector2 forwardPos2D = WorldToScreen(canvas, forwardEnd);
	Vector2 rightPos2D = WorldToScreen(canvas, rightEnd);
	Vector2 upPos2D = WorldToScreen(canvas, upEnd);

	// Vérifier si tous les points sont visibles sur l'écran
	bool carVisible = IsPointOnScreen(carPos2D, screenWidth, screenHeight);
	bool forwardVisible = IsPointOnScreen(forwardPos2D, screenWidth, screenHeight);
	bool rightVisible = IsPointOnScreen(rightPos2D, screenWidth, screenHeight);
	bool upVisible = IsPointOnScreen(upPos2D, screenWidth, screenHeight);

	// Dessiner les axes principaux s'ils sont visibles
	if (carVisible) {
		// Dessiner un point central à la position de la voiture
		canvas.SetColor(whiteColor.red, whiteColor.green, whiteColor.blue, (char)(255 * whiteColor.opacity));
		canvas.SetPosition(carPos2D);
		canvas.FillBox(Vector2{ 5, 5 });

		// Dessiner l'axe avant (Y) - Vert
		if (forwardVisible) {
			DrawArrow(canvas, carPos2D, forwardPos2D, greenColor);
		}

		// Dessiner l'axe droit (X) - Rouge
		if (rightVisible) {
			DrawArrow(canvas, carPos2D, rightPos2D, redColor);
		}

		// Dessiner l'axe haut (Z) - Bleu
		if (upVisible) {
			DrawArrow(canvas, carPos2D, upPos2D, blueColor);
		}

		// Dessiner les axes diagonaux
		float diagonalLength = (*axisLength) * 0.6f;

		// Avant-gauche/arrière-droit (orange)
		Vector frontLeft = forward - right;
		frontLeft.normalize();
		Vector frontLeftEnd = carLocation + frontLeft * diagonalLength;
		Vector2 frontLeftPos2D = WorldToScreen(canvas, frontLeftEnd);

		if (IsPointOnScreen(frontLeftPos2D, screenWidth, screenHeight)) {
			DrawArrow(canvas, carPos2D, frontLeftPos2D, orangeColor);
		}

		// Avant-droit/arrière-gauche (violet)
		Vector frontRight = forward + right;
		frontRight.normalize();
		Vector frontRightEnd = carLocation + frontRight * diagonalLength;
		Vector2 frontRightPos2D = WorldToScreen(canvas, frontRightEnd);

		if (IsPointOnScreen(frontRightPos2D, screenWidth, screenHeight)) {
			DrawArrow(canvas, carPos2D, frontRightPos2D, purpleColor);
		}

		// Marqueurs des côtés gauche/droit
		Vector leftSide = carLocation - right * 40.0f;
		Vector rightSide = carLocation + right * 40.0f;
		Vector2 leftSidePos2D = WorldToScreen(canvas, leftSide);
		Vector2 rightSidePos2D = WorldToScreen(canvas, rightSide);

		// Côté gauche (cyan)
		if (IsPointOnScreen(leftSidePos2D, screenWidth, screenHeight)) {
			canvas.SetColor(cyanColor.red, cyanColor.green, cyanColor.blue, (char)(255 * 0.7f));
			canvas.SetPosition(leftSidePos2D);
			canvas.FillBox(Vector2{ 10, 10 });
		}

		// Côté droit (jaune)
		if (IsPointOnScreen(rightSidePos2D, screenWidth, screenHeight)) {
			canvas.SetColor(yellowColor.red, yellowColor.green, yellowColor.blue, (char)(255 * 0.7f));
			canvas.SetPosition(rightSidePos2D);
			canvas.FillBox(Vector2{ 10, 10 });
		}
	}

	// Ajouter des informations textuelles pour aider à l'orientation
	canvas.SetColor(255, 255, 255, 255);
	canvas.SetPosition(Vector2{ 10, static_cast<int>(screenHeight - 60) });
	canvas.DrawString("Rouge (X): Droite | Vert (Y): Avant | Bleu (Z): Haut", 1.0f, 1.0f);
	canvas.SetPosition(Vector2{ 10, static_cast<int>(screenHeight - 40) });
	canvas.DrawString("Orange: Avant-Gauche | Violet: Avant-Droit", 1.0f, 1.0f);
	canvas.SetPosition(Vector2{ 10, static_cast<int>(screenHeight - 20) });
	canvas.DrawString("Cyan: Côté Gauche | Jaune: Côté Droit", 1.0f, 1.0f);
}

// Modifier la fonction RenderMeters pour appeler la nouvelle fonction
void SpeedFlipTrainer::RenderMeters(CanvasWrapper canvas)
{
	bool training = gameWrapper->IsInCustomTraining();

	if (!*enabled || !loaded || !training) return;

	float SCREENWIDTH = canvas.GetSize().X;
	float SCREENHEIGHT = canvas.GetSize().Y;

	if (*showAngleMeter)
		RenderAngleMeter(canvas, SCREENWIDTH, SCREENHEIGHT);

	if (*showPositionMeter)
		RenderPositionMeter(canvas, SCREENWIDTH, SCREENHEIGHT);

	if (*showFlipMeter)
		RenderFlipCancelMeter(canvas, SCREENWIDTH, SCREENHEIGHT);

	if (*showJumpMeter)
		RenderFirstJumpMeter(canvas, SCREENWIDTH, SCREENHEIGHT);

	// Ajout de l'appel à la fonction de rendu des axes
	if (*showCarAxes)
		RenderCarAxes(canvas);
}


int ComputeDodgeAngle(DodgeComponentWrapper dodge)
{
	if (dodge.IsNull())
		return 0;

	Vector dd = dodge.GetDodgeDirection();
	if (dd.X == 0 && dd.Y == 0)
		return 0;

	//LOG("X: {0} Y: {1}", dd.X, dd.Y);
	return (int)(atan2f(dd.Y, dd.X) * (180 / M_PI));
}

clock_time ComputeClockTime(int angle)
{
	if (angle < 0)
	{
		angle = 360 + angle;
	}
	clock_time time;

	time.hour_hand = (int)(angle * (12.0 / 360.0));
	time.min_hand = (angle % (360 / 12)) * (60 / 30);

	return time;
}

float distance(Vector a, Vector b)
{
	// Calculating distance
	return sqrt(pow(a.X - b.X, 2) +
		pow(a.Y - b.Y, 2));
}

void SpeedFlipTrainer::Measure(CarWrapper car, std::shared_ptr<GameWrapper> gameWrapper)
{
	int currentPhysicsFrame = gameWrapper->GetEngine().GetPhysicsFrame();
	int currentTick = currentPhysicsFrame - startingPhysicsFrame;

	ControllerInput input = car.GetInput();
	attempt.Record(currentTick, input);

	auto loc = car.GetLocation();
	attempt.traveledY += abs(loc.Y - attempt.positionY);
	attempt.positionY = loc.Y;

	if (!attempt.jumped && car.GetbJumped())
	{
		attempt.jumped = true;
		attempt.jumpTick = currentTick;
		LOG("First jump: " + to_string(currentTick) + " ticks");
	}

	if (!attempt.dodged && car.IsDodging())
	{
		attempt.dodged = true;
		attempt.dodgedTick = currentTick;
		auto dodge = car.GetDodgeComponent();
		if (!dodge.IsNull())
		{
			attempt.dodgeAngle = ComputeDodgeAngle(dodge);
			clock_time time = ComputeClockTime(attempt.dodgeAngle);
			string msg = fmt::format("Dodge Angle: {0:#03d} deg or {1:#02d}:{2:#02d} PM", attempt.dodgeAngle, time.hour_hand, time.min_hand);
			LOG(msg);
		}
	}

	if (input.Throttle != 1)
		attempt.ticksNotPressingThrottle++;
	if (input.ActivateBoost != 1)
		attempt.ticksNotPressingBoost++;

	if (!attempt.flipCanceled  && attempt.dodged && input.Pitch > 0.8)
	{
		attempt.flipCanceled = true;
		attempt.flipCancelTick = currentTick;
		LOG("Flip Cancel: " + to_string(attempt.flipCancelTick - attempt.dodgedTick) + " ticks");
	}
}

void SpeedFlipTrainer::Hook()
{
	if (loaded)
		return;
	loaded = true;

	LOG("Hooking");
	gameWrapper->RegisterDrawable(bind(&SpeedFlipTrainer::RenderMeters, this, std::placeholders::_1));

	if (*rememberSpeed)
		_globalCvarManager->getCvar("sv_soccar_gamespeed").setValue(*speed);

	gameWrapper->HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.SetVehicleInput",
		[this](CarWrapper car, void* params, std::string eventname) {
		if (!*enabled || !loaded || !gameWrapper->IsInCustomTraining())
			return;

		if (car.IsNull())
			return;

		if (*rememberSpeed)
			*(this->speed) = _globalCvarManager->getCvar("sv_soccar_gamespeed").getFloatValue();

		auto input = (ControllerInput*)params;

		if(mode == SpeedFlipTrainerMode::Bot)
			PlayBot(gameWrapper, input);
		else if (mode == SpeedFlipTrainerMode::Replay)
			PlayAttempt(&replayAttempt, gameWrapper, input);

		// Has time started counting down?
		float timeLeft = gameWrapper->GetCurrentGameState().GetGameTimeRemaining();
		int currentFrame = gameWrapper->GetEngine().GetPhysicsFrame();
		if (initialTime <= 0 || timeLeft == initialTime) // if we have no time OR countdown hasn't started just return
			return;
		if (startingPhysicsFrame < 0 && timeLeft < initialTime) // if this is the first frame the countdown has begun
		{
			startingPhysicsFrame = currentFrame;

			attempt = Attempt();

			if (!car.IsOnGround())
				attempt.startedInAir = true;

			if (!input->ActivateBoost)
				attempt.startedNoBoost = true;
		}

		// ball hasn't exploded or been hit yet
		if (!attempt.exploded && !attempt.hit)
		{
			Measure(car, gameWrapper);
		}
	});

	gameWrapper->HookEvent("Function TAGame.Ball_TA.RecordCarHit",
		[this](std::string eventname) {
		if (!*enabled || !loaded || !gameWrapper->IsInCustomTraining() || attempt.hit || attempt.exploded)
			return;

		attempt.ticksToBall = gameWrapper->GetLocalCar().GetLastBallImpactFrame() - startingPhysicsFrame;
		attempt.timeToBall = initialTime - gameWrapper->GetCurrentGameState().GetGameTimeRemaining();
		attempt.hit = true;
		LOG("Time to ball: {0:.3f}s after {1} tick", attempt.timeToBall, attempt.ticksToBall);
	});

	gameWrapper->HookEvent("Function TAGame.Ball_TA.Explode", 
		[this](std::string eventName) {
		if (!*enabled || !loaded || !gameWrapper->IsInCustomTraining())
			return;

		auto ball = gameWrapper->GetGameEventAsServer().GetBall();
		auto car = gameWrapper->GetGameEventAsServer().GetLocalPrimaryPlayer();

		auto distanceToBall = distance(ball.GetLocation(), car.GetLocation());
		auto meters = (distanceToBall / 100.0) - 4.8; // account for car distance
		LOG("Distance to ball = {0:.1f}m", meters);

		attempt.exploded = true;
		attempt.hit = false;
	});

	gameWrapper->HookEventPost("Function Engine.Controller.Restart", 
		[this](std::string eventName) {
		if (!*enabled || !loaded || !gameWrapper->IsInCustomTraining())
			return;

		//gameWrapper->GetCurrentGameState().SetGameTimeRemaining(2.14);
		initialTime = gameWrapper->GetCurrentGameState().GetGameTimeRemaining();
		ticksBeforeTimeExpired = initialTime * 120;
		startingPhysicsFrame = -1;

		if (attempt.hit && !attempt.exploded)
		{
			consecutiveHits++;
			consecutiveMiss = 0;
		}
		else
		{
			consecutiveHits = 0;
			consecutiveMiss++;
		}

		if (*saveToFile && attempt.inputs.size() > 0)
		{
			auto path = attempt.GetFilename(dataDir);
			attempt.WriteInputsToFile(path);
			LOG("Saving attempt to: {0}", path.string());
		}

		auto speedCvar = _globalCvarManager->getCvar("sv_soccar_gamespeed");
		float speed = speedCvar.getFloatValue();

		if (*changeSpeed)
		{
			if (consecutiveHits != 0 && consecutiveHits % (*numHitsChangedSpeed) == 0)
			{
				gameWrapper->LogToChatbox(to_string(consecutiveHits) + " " + (consecutiveHits > 1 ? "hits" : "hit") + " in a row");
				speed += *speedIncrement;
				speedCvar.setValue(speed);
				string msg = fmt::format("Game speed + {0:.3f} = {1:.3f}", *speedIncrement, speed);
				gameWrapper->LogToChatbox(msg);
			}
			else if (consecutiveMiss != 0 && consecutiveMiss % (*numHitsChangedSpeed) == 0)
			{
				gameWrapper->LogToChatbox(to_string(consecutiveMiss) + " " + (consecutiveMiss > 1 ? "misses" : "miss") + " in a row");
				speed -= *speedIncrement;
				if (speed <= 0)
					speed = 0;
				speedCvar.setValue(speed);
				string msg = fmt::format("Game speed - {0:.3f} = {1:.3f}", *speedIncrement, speed);
				gameWrapper->LogToChatbox(msg);
			}
		}
	});
}

void SpeedFlipTrainer::onLoad()
{
	_globalCvarManager = cvarManager;

	// DEBUT 
	cvarManager->registerCvar("sf_show_axes", "1", "Show car orientation axes.", true, false, 0, false, 0, true).bindTo(showCarAxes);
	cvarManager->registerCvar("sf_axis_length", "150.0", "Length of car orientation axes.", true, true, 50.0f, true, 300.0f, true).bindTo(axisLength);
	// FIN

	cvarManager->registerCvar("sf_enabled", "1", "Enabled speedflip training.", true, false, 0, false, 0, true).bindTo(enabled);
	cvarManager->getCvar("sf_enabled").addOnValueChanged([this](string oldVal, CVarWrapper cvar)
	{
		LOG("sf_enabled = {0}", *enabled);
		if (!*enabled)
		{
			onUnload();
		}
	});

	cvarManager->registerCvar("sf_save_attempts", "0", "Save attmempts to a file.", true, false, 0, false, 0, true).bindTo(saveToFile);

	cvarManager->registerCvar("sf_change_speed", "0", "Change game speed on consecutive hits and misses.", true, false, 0, false, 0, true).bindTo(changeSpeed);
	cvarManager->registerCvar("sf_speed", "1.0", "Change game speed on consecutive hits and misses.", true, false, 0.0, false, 1.0, true).bindTo(speed);
	cvarManager->registerCvar("sf_remember_speed", "1", "Remember last set speed.", true, true, 1, true, 1, true).bindTo(rememberSpeed);
	cvarManager->registerCvar("sf_num_hits", "3", "Number of hits/misses before the speed increases/decreases.", true, true, 0, true, 50, true).bindTo(numHitsChangedSpeed);
	cvarManager->registerCvar("sf_speed_increment", "0.05", "Speed increase/decrease increment.", true, true, 0.001, true, 0.999, true).bindTo(speedIncrement);

	cvarManager->registerCvar("sf_left_angle", "-30", "Optimal left angle", true, true, -78, true, -12, true).bindTo(optimalLeftAngle);
	cvarManager->registerCvar("sf_right_angle", "30", "Optimal right angle", true, true, 12, true, 78, true).bindTo(optimalRightAngle);
	cvarManager->registerCvar("sf_cancel_threshold", "10", "Optimal flip cancel threshold.", true, true, 1, true, 15, true).bindTo(flipCancelThreshold);

	cvarManager->registerCvar("sf_show_angle", "1", "Show angle meter.", true, false, 0, false, 0, true).bindTo(showAngleMeter);
	cvarManager->registerCvar("sf_show_position", "1", "Show horizontal position meter.", true, false, 0, false, 0, true).bindTo(showPositionMeter);
	cvarManager->registerCvar("sf_show_jump", "1", "Show jump meter.", true, false, 0, false, 0, true).bindTo(showJumpMeter);
	cvarManager->registerCvar("sf_show_flip", "1", "Show flip cancel meter.", true, false, 0, false, 0, true).bindTo(showFlipMeter);

	//cvarManager->registerCvar("sf_jump_low", "40", "Low threshold for first jump of speedflip.", true, true, 10, true, 110, false).bindTo(jumpLow);
	//cvarManager->registerCvar("sf_jump_high", "90", "High threshold for first jump of speedflip.", true, true, 20, true, 120, false).bindTo(jumpHigh);

	if (*enabled)
	{
		gameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.GameEvent_TrainingEditor_TA.LoadRound", [this](ActorWrapper cw, void* params, std::string eventName) {
			if (!*enabled || !IsMustysPack((TrainingEditorWrapper)cw.memory_address))
				return;
			Hook();
		});

		gameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.GameEvent_TrainingEditor_TA.Destroyed", [this](ActorWrapper cw, void* params, std::string eventName) {
			if(loaded)
				onUnload();
		});

		dataDir = gameWrapper->GetDataFolder().append("speedflip");
		if (!std::filesystem::exists(dataDir))
			std::filesystem::create_directories(dataDir);

		attemptFileDialog.workingDirectory = dataDir / "attempts";
		if (!std::filesystem::exists(attemptFileDialog.workingDirectory))
			std::filesystem::create_directories(attemptFileDialog.workingDirectory);
		attemptFileDialog.name = "Select replay attempt";

		botFileDialog.workingDirectory = dataDir / "bots";
		if (!std::filesystem::exists(botFileDialog.workingDirectory))
			std::filesystem::create_directories(botFileDialog.workingDirectory);
		botFileDialog.name = "Select bot file";
	}

}

void SpeedFlipTrainer::onUnload()
{
	if (!loaded)
		return;
	loaded = false;

	LOG("Unhooking");
	gameWrapper->UnhookEvent("Function TAGame.Car_TA.SetVehicleInput");
	gameWrapper->UnhookEvent("Function TAGame.Ball_TA.RecordCarHit");
	gameWrapper->UnhookEvent("Function TAGame.Ball_TA.Explode");
	gameWrapper->UnhookEventPost("Function Engine.Controller.Restart"); 
	gameWrapper->UnregisterDrawables();
}

bool SpeedFlipTrainer::IsMustysPack(TrainingEditorWrapper tw)
{
	if (!tw.IsNull())
	{
		GameEditorSaveDataWrapper data = tw.GetTrainingData();
		if (!data.IsNull())
		{
			TrainingEditorSaveDataWrapper td = data.GetTrainingData();
			if (!td.IsNull())
			{
				if (!td.GetCode().IsNull())
				{
					auto code = td.GetCode().ToString();
					if (code == "A503-264C-A7EB-D282")
					{
						return true;
					}
				}
			}
		}
	}
	return false;
}

void SpeedFlipTrainer::RenderPositionMeter(CanvasWrapper canvas, float screenWidth, float screenHeight)
{
	float mid = -1.1;
	int range = 200;
	int relLocation = (-1 * attempt.positionY) + range;
	int totalUnits = range * 2;

	float opacity = 1.0;
	Vector2 reqSize = Vector2{ (int)(screenWidth * 70 / 100.f), (int)(screenHeight * 4 / 100.f) };
	int unitWidth = reqSize.X / totalUnits;

	Vector2 boxSize = Vector2{ unitWidth * totalUnits, reqSize.Y };
	Vector2 startPos = Vector2{ (int)((screenWidth/2) - (boxSize.X/2)), (int)(screenHeight * 10 / 100.f) };

	struct Color baseColor = { (char)255, (char)255, (char)255, opacity };
	struct Line border = { (char)255, (char)255, (char)255, opacity, 2 };

	std::list<MeterRange> ranges;
	if (startingPhysicsFrame > 0)
	{
		float go = 1, ro = 1, yo = 1;
		if (relLocation >= range - 80 && relLocation <= range + 80)
		{
			ranges.push_back({ (char)50, (char)255, (char)50, go, range - 80, range + 80 });
		}
		else if (relLocation >= range - 160 && relLocation <= range + 160)
		{
			ranges.push_back({ (char)255, (char)255, (char)50, yo,  range - 160, range + 160 });
			ranges.push_back({ (char)255, (char)255, (char)50, yo,  range - 160, range + 160 });
		}
		else
		{
			ranges.push_back({ (char)255,(char)50, (char)50, ro, 0, totalUnits });
		}
	}

	std::list<MeterMarking> markings;
	markings.push_back({ (char)255, (char)255, (char)255, opacity, unitWidth, range - 80 });
	markings.push_back({ (char)255, (char)255, (char)255, opacity, unitWidth, range + 80 });
	markings.push_back({ (char)255, (char)255, (char)255, opacity, unitWidth, range - 160 });
	markings.push_back({ (char)255, (char)255, (char)255, opacity, unitWidth, range + 160 });
	markings.push_back({ (char)0, (char)0, (char)0, 0.6, unitWidth*2, relLocation });

	RenderMeter(canvas, startPos, reqSize, baseColor, border, totalUnits, ranges, markings, false);

	//draw speed label	
	auto speedCvar = _globalCvarManager->getCvar("sv_soccar_gamespeed");
	float speed = speedCvar.getFloatValue();
	string msg = fmt::format("Game speed: {0}%", (int)(speed * 100));
	int width = (msg.length() * 8.5) - 10;
	canvas.SetColor(255, 255, 255, (char)(255 * opacity));
	canvas.SetPosition(Vector2{ startPos.X + boxSize.X - width, (int)(startPos.Y - 20) });
	canvas.DrawString(msg, 1, 1, true, false);

	int ms = (int)(attempt.ticksNotPressingBoost / 120.0 * 1000.0);
	if (ms != 0)
	{
		canvas.SetColor(255, 255, 50, (char)(255 * opacity));
		//draw time not pressing boost label
		msg = fmt::format("Not pressing Boost: {0}ms", ms);
		width = 200;
		canvas.SetPosition(Vector2{ startPos.X, (int)(startPos.Y + boxSize.Y + 10) });
		canvas.DrawString(msg, 1, 1, true, false);
	}

	ms = (int)(attempt.ticksNotPressingThrottle / 120.0 * 1000.0);
	if (ms != 0)
	{
		canvas.SetColor(255, 255, 50, (char)(255 * opacity));
		//draw time not pressing throttle label
		msg = fmt::format("Not pressing Throttle: {0}ms", ms);
		width = 200;
		canvas.SetPosition(Vector2{ startPos.X, (int)(startPos.Y + boxSize.Y + 25) });
		canvas.DrawString(msg, 1, 1, true, false);
	}
}

void SpeedFlipTrainer::RenderFirstJumpMeter(CanvasWrapper canvas, float screenWidth, float screenHeight)
{
	int totalUnits = *jumpHigh - *jumpLow;
	int halfMark = totalUnits / 2;
	
	float opacity = 1.0;
	Vector2 reqSize = Vector2{ (int)(screenWidth * 2 / 100.f), (int)(screenHeight * 56 / 100.f) };
	int unitWidth = reqSize.Y / totalUnits;

	Vector2 boxSize = Vector2{ reqSize.X, unitWidth * totalUnits };
	Vector2 startPos = Vector2{ (int)((screenWidth * 75 / 100.f) + 2.5 * reqSize.X), (int)((screenHeight * 80 / 100.f) - boxSize.Y) };
	
	struct Color baseColor = { (char)255, (char)255, (char)255, opacity };
	struct Line border = { (char)255, (char)255, (char)255, opacity, 2 };

	
	std::list<MeterMarking> markings;
	markings.push_back({ (char)255, (char)255, (char)255, opacity, 3, (int)(halfMark - 15) });
	markings.push_back({ (char)255, (char)255, (char)255, opacity, 3, (int)(halfMark - 10) });
	markings.push_back({ (char)255, (char)255, (char)255, opacity, 3, (int)(halfMark + 5) });
	markings.push_back({ (char)255, (char)255, (char)255, opacity, 3, (int)(halfMark + 10) });

	std::list<MeterRange> ranges;
	ranges.push_back({ (char)255, (char)255, (char)50, 0.2, (int)(halfMark - 15), (int)(halfMark - 10) });
	ranges.push_back({ (char)50,(char)255, (char)50, 0.2, (int)(halfMark - 10), (int)(halfMark + 5) });
	ranges.push_back({ (char)255,(char)255, (char)50, 0.2, (int)(halfMark + 5), (int)(halfMark + 10) });

	if (attempt.jumped)
	{
		int ticks = attempt.jumpTick - ((*jumpHigh - (halfMark)) - halfMark);
		if (ticks < 0)
		{
			ticks = 0;
		}
		else if (ticks > totalUnits)
		{
			ticks = totalUnits;
		}

		if (ticks < halfMark - 15)
		{
			ranges.push_back({ (char)255,(char)50, (char)50, 1, 0, halfMark - 15 });
		}
		else if (ticks < halfMark - 10)
		{
			ranges.push_back({ (char)255, (char)255, (char)50, 1, (int)(halfMark - 15), (int)(halfMark - 10) });
		}
		else if (ticks < halfMark + 5)
		{
			ranges.push_back({ (char)50,(char)255, (char)50, 1, (int)(halfMark - 10), (int)(halfMark + 5) });
		}
		else if (ticks < halfMark + 10)
		{
			ranges.push_back({ (char)255,(char)255, (char)50, 1, (int)(halfMark + 5), (int)(halfMark + 10) });
		}
		else
		{
			ranges.push_back({ (char)255,(char)50, (char)50, 1, (int)(halfMark + 10), totalUnits });
		}
		markings.push_back({ (char)0, (char)0, (char)0, 0.6, (int)reqSize.Y / totalUnits, (int)ticks });
	}

	RenderMeter(canvas, startPos, reqSize, baseColor, border, totalUnits, ranges, markings, true);

	//draw label
	string msg = "First Jump";
	canvas.SetColor(255, 255, 255, (char)(255 * opacity));
	canvas.SetPosition(Vector2{ (int)(startPos.X - 13), (int)(startPos.Y + boxSize.Y + 8) });
	canvas.DrawString(msg, 1, 1, true, false);

	auto ms = (int)(attempt.jumpTick * 1.0 / 120.0 * 1000.0 / 1.0);
	msg = to_string(ms) + " ms";
	canvas.SetPosition(Vector2{ startPos.X, (int)(startPos.Y + boxSize.Y + 8 + 15) });
	canvas.DrawString(msg, 1, 1, true, false);
}

void SpeedFlipTrainer::RenderFlipCancelMeter(CanvasWrapper canvas, float screenWidth, float screenHeight)
{
	float opacity = 1.0;
	int totalUnits = *flipCancelThreshold;

	Vector2 reqSize = Vector2{ (int)(screenWidth * 2 / 100.f), (int)(screenHeight * 55 / 100.f) };
	int unitWidth = reqSize.Y / totalUnits;

	Vector2 boxSize = Vector2{ reqSize.X, unitWidth * totalUnits };
	Vector2 startPos = Vector2{ (int)(screenWidth * 75 / 100.f), (int)((screenHeight * 80 / 100.f) - boxSize.Y) };

	struct Color baseColor = { (char)255, (char)255, (char)255, opacity };
	struct Line border = { (char)255, (char)255, (char)255, opacity, 2 };

	auto tickBeforeCancel = attempt.flipCancelTick - attempt.dodgedTick;

	// flip cancel time range
	std::list<MeterRange> ranges;
	if (attempt.flipCanceled)
	{
		auto ticks = tickBeforeCancel > totalUnits ? totalUnits : tickBeforeCancel;
		struct Color meterColor;
		if (ticks <= (int)(totalUnits * 0.6f))
			meterColor = { (char)50, (char)255, (char)50, 0.7 }; // green
		else if (ticks <= (int)(totalUnits * 0.9f))
			meterColor = { (char)255, (char)255, (char)50, 0.7 }; // yellow
		else
			meterColor = { (char)255, (char)50, (char)50, 0.7 }; // red
		ranges.push_back({ meterColor.red, meterColor.green, meterColor.blue, 1, 0, ticks });
	}

	std::list<MeterMarking> markings;
	markings.push_back({ (char)255, (char)255, (char)255, opacity, 3, ((int)(totalUnits * 0.6f)) });
	markings.push_back({ (char)255, (char)255, (char)255, opacity, 3, ((int)(totalUnits * 0.9f)) });
	//markings.push_back({ (char)0, (char)0, (char)0, 0.6, 10, ticks });

	RenderMeter(canvas, startPos, reqSize, baseColor, border, totalUnits, ranges, markings, true);

	//draw label
	string msg = "Flip Cancel";
	canvas.SetColor(255, 255, 255, (char)(255 * opacity));
	canvas.SetPosition(Vector2{ (int)(startPos.X - 16), (int)(startPos.Y + boxSize.Y + 8) });
	canvas.DrawString(msg, 1, 1, true, false);

	auto ms = (int)(tickBeforeCancel * 1.0 / 120.0 * 1000.0 / 1.0);
	msg = to_string(ms) + " ms";
	canvas.SetPosition(Vector2{ startPos.X, (int)(startPos.Y + boxSize.Y + 8 + 15) });
	canvas.DrawString(msg, 1, 1, true, false);
}

void SpeedFlipTrainer::RenderAngleMeter(CanvasWrapper canvas, float screenWidth, float screenHeight)
{
	// Cap angle at 90
	int totalUnits = 180;
	
	float opacity = 1.0;
	Vector2 reqSize = Vector2{ (int)(screenWidth * 66 / 100.f), (int)(screenHeight * 4 / 100.f) };
	int unitWidth = reqSize.X / totalUnits;

	Vector2 boxSize = Vector2{ unitWidth * totalUnits, reqSize.Y };
	Vector2 startPos = Vector2{ (int)((screenWidth / 2) - (boxSize.X / 2)), (int)(screenHeight * 90 / 100.f) };

	struct Color baseColor = { (char)255, (char)255, (char)255, opacity };
	struct Line border = { (char)255, (char)255, (char)255, opacity, 2 };

	std::list<MeterRange> ranges;
	std::list<MeterMarking> markings;

	int greenRange = 8, yellowRange = 15;
	int lTarget = *optimalLeftAngle + 90;
	int rTarget = *optimalRightAngle + 90;

	int lyl = lTarget - yellowRange;
	int lgl = lTarget - greenRange;
	int lgh = lTarget + greenRange;
	int lyh = lTarget + yellowRange;

	int ryl = rTarget - yellowRange;
	int rgl = rTarget - greenRange;
	int rgh = rTarget + greenRange;
	int ryh = rTarget + yellowRange;

	markings.push_back({ (char)255, (char)255, (char)255, opacity, 3, lgh });
	markings.push_back({ (char)255, (char)255, (char)255, opacity, 3, lyh });
	markings.push_back({ (char)255, (char)255, (char)255, opacity, 3, lgl });
	markings.push_back({ (char)255, (char)255, (char)255, opacity, 3, lyl });
	markings.push_back({ (char)255, (char)255, (char)255, opacity, 3, rgh });
	markings.push_back({ (char)255, (char)255, (char)255, opacity, 3, ryh });
	markings.push_back({ (char)255, (char)255, (char)255, opacity, 3, rgl });
	markings.push_back({ (char)255, (char)255, (char)255, opacity, 3, ryl });

	ranges.push_back({ (char)255, (char)255, (char)50, 0.2, lyl, lgl });
	ranges.push_back({ (char)50, (char)255, (char)50, 0.2, lgl, lgh });
	ranges.push_back({ (char)255, (char)255, (char)50, 0.2, lgh, lyh });
	ranges.push_back({ (char)255, (char)255, (char)50, 0.2, ryl, rgl });
	ranges.push_back({ (char)50, (char)255, (char)50, 0.2, rgl, rgh });
	ranges.push_back({ (char)255, (char)255, (char)50, 0.2, rgh, ryh });

	if (attempt.dodged)
	{
		int angle = attempt.dodgeAngle;
		if (angle > 90) angle = 90;
		if (angle < -90) angle = -90;

		int angleAdjusted = 90 + angle;
		markings.push_back({ (char)0, (char)0, (char)0, 0.6, unitWidth, angleAdjusted });

		if (angleAdjusted < lyl)
		{
			ranges.push_back({ (char)255,(char)50, (char)50, 1, 0, lyl });
		}
		else if (angleAdjusted < lgl)
		{
			ranges.push_back({ (char)255, (char)255, (char)50, 1, lTarget - yellowRange, lTarget - greenRange });
		}
		else if (angleAdjusted < lgh)
		{
			ranges.push_back({ (char)50, (char)255, (char)50, 1, lTarget - greenRange, lTarget + greenRange });
		}
		else if (angleAdjusted < lyh)
		{
			ranges.push_back({ (char)255, (char)255, (char)50, 1, lTarget + greenRange, lTarget + yellowRange });
		}
		else if (angleAdjusted < ryl)
		{
			ranges.push_back({ (char)255,(char)50, (char)50, 1, lTarget + yellowRange, rTarget - yellowRange });
		}
		else if (angleAdjusted < rgl)
		{
			ranges.push_back({ (char)255, (char)255, (char)50, 1, rTarget - yellowRange, rTarget - greenRange });
		}
		else if (angleAdjusted < rgh)
		{
			ranges.push_back({ (char)50, (char)255, (char)50, 1, rTarget - greenRange, rTarget + greenRange });
		}
		else if (angleAdjusted < ryh)
		{
			ranges.push_back({ (char)255, (char)255, (char)50, 1, rTarget + greenRange, rTarget + yellowRange });
		}
		else
		{
			ranges.push_back({ (char)255, (char)50, (char)50, 1, rTarget + yellowRange, totalUnits });
		}
	}

	RenderMeter(canvas, startPos, reqSize, baseColor, border, totalUnits, ranges, markings, false);

	//draw angle label
	canvas.SetColor(255, 255, 255, (char)(255 * opacity));
	canvas.SetPosition(Vector2{ startPos.X, (int)(startPos.Y - 20) });
	canvas.DrawString("Dodge Angle: " + to_string(attempt.dodgeAngle) + " DEG", 1, 1, true, false);

	//draw time to ball label
	if (attempt.hit && attempt.ticksToBall > 0)
	{
		auto ms = attempt.ticksToBall * 1.0 / 120.0;
		//string msg = fmt::format("Time to ball: {0:.3f}s", attempt.timeToBall);
		string msg = fmt::format("Time to ball: {0:.4f}s", ms);

		int width = (msg.length() * 8) - (5 * 3); // 8 pixels per char - 5 pixels per space

		canvas.SetColor(255, 255, 255, (char)(255 * opacity));
		canvas.SetPosition(Vector2{ startPos.X + (int)(boxSize.X / 2) - (width/2), startPos.Y - 20 });
		canvas.DrawString(msg, 1, 1, true, false);
	}

	string msg = fmt::format("Horizontal distance traveled: {0:.1f}", attempt.traveledY);
	int width = (msg.length() * 6.6);

	//draw angle label
	if(attempt.traveledY < 225)
		canvas.SetColor(50, 255, 50, (char)(255 * opacity));
	else if(attempt.traveledY < 425)
		canvas.SetColor(255, 255, 50, (char)(255 * opacity));
	else
		canvas.SetColor(255, 10, 10, (char)(255 * opacity));

	canvas.SetPosition(Vector2{ startPos.X + boxSize.X - width, (int)(startPos.Y - 20) });
	canvas.DrawString(msg, 1, 1, true, false);

	int start = 10;
	if (attempt.startedInAir)
	{
		msg = "Started before car touched the ground! Wait for car to settle on next attempt.";
		canvas.SetColor(255, 10, 10, (char)(255 * opacity));
		canvas.SetPosition(Vector2{ startPos.X + (boxSize.X / 2) - 265, (int)(startPos.Y + boxSize.Y + start) });
		canvas.DrawString(msg, 1, 1, true, false);
		start += 15;
	}
	if (attempt.startedNoBoost)
	{
		msg = "Was not pressing boost when time started! Press boost before you press throttle on next attempt.";
		canvas.SetColor(255, 10, 10, (char)(255 * opacity));
		canvas.SetPosition(Vector2{ startPos.X + (boxSize.X / 2) - 330, (int)(startPos.Y + boxSize.Y + start) });
		canvas.DrawString(msg, 1, 1, true, false);
	}
}

void SpeedFlipTrainer::PlayAttempt(Attempt* a, shared_ptr<GameWrapper> gameWrapper, ControllerInput* ci)
{
	if (a->inputs.size() <= 0)
		return;

	int currentPhysicsFrame = gameWrapper->GetEngine().GetPhysicsFrame();
	int tick = currentPhysicsFrame - startingPhysicsFrame;

	a->Play(ci, tick);

	gameWrapper->OverrideParams(ci, sizeof(ControllerInput));
}

void SpeedFlipTrainer::PlayBot(shared_ptr<GameWrapper> gameWrapper, ControllerInput* ci)
{
	int currentPhysicsFrame = gameWrapper->GetEngine().GetPhysicsFrame();
	int tick = currentPhysicsFrame - startingPhysicsFrame;

	bot.Play(ci, tick);

	gameWrapper->OverrideParams(ci, sizeof(ControllerInput));
}