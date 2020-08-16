#include <iostream>
#include <vector>
#include <fstream>

#include "sre/Renderer.hpp"
#include "sre/SDLRenderer.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <sre/Skybox.hpp>
#include <sre/ModelImporter.hpp>
#include <glm/gtc/type_ptr.hpp>
// Needed to add the following (remove later) to access 'eulerAngleXYZ'
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/constants.hpp>
// Needed the following (remove later) to assert a camera condition
#include <cassert>

// Things to do:
// 2) Place a few objects in the domain
// 3) Create a camera class (use LearnOpenGL as a [partial] example)
// 4) Do full "space flight" demo for Max (show original camera demo first,
//		followd by mileposts in development [if Max wants to see that much])
// 5) Fork SimpleRenderEngine
//		a) Create branch for porting fixes
//		b) Test on port to Windows and new Linux computer
//		c) Make pull request
//		d) Create branch for new Camera
//		e) Add updated Camera class and new example and make pull request
// 6) Scale movement (both keyboard and mouse) by frame rate

using namespace sre;
using namespace glm;

// Definition of Cube class
class Cube {
public:
    Cube();
    Cube(const glm::vec3 & center, const float& sideLen);
    void initializeVertexes();
    void draw(RenderPass& renderPass);
    void setCenter(const glm::vec3& center);
    glm::vec3 getCenter();
private:
    float mSideLen;
    glm::vec3 mCenter, m_xAxis, m_yAxis, m_zAxis;
    glm::vec3 mP1, mP2, mP3, mP4, mP5, mP6, mP7, mP8;
    glm::vec3 mWorldOrigin;
};

// Definition of GridPlane class
class GridPlane {
public:
    GridPlane();
    GridPlane(const glm::vec3 & center, const float& sideLen,
			  const float& gridSpace, const Color& color);
    void initializeGrid();
    void draw(RenderPass& renderPass);
private:
    glm::vec3 mCenter;
    float mSideLen;
	float mGridSpace;
	int mNumCells;
	Color mColor;
    std::vector<std::vector<glm::vec3>> m_zLines;
    std::vector<std::vector<glm::vec3>> m_xLines;
};

// Global variables ============================================================

float worldUnit = 1.0; // Used as a unit of measure to scale all objects
float elapsedTime = 0.0f;
std::shared_ptr<Mesh> sphere;
std::shared_ptr<Mesh> Suzanne; // Monkey object
Cube cube {{0.0, 0.0, 0.0}, 5.0f * worldUnit};

// Grids (note: grid should come first, others use info)
float gridSpace = 5.0f * worldUnit;
float gridLength = 150.0f * worldUnit;
Color black {0.0f, 0.0f, 0.0f, 1.0f};
Color white {1.0f, 1.0f, 1.0f, 1.0f};
GridPlane gridPlaneTop {{0.0, 4.0 * gridSpace, 0.0},
						gridLength, gridSpace, black};
GridPlane gridPlaneBottom {{0.0, -4.0 * gridSpace, 0.0},
							gridLength, gridSpace, white};

FPS_Camera camera;
WorldLights worldLights;
std::shared_ptr<Skybox> skybox;

// Mouse callback state
bool mouseDown = false;
int lastMouse_x = 0;
int lastMouse_y = 0;

// Definition of callback functions
void frameUpdate(float deltaTime);
void frameRender();
void keyEvent(SDL_Event& event);
void mouseEvent(SDL_Event& event);

// Main function ===============================================================

int main() {

	// Define and initialize Graphics renderer (needs to be done first)
    SDLRenderer renderer;
    renderer.init();
	// Assign SDLRenderer 'callback' functions to functions implemented below
    renderer.frameUpdate = frameUpdate;
    renderer.frameRender = frameRender;
    renderer.mouseEvent = mouseEvent;
    renderer.keyEvent = keyEvent;

	// Create camera
	glm::vec3 position {0.0, 0.0, 65.0f * worldUnit};
	glm::vec3 direction {0.0, 0.0, -1.0};
	glm::vec3 worldUp {0.0, 1.0, 0.0};
	float speed = 2.0f * worldUnit; // 2 worldUnits / second
	float fieldOfView= 45.0;
	camera.init(position, direction, worldUp, speed, fieldOfView);
//	camera.init({0.0, 0.0,  10.0f * worldUnit}, // position
//				{0.0, 0.0, -1.0}, // direction
//				{0.0, 1.0,  0.0}, // worldUp
//				2.0f * worldUnit, 45.0) { // speed, fieldOfView

	// Create lighting
    worldLights.setAmbientLight({0.05,0.05,0.05});
	Light sun = Light::create()
				.withDirectionalLight({1, 1,1})
				.withColor({1,1,1})
				.build();
    worldLights.addLight(sun);

	// Create the sky (with a horizon, called the 'Skybox')
    skybox = Skybox::create();

	// Create sphere
	std::shared_ptr<Material> sphereMaterial;
    sphereMaterial = Shader::getStandardPBR()->createMaterial();
    sphereMaterial->setColor({0.0f, 1.0f, 0.0f, 1.0f});
    sphereMaterial->setMetallicRoughness({0.5f, 0.5f});
    sphere = Mesh::create()
					.withSphere()
					.withLocation({0.0, 0.0, 0.0})
					.withScale(worldUnit)
					.withMaterial(sphereMaterial)
					.build();

	// Create the monkey object "Suzanne"
	std::shared_ptr<Material> SuzanneMaterial;
    SuzanneMaterial = Shader::getStandardPBR()->createMaterial();
    SuzanneMaterial->setColor({1.0f, 0.7f, 0.2f, 1.0f});
    SuzanneMaterial->setMetallicRoughness({0.5f, 0.5f});
	Suzanne = sre::ModelImporter::importObj("examples_data/", "suzanne.obj");
	Suzanne->setLocation({20.0f * worldUnit, 0.0, 0.0});
	Suzanne->setScale(worldUnit);
	Suzanne->setMaterial(SuzanneMaterial);

	// Capture the mouse (mouse won't be visible)
	//SDL_SetRelativeMouseMode(SDL_TRUE);
	//SDL_CaptureMouse(SDL_TRUE);

	// Start processing mouse and keyboard events (continue until user quits)
    renderer.startEventLoop();

	// Exit the program
    return 0;
}

// Callback functions ==========================================================

// Update the rendering frame (move the sphere)
void frameUpdate(float deltaTime) {
	elapsedTime++;
	glm::vec3 sphereLocation = sphere->Location();	
	sphereLocation.z += cos(elapsedTime/50.0f)/7.0;
	sphere->setLocation(sphereLocation);
};

// Render (draw) the updated frame
void frameRender() {
	// Create render pass, initialize with world variables
	auto renderPass = RenderPass::create()
   	     .withCamera(camera)
   	     .withWorldLights(&worldLights)
   	     .withSkybox(skybox)
   	     .withName("Frame")
   	     .build();
	// Draw objects
	sphere->draw(renderPass);
	Suzanne->draw(renderPass);
	cube.draw(renderPass);
	gridPlaneTop.draw(renderPass);
	gridPlaneBottom.draw(renderPass);
};

// Process keyboard events
void keyEvent(SDL_Event& event) {
	auto key = event.key.keysym.sym;
	// Check for key presses and take appropriate action
	if (event.type == SDL_KEYDOWN) {
		if (key == SDLK_EQUALS || key == SDLK_MINUS) {
			float zoomIncrement = 5.0;
			if (key == SDLK_EQUALS) {
				camera.zoom(zoomIncrement);
			} else if (key == SDLK_MINUS) {
				camera.zoom(-zoomIncrement);
			}
		}
		if (key == SDLK_w || key == SDLK_k || key == SDLK_UP ||
			key == SDLK_s || key == SDLK_j || key == SDLK_DOWN ||
			key == SDLK_a || key == SDLK_h || key == SDLK_LEFT ||
			key == SDLK_d || key == SDLK_l || key == SDLK_RIGHT ||
			key == SDLK_LCTRL || key == SDLK_SPACE || key == SDLK_LSHIFT ||
			key == SDLK_z) {
			// Calculate the distance the camera should move
			// Assume user presses a key in 1/5 of a second (5 presses per sec)
			// Distance traveled in 1/5 of a second = speed (unit/s) * time (s)
			float distance = camera.Speed() * 1.0/5.0;
			if (key == SDLK_w || key == SDLK_k || key == SDLK_UP) {
				// Move camera forward in horizontal plane towards target
				camera.move(FPS_Camera::Direction::Forward, distance);
			} else if (key == SDLK_s || key == SDLK_j || key == SDLK_DOWN) {
				// Move camera backward in horizontal plane away from target
				camera.move(FPS_Camera::Direction::Backward, distance);
			} else if (key == SDLK_a || key == SDLK_h || key == SDLK_LEFT) {
				// Move camera (strafe) left
				camera.move(FPS_Camera::Direction::Left, distance);
			} else if (key == SDLK_d || key == SDLK_l || key == SDLK_RIGHT) {
				// Move camera (strafe) right
				camera.move(FPS_Camera::Direction::Right, distance);
			} else if (key == SDLK_SPACE) {		// Minecraft uses LCTRL || SPACE
				// Move camera up vertically 
				camera.move(FPS_Camera::Direction::Up, distance);
			} else if (key == SDLK_z) {   		// Minecraft uses LSHIFT
				// Move camera down vertically	// Max uses  LCTRL
				camera.move(FPS_Camera::Direction::Down, distance);
			}
		}
	}
}

// Process mouse events
void mouseEvent(SDL_Event& event) {
	// SDL_GetMouseState(&xpos, &ypos); // Use to set the mouse position?
    if (event.type == SDL_MOUSEBUTTONDOWN) {
		lastMouse_x = event.button.x;	
		lastMouse_y = event.button.y;	
		mouseDown = true;
	} else if (event.type == SDL_MOUSEBUTTONUP) {
		mouseDown = false;
	} else if (event.type == SDL_MOUSEMOTION && mouseDown) {
		// Mouse movement is degrees per pixel moved
        float degreesPerPixel = 0.1;
		float yaw = (event.motion.x - lastMouse_x) * degreesPerPixel;
		float pitch = (lastMouse_y - event.motion.y) * degreesPerPixel;
		lastMouse_x = event.motion.x;
		lastMouse_y = event.motion.y;
		camera.pitchAndYaw(pitch, yaw);
    } else if (event.type == SDL_MOUSEWHEEL){ // Works for two-finger touch
		float zoomPerClick = 0.5;
		float zoomIncrement = event.wheel.y * zoomPerClick;
		camera.zoom(zoomIncrement);
	}
}

// Cube class =================================================================

Cube::Cube() {
	mWorldOrigin = {0.0, 0.0, 0.0};
	mSideLen = {1.0};
	mCenter = {0,0,0};
	initializeVertexes();
}

Cube::Cube(const glm::vec3 & center, const float& sideLen) {
	mWorldOrigin = {0.0, 0.0, 0.0};
	mSideLen = {sideLen};
	mCenter = {center};
	initializeVertexes();
}

void
Cube::initializeVertexes() {
	//Points at end of of cube coordinate axes
//	m_xAxis {1.1*mSideLen, mCenter.y, mCenter.z};
//	m_yAxis {mCenter.x, 1.1*mSideLen, mCenter.z};
//	m_zAxis {mCenter.x, mCenter.y, 1.1*mSideLen};

	//Points at end of of world coordinate axes
	m_xAxis = {1.1*mSideLen, mWorldOrigin.y, mWorldOrigin.z};
	m_yAxis = {mWorldOrigin.x, 1.1*mSideLen, mWorldOrigin.z};
	m_zAxis = {mWorldOrigin.x,mWorldOrigin.y, 1.1*mSideLen};

	// Points of cube
	mP1 = {mCenter.x-0.5*mSideLen, mCenter.y+0.5*mSideLen, 
														mCenter.z+0.5*mSideLen};
	mP2 = mP1 + vec3(mSideLen, 0, 0);
	mP3 = mP2 + vec3(0, -mSideLen, 0);
	mP4 = mP3 + vec3(-mSideLen, 0, 0);
	mP5 = mP1 + vec3(0, 0, -mSideLen);
	mP6 = mP2 + vec3(0, 0, -mSideLen);
	mP7 = mP3 + vec3(0, 0, -mSideLen);
	mP8 = mP4 + vec3(0, 0, -mSideLen);
}

void
Cube::draw(RenderPass& renderPass) {
	Color red {1.0f, 0.0f, 0.0f, 1.0f};
	Color green {0.0f, 1.0f, 0.0f, 1.0f};
	Color blue {0.0f, 0.0f, 1.0f, 1.0f};
	// Draw cube coordinate axes
//	renderPass.drawLines({mCenter, m_xAxis}, red);
//	renderPass.drawLines({mCenter, m_yAxis}, green);
//	renderPass.drawLines({mCenter, m_zAxis}, blue);
	// Draw world coordinate axes
	renderPass.drawLines({mWorldOrigin, m_xAxis}, red);
	renderPass.drawLines({mWorldOrigin, m_yAxis}, green);
	renderPass.drawLines({mWorldOrigin, m_zAxis}, blue);
	// Draw cube sides
	renderPass.drawLines({mP1, mP2});
	renderPass.drawLines({mP2, mP3, mP3, mP4, mP4, mP1});
	renderPass.drawLines({mP5, mP6, mP6, mP7, mP7, mP8, mP8, mP5});
	renderPass.drawLines({mP1, mP5, mP2, mP6, mP3, mP7, mP4, mP8});
}

void
Cube::setCenter(const glm::vec3& center) {
	mCenter = center;
	initializeVertexes();
}

vec3
Cube::getCenter() {
	return mCenter;
}

// GridPlane class ============================================================

GridPlane::GridPlane() {
	mCenter = {0,0,0};
	mSideLen = {10.0};
	mGridSpace = 1.0;
	mNumCells = static_cast<int>(mSideLen/mGridSpace);
	mColor = {1.0f, 0.0f, 0.0f, 1.0f}; // Red
	initializeGrid();
}

GridPlane::GridPlane(const glm::vec3 & center, const float& sideLen,
					 const float& gridSpace, const Color& color) {
	mCenter = {center};
	mSideLen = {sideLen};
	mGridSpace = {gridSpace};
	mNumCells = static_cast<int>(mSideLen/mGridSpace);
	mColor = color;
	initializeGrid();
}

void
GridPlane::initializeGrid() {
	
	m_zLines.resize(mNumCells+1);
	for (auto& z : m_zLines) {
		z.resize(mNumCells + mNumCells);
	}
	vec3 lowerLeft = {-0.5*mSideLen + mCenter.x, mCenter.y,
					  -0.5*mSideLen + mCenter.z};
	vec3 currentCoord = lowerLeft;

	for (auto i = 0; i < mNumCells+1; i++) {
		for (auto j = 0; j < mNumCells +  mNumCells; j++) {
			m_zLines[i][j] = currentCoord;
			if (j % 2 == 0) currentCoord.x += mGridSpace;	
		}
		currentCoord.x = lowerLeft.x;
		currentCoord.z += mGridSpace;
	}

	m_xLines.resize(mNumCells+1);
	for (auto& x : m_xLines) {
		x.resize(mNumCells + mNumCells);
	}

	currentCoord = lowerLeft;

	for (auto i = 0; i < mNumCells+1; i++) {
		for (auto j = 0; j < mNumCells +  mNumCells; j++) {
			m_xLines[i][j] = currentCoord;
			if (j % 2 == 0) currentCoord.z += mGridSpace;	
		}
		currentCoord.z = lowerLeft.z;
		currentCoord.x += mGridSpace;
	}
}

void
GridPlane::draw(RenderPass& renderPass) {
	for (auto i = 0; i < mNumCells+1; i++) {
		renderPass.drawLines(m_zLines[i], mColor);
	}
	for (auto i = 0; i < mNumCells+1; i++) {
		renderPass.drawLines(m_xLines[i], mColor);
	}
}
