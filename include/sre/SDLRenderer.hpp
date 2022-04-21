/*
 *  SimpleRenderEngine (https://github.com/mortennobel/SimpleRenderEngine)
 *
 *  Created by Morten Nobel-Jørgensen ( http://www.nobel-joergensen.com/ )
 *  License: MIT
 */

#pragma once

#ifdef _SDL_H
#error SDL should not be included before SDLRenderer
#endif

#define SDL_MAIN_HANDLED

#include "SDL.h"


#include <functional>
#include <string>
#include <sstream>
#include "sre/Renderer.hpp"



namespace sre {

// forward declaration
class Renderer;

// Simplifies SDL applications by abstracting away boilerplate code.
//
// SDLRenderer is a pure helper-class, and no other class in the SimpleRenderEngine depends on it.
//
// The class will create a window with a graphics context in the `init()` member function.
// The `startEventLoop()` will start the event loop, which polls the event queue in the
// beginning of each frame (and providing callbacks to `keyEvent` and `mouseEvent`), followed by a `frameUpdate(float)`
// and a `frameRender()`.

enum class Cursor {Arrow, Wait, Hand, SizeAll};

class DllExport SDLRenderer {
public:
    class InitBuilder {
    public:
        ~InitBuilder();
        InitBuilder& withSdlInitFlags(uint32_t sdlInitFlag);            // Set SDL Init flags (See: https://wiki.libsdl.org/SDL_Init )
        InitBuilder& withSdlWindowFlags(uint32_t sdlWindowFlags);       // Set SDL Window flags (See: https://wiki.libsdl.org/SDL_WindowFlags )
        InitBuilder& withVSync(bool vsync);
        InitBuilder& withGLVersion(int majorVersion, int minorVersion);
        InitBuilder& withMaxSceneLights(int maxSceneLights);            // Set max amount of concurrent lights
        InitBuilder& withMinimalRendering(bool minimalRendering);       // Minimize rendering for graphics that are mostly static (e.g. drawings)
        void build();
    private:
        explicit InitBuilder(SDLRenderer* sdlRenderer);
        SDLRenderer* sdlRenderer;
        uint32_t sdlInitFlag = SDL_INIT_EVERYTHING;
        uint32_t sdlWindowFlags = SDL_WINDOW_ALLOW_HIGHDPI  | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
        bool vsync = true;
        int glMajorVersion = 3;
        int glMinorVersion = 3;
        int maxSceneLights = 4;
        bool minimalRendering = false;
        friend class SDLRenderer;
    };

    SDLRenderer();
    virtual ~SDLRenderer();

    // event handlers (assigned empty default handlers)
    std::function<void(float deltaTimeSec)> frameUpdate;        // Callback every frame with time since last callback in seconds
    std::function<void()> frameRender;                          // Callback to render events - called after frameUpdate. The `Renderer::swapFrame()` is
                                                                // automatically invoked after the callback.
    std::function<void()> stopProgram;                          // Callback to stop the program -- called upon "SDL_QUIT". This gives the program an opportunity
                                                                // to perform and orderly shutdown before calling "stopEventLoop()" to end the program.
    std::function<void(SDL_Event& e)> keyEvent;                 // Callback of `SDL_KEYDOWN` and `SDL_KEYUP`.
    std::function<void(SDL_Event& e)> mouseEvent;               // Callback of `SDL_MOUSEMOTION`, `SDL_MOUSEBUTTONDOWN`, `SDL_MOUSEBUTTONUP`, `SDL_MOUSEWHEEL`.
    std::function<void(SDL_Event& e)> controllerEvent;          // Callback of `SDL_CONTROLLERAXISMOTION`, `SDL_CONTROLLERBUTTONDOWN`, `SDL_CONTROLLERBUTTONUP`,
                                                                // `SDL_CONTROLLERDEVICEADDED`, `SDL_CONTROLLERDEVICEREMOVED` and `SDL_CONTROLLERDEVICEREMAPPED`.
    std::function<void(SDL_Event& e)> joystickEvent;            // Callback of `SDL_JOYAXISMOTION`, `SDL_JOYBALLMOTION`, `SDL_JOYHATMOTION`, `SDL_JOYBUTTONDOWN`,
                                                                // `SDL_JOYBUTTONUP`, `SDL_JOYDEVICEADDED`, `SDL_JOYDEVICEREMOVED`.
    std::function<void(SDL_Event& e)> touchEvent;               // Callback of `SDL_FINGERDOWN`, `SDL_FINGERUP`, `SDL_FINGERMOTION`.
    std::function<void(SDL_Event& e)> otherEvent;               // Invoked if unhandled SDL event

    InitBuilder init();                                         // Create the window and the graphics context (instantiates the sre::Renderer). Note that most
                                                                // other sre classes requires the graphics content to be created before they can be used (e.g.
                                                                // a Shader cannot be created before `init()`).
                                                                // The initialization happens on InitBuilder::build or InitBuilder::~InitBuilder()

    void setWindowTitle(std::string title);
    void setWindowIcon(std::shared_ptr<Texture> tex);           // Set application icon
    void setWindowSize(glm::ivec2 size);

    void setFullscreen(bool enabled = true);                    // Toggle fullscreen mode (default mode is windowed). Not supported in Emscripten
    bool isFullscreen();

    void setMouseCursorVisible(bool enabled = true);            // Show/hide mouse cursor. Not supported in Emscripten
    bool isMouseCursorVisible();                                // GUI should not be rendered when mouse cursor is not visible (this would force the mouse
                                                                // cursor to appear again)

    bool setMouseCursorLocked(bool enabled = true);             // Lock the mouse cursor, such that mouse cursor motion is detected, (while position remains
                                                                // fixed). Not supported in Emscripten
    bool isMouseCursorLocked();                                 // Locking the mouse cursor automatically hides the mouse cursor

    void startEventLoop();                                      // Start the event loop. Note that this member function in usually blocking (until the
                                                                // `stopEventLoop()` has been  called). Using Emscripten the event loop is not blocking (but
                                                                // internally using a callback function), which means that when using Emscripten avoid
                                                                // allocating objects on the stack (see examples for a workaround).

    void stopEventLoop();                                       // The render loop will stop running when the frame is complete.

    void startEventSubLoop();                                   // Start a second event loop within the main event loop. Useful when mouse and keyboard events
                                                                // need to be captured deep within a time-consuming function. Note that there will be a
                                                                // discrepancy between the detaTime passed to the first call to frameUpdate(deltaTimeSec) in
                                                                // the event sub-loop and the last call in the main event loop.

    void stopEventSubLoop();                                    // The render sub-loop will stop running when the frame is complete.

    void drawFrame();                                           // Draw a single frame. This is useful when application graphics need to be updated from deep
                                                                // within a time-consuming function while not desiring user input (for example, a progress
                                                                // dialog).
    int getFrameNumber();                                       // Get the current drawing frame number. This is useful to label output (e.g. screen images).

    void startEventLoop(std::shared_ptr<VR> vr);                // Start event loop for VR

    SDL_Window *getSDLWindow();                                 // Get a pointer to SDL_Window

    static SDLRenderer* instance;                               // Singleton reference to the engine after initialization.

    glm::vec3 getLastFrameStats();                              // Returns delta time for last frame wrt event, update and render

    void SetArrowCursor();                                      // Set the cursor to the default "arrow"
    void Begin(Cursor cursorIn);                                // Change the cursor to cursorIn
    void End(Cursor cursorIn);                                  // Restore the cursor to what it was when Begin(cursorIn) was called

    void SetMinimalRendering(bool minimalRendering);            // If SetMinimalRendering is passed "true", then SRE will minimize the number of rendering
                                                                // operations performed -- SRE will only render upon mouse or keyboard activity, or if the
                                                                // application notifies SRE that it has been updated through the SetAppUpdated method.

    void SetAppUpdated(bool appUpdated);                        // Let SRE know that the application has updated so that it will force rendering when
                                                                // using the flag MiniumalRendering. This will be set to "false" after the next render operation.

    bool startRecordingEvents(std::string fileName);            // Start recording SDL events (e.g. keyboard, mouse, mouse motion, etc.). This is useful
                                                                // for testing. Recommend only allowing recording right after application has started (e.g.
                                                                // start recording only based on a flag passed as an argument to the application) because
                                                                // it is nearly impossible to characterize the starting state of an application after a user
                                                                // has been operating for even a short amount of time)
    void setPauseRecordingOfTextEvents(const bool pause);       // Pause (or un-pause) recording SDL events
    bool stopRecordingEvents();                                 // End recording SDL events
    bool recordingEvents();                                     // Returns true if SRE is recording SDL events
    bool playBackRecordedEvents(std::string fileName);          // Play back recorded SDL events
    void setPausePlaybackOfEvents(const bool pause);            // Pause (or un-pause) playback of recorded SDL events
    bool playingBackEvents();                                   // Returns true if SRE is playing back recorded SDL events
    void captureFrameAndFinishRenderPass(RenderPass * renderPass); // Capture an image of the frame generated by renderPass and store in memory
    void writeCapturedImages();                                 // Write all the images stored in memory to files

    bool isKeyPressed(SDL_Keycode keyCode);                     // Return true if a specific key is pressed 
    bool isAnyKeyPressed();                                     // Return true if any key is pressed

private:
    void frame(float deltaTimeSec);
    Renderer* r;
    SDLRenderer(const SDLRenderer&) = delete;

    std::unique_ptr<VR> vr;
    std::string windowTitle;

    float timePerFrame = 1.0f/60.0f;

    // Event loop control and execution
    bool running = false;
    bool runningEventSubLoop = false;
    void executeEventLoop(bool& runEventLoop); // Implementation of event loop
    void processEvents();

    // Window properties
    int windowWidth = 800;
    int windowHeight = 600;
    SDL_Window *window = nullptr;

    float deltaTimeEvent;
    float deltaTimeUpdate;
    float deltaTimeRender;

    friend class SDLRendererInternal;
    friend class Inspector;

    // Minimal rendering option
    int frameNumber = 0;
    int lastEventFrameNumber = -99; // Invalid value to start
    bool appUpdated = false;
    bool minimalRendering = false;
    unsigned short nMinimalRenderingFrames = 10;

    // Handle mouse cursor changes
    SDL_Cursor* cursor;
    Cursor cursorType = Cursor::Arrow;
    bool imGuiWantCaptureMousePrevious = true;

    // Recording and playback of frames and events
    void recordFrame();
    void recordEvent(const SDL_Event& e);
    SDL_Event getNextRecordedEvent(bool& endOfFile);
    bool readRecordedEvents(std::string fileName);
    bool pushRecordedEventsForNextFrameToSDL();
    bool pushNextRecordedEventToSDL(bool endOfFile);
    int nextRecordedFramePeek();
    bool m_recordingEvents = false;
    bool m_playingBackEvents = false;
    std::string m_recordingFileName;
    std::stringstream m_recordingStream;
    std::stringstream m_playbackStream;
    int m_playbackFrame = -99; // Invalid value to start
    SDL_Keymod m_playbackKeymodState;
    Uint32 m_playbackMouseState;
    int m_playbackMouse_x;
    int m_playbackMouse_y;
    bool m_pausePlaybackOfEvents = false;
    bool m_pauseRecordingOfTextEvents = false;
    bool m_writingImages = false;
    std::string m_imageFileName = "Test";
    std::vector<std::vector<glm::u8vec4>> m_image;
    std::vector<glm::ivec2> m_imageDimensions;

    // Mouse and keyboard interface
    Uint32 getMouseState(int* x, int* y);                       // Intercept calls to SDL_GetMouseState for Dear ImGui during playback of recorded events
    SDL_Keymod getKeymodState();                                // Intercept calls to SDL_GetModState for Dear ImGui during playback of recorded events
    friend bool ImGui_SRE_ProcessEvent(SDL_Event *event);       // This ImGui SRE interface function calls getKeymodState
    friend void ImGui_SRE_NewFrame(SDL_Window *window);         // This ImGui SRE interface function calls getMouseState
    std::vector<SDL_Keycode> keyPressed;
    void addKeyPressed(SDL_Keycode keyCode);
    void removeKeyPressed(SDL_Keycode keyCode);
};

}
