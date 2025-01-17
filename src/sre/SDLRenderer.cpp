/*
 *  SimpleRenderEngine (https://github.com/mortennobel/SimpleRenderEngine)
 *
 *  Created by Morten Nobel-Jørgensen ( http://www.nobel-joergensen.com/ )
 *  License: MIT
 */

#include <chrono>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <algorithm>
#include <unistd.h> /* getopt utility for executable options */
#include <sre/imgui_sre.hpp>
#include <sre/Log.hpp>
#include <sre/VR.hpp>
#include "imgui.h"
#include <sre/imgui_addon.hpp>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#include "sre/SDLRenderer.hpp"
#define SDL_MAIN_HANDLED


#ifdef EMSCRIPTEN
#include "emscripten.h"
#endif
#include "sre/impl/GL.hpp"

#ifdef SRE_DEBUG_CONTEXT
void GLAPIENTRY openglCallbackFunction(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam) {
    using namespace std;
    const char* typeStr;
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        typeStr = "ERROR";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        typeStr = "DEPRECATED_BEHAVIOR";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        typeStr = "UNDEFINED_BEHAVIOR";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        typeStr = "PORTABILITY";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        typeStr = "PERFORMANCE";
        break;
    case GL_DEBUG_TYPE_OTHER:
    default:
        typeStr = "OTHER";
        break;
        }
    const char* severityStr;
    switch (severity) {
    case GL_DEBUG_SEVERITY_LOW:
        severityStr = "LOW";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        severityStr = "MEDIUM";
        break;
    case GL_DEBUG_SEVERITY_HIGH:
        severityStr = "HIGH";
        break;
    default:
        severityStr = "Unknown";
        break;
    }
    LOG_ERROR("---------------------opengl-callback-start------------\n"
              "message: %s\n"
              "type: %s\n"
              "id: %i\n"
              "severity: %s\n"
              "---------------------opengl-callback-end--------------"
              ,message,typeStr, id ,severityStr
    );

        }
#endif

namespace sre{

    SDLRenderer* SDLRenderer::instance = nullptr;

    struct SDLRendererInternal{
        static void update(float f){
            SDLRenderer::instance->frame(f);
        }
    };

    void update(){
        typedef std::chrono::high_resolution_clock Clock;
        using FpSeconds = std::chrono::duration<float, std::chrono::seconds::period>;
        static auto lastTick = Clock::now();
        auto tick = Clock::now();
        float deltaTime = std::chrono::duration_cast<FpSeconds>(tick - lastTick).count();
        lastTick = tick;
        SDLRendererInternal::update(deltaTime);
    }

    SDLRenderer::SDLRenderer()
    :frameUpdate ([](float){}),
     frameRender ([](){}),
     stopProgram ([this](){stopEventLoop();}),
     keyEvent ([](SDL_Event&){}),
     mouseEvent ([](SDL_Event&){}),
     controllerEvent ([](SDL_Event&){}),
     joystickEvent ([](SDL_Event&){}),
     touchEvent ([](SDL_Event&){}),
     otherEvent([](SDL_Event&){}),
     windowTitle( std::string("SimpleRenderEngine ")+std::to_string(Renderer::sre_version_major)+"."+std::to_string(Renderer::sre_version_minor )+"."+std::to_string(Renderer::sre_version_point))
    {

        instance = this;

    }

    SDLRenderer::~SDLRenderer() {
        delete r;
        r = nullptr;

        instance = nullptr;

        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void SDLRenderer::frame(float deltaTimeSec){
        typedef std::chrono::high_resolution_clock Clock;
        using MilliSeconds = std::chrono::duration<float, std::chrono::milliseconds::period>;
        auto lastTick = Clock::now();

        processEvents();

        // Determine whether to render frame for "minimalRendering" option
        bool shouldRenderFrame = true;
        if (minimalRendering) {
            if (appUpdated || isAnyKeyPressed()) {
                if (m_recordingEvents && lastEventFrameNumber != frameNumber) {
                    // Record frame for app update or if any key is pressed
                    // (unless event is already recorded)
                    recordFrame();
                }
                lastEventFrameNumber = frameNumber;
                if (appUpdated) {
                    appUpdated = false;
                }
            }
            if (frameNumber > lastEventFrameNumber + nMinimalRenderingFrames) {
                // Draw at least two frames after each event: one to allow ImGui
                // to handle the event and one to process actions triggered by
                // ImGui (e.g. draw #1 draws pressed down OK button, draw #2
                // hides the window and executes actions resulting from OK).
                // However, ImGui uses 10 frames to "fade" gray screen for modal
                // dialogs: respect this feature by using 10 rendering frames.
                // TODO: create a function in ImGui called io.WantToRender() that
                //       determines whether ImGui wants to render (e.g. needs to
                //       redraw something or fade for a modal window). This should
                //       return false if ImGui is just waiting for an event.
                //       Then check this new function to decide if we should
                //       render the frame when no event and no key down.
                shouldRenderFrame = false;
            }
        }
        // Update and draw frame, measure times, and swap window
        {   // Measure time for event processing
            auto tick = Clock::now();
            deltaTimeEvent = std::chrono::duration_cast<MilliSeconds>(tick - lastTick).count();
            lastTick = tick;
        }
        if (shouldRenderFrame) {
            frameUpdate(deltaTimeSec);
            {   // Measure time for updating the frame
                auto tick = Clock::now();
                deltaTimeUpdate = std::chrono::duration_cast<MilliSeconds>(tick - lastTick).count();
                lastTick = tick;
            }
            frameRender();
            {   // Measure time for rendering the frame
                auto tick = Clock::now();
                deltaTimeRender = std::chrono::duration_cast<MilliSeconds>(tick - lastTick).count();
                lastTick = tick;
            }
            if (m_recordingEvents && frameNumber > lastEventFrameNumber
                    && frameNumber <= lastEventFrameNumber + 2) {
                // Only record two frames after the last event (see minimal
                // rendering comments above)
                recordFrame();
            }
            r->swapWindow();
            frameNumber++;
        } else {
            deltaTimeUpdate = 0.0f;
            deltaTimeRender = 0.0f;
        }
    }

    void
    SDLRenderer::processEvents() {
        SDL_Event e;
        if (m_playingBackEvents) {
            while( SDL_PollEvent( &e) != 0 ) {
                // Remove events in event queue by polling them. Note that this
                // will prevent any user interaction during playback. Changing
                // the SRE window title to reflect this is recommended.
            }
            if (!m_pausePlaybackOfEvents) {
                pushRecordedEventsForNextFrameToSDL();
            }
        }
        //Handle events on queue
        while( SDL_PollEvent( &e ) != 0 )
        {
            lastEventFrameNumber = frameNumber;

            if (m_recordingEvents) {
                recordEvent(e);
            }

            ImGuiIO& imguiIO = ImGui::GetIO();
            ImGui_SRE_ProcessEvent(&e);
            auto key = e.key.keysym.sym;
            auto keyState = e.key.state;
            bool hotKey;

            switch (e.type) {
                case SDL_QUIT:
                    stopProgram();
                    break;
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    // Dispatch key events to app (through keyEvent) if ImGui
                    // does not want key event or key event is a hotkey
                    hotKey   =  (key == SDLK_F1  || key == SDLK_F2
                              || key == SDLK_F3  || key == SDLK_F4
                              || key == SDLK_F5  || key == SDLK_F6
                              || key == SDLK_F7  || key == SDLK_F8
                              || key == SDLK_F9  || key == SDLK_F10
                              || key == SDLK_F11 || key == SDLK_F12
                              || key == SDLK_UP  || key == SDLK_DOWN);
                    if (!imguiIO.WantCaptureKeyboard || hotKey)
                        keyEvent(e);
                    // Remember pressed keys (check for rendering and recording)
                    if (keyState == SDL_PRESSED) {
                        addKeyPressed(key);
                    } else {
                        removeKeyPressed(key);
                    }
                    break;
                case SDL_MOUSEMOTION:
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                case SDL_MOUSEWHEEL:
                    if (!imguiIO.WantCaptureMouse
                                   && imGuiWantCaptureMousePrevious)
                    {
                        // If ImGui changed from wanting the mouse to not
                        // wanting it, set the cursor to the regular cursor
                        // (sometimes ImGui will not reset the cursor when
                        // it no longer wants to capture)
                        SetArrowCursor();
                        // Block ImGui from setting mouse cursor: allow user set
                        imguiIO.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
                    }
                    if (!imguiIO.WantCaptureMouse)
                    {
                        // Pass event to SRE
                        mouseEvent(e);
                    }
                    else // imguiIO.WantCaptureMouse
                    {
                        // Allow ImGui to set mouse cursor
                        imguiIO.ConfigFlags = !ImGuiConfigFlags_NoMouseCursorChange;
                        // Do not pass event to SRE
                    }
                    imGuiWantCaptureMousePrevious = imguiIO.WantCaptureMouse;
                    break;
                case SDL_CONTROLLERAXISMOTION:
                case SDL_CONTROLLERBUTTONDOWN:
                case SDL_CONTROLLERBUTTONUP:
                case SDL_CONTROLLERDEVICEADDED:
                case SDL_CONTROLLERDEVICEREMOVED:
                case SDL_CONTROLLERDEVICEREMAPPED:
                    controllerEvent(e);
                    break;
                case SDL_JOYAXISMOTION:
                case SDL_JOYBALLMOTION:
                case SDL_JOYHATMOTION:
                case SDL_JOYBUTTONDOWN:
                case SDL_JOYBUTTONUP:
                case SDL_JOYDEVICEADDED:
                case SDL_JOYDEVICEREMOVED:
                    joystickEvent(e);
                    break;
                case SDL_FINGERDOWN:
                case SDL_FINGERUP:
                case SDL_FINGERMOTION:
                    touchEvent(e);
                    break;
                default:
                    otherEvent(e);
                    break;
            }
        }
    }

    void SDLRenderer::startEventLoop() {
        if (!window){
            LOG_INFO("SDLRenderer::init() not called");
        }
        running = true;
        executeEventLoop(running);
    }

    void SDLRenderer::stopEventLoop() {
        running = false;
        runningEventSubLoop = false;
        if (m_recordingEvents) {
            std::string errorMessage;
            if (!stopRecordingEvents(errorMessage)) {
                LOG_ERROR(errorMessage.c_str());
            }
        }
    }

    void SDLRenderer::startEventSubLoop() {
        if (!running) return; // Only allow start as a sub-loop
        if (runningEventSubLoop)
            LOG_INFO("Multiple simultaneous render sub-loops attempted");
        else
        {
            runningEventSubLoop = true;
            executeEventLoop(runningEventSubLoop);
        }
    }

    void SDLRenderer::stopEventSubLoop() {
        runningEventSubLoop = false;
    }

    void SDLRenderer::executeEventLoop(bool& runEventLoop) {
#ifdef EMSCRIPTEN
        emscripten_set_main_loop(update, 0, 1);
#else
        typedef std::chrono::high_resolution_clock Clock;
        using FpSeconds = std::chrono::duration<float, std::chrono::seconds::period>;
        auto lastTick = Clock::now();
        float deltaTime = 0;

        while (runEventLoop){
            frame(deltaTime);

            auto tick = Clock::now();
            deltaTime = std::chrono::duration_cast<FpSeconds>(tick - lastTick).count();

            while (deltaTime < timePerFrame){
                Uint32 delayMs;
                float delayS = timePerFrame - deltaTime;
                if (!minimalRendering)
                {   // Match frame rate exactly by underestimating delay
                    // The while loop will fill the < 1 Millisecond gap
                    delayMs = static_cast<Uint32>(delayS * 1000.0f);
                }
                else
                {   // For minimal CPU use, overestimate delay by <= 1 Ms
                    delayMs = static_cast<Uint32>(delayS * 1000.0f + 1);
                }
                SDL_Delay(delayMs);
                tick = Clock::now();
                deltaTime = std::chrono::duration_cast<FpSeconds>(tick - lastTick).count();
            }
            lastTick = tick;
        }
#endif
    }

    void SDLRenderer::drawFrame() {
        // The processEvents call should be removed if possible. Processing the
        // events is currently necessary because the "up" stroke of the "Enter"
        // key needs to be captured after the user has initiated a long
        // calculation from ImGui::InputText (if this is not done, the
        // ImGui::InputText function will continue to think that the Enter key
        // is still down and continue to return true, causing a large number
        // of "Enter" strokes to get registered in the command log)).
        processEvents();
        frameUpdate(0);
        frameRender();
        frameNumber++;
        r->swapWindow();
    }

    int SDLRenderer::getFrameNumber() {
        return frameNumber;
    }

    void SDLRenderer::startEventLoop(std::shared_ptr<VR> vr) {
        if (!window){
            LOG_INFO("SDLRenderer::init() not called");
        }

        running = true;

        typedef std::chrono::high_resolution_clock Clock;
        using FpSeconds = std::chrono::duration<float, std::chrono::seconds::period>;
        auto lastTick = Clock::now();
        float deltaTime = 0;

        while (running){
            vr->render();
            frame(deltaTime);

            auto tick = Clock::now();
            deltaTime = std::chrono::duration_cast<FpSeconds>(tick - lastTick).count();
            lastTick = tick;
        }
    }

    void SDLRenderer::setWindowSize(glm::ivec2 size) {
        int width = size.x;
        int height = size.y;
        windowWidth = width;
        windowHeight = height;
        if (window!= nullptr){
            SDL_SetWindowSize(window, width, height);
        }
    }

    void SDLRenderer::setWindowTitle(std::string title) {
        windowTitle = title;
        if (window != nullptr) {
            SDL_SetWindowTitle(window, title.c_str());
        }
    }

    SDL_Window *SDLRenderer::getSDLWindow() {
        return window;
    }

    void SDLRenderer::setFullscreen(bool enabled) {
#ifndef EMSCRIPTEN
        if (isFullscreen() != enabled){
            Uint32 flags = (SDL_GetWindowFlags(window) ^ SDL_WINDOW_FULLSCREEN_DESKTOP);
            if (SDL_SetWindowFullscreen(window, flags) < 0) // NOTE: this takes FLAGS as the second param, NOT true/false!
            {
                std::cout << "Toggling fullscreen mode failed: " << SDL_GetError() << std::endl;
                return;
            }
        }
#endif
    }

    bool SDLRenderer::isFullscreen() {
        return ((SDL_GetWindowFlags(window)&(SDL_WINDOW_FULLSCREEN|SDL_WINDOW_FULLSCREEN_DESKTOP)) != 0);
    }

    void SDLRenderer::setMouseCursorVisible(bool enabled) {
        SDL_ShowCursor(enabled?SDL_ENABLE:SDL_DISABLE);
    }

    bool SDLRenderer::isMouseCursorVisible() {
        return SDL_ShowCursor(SDL_QUERY)==SDL_ENABLE;
    }

    bool SDLRenderer::setMouseCursorLocked(bool enabled) {
        if (enabled){
            setMouseCursorVisible(false);
        }
        return SDL_SetRelativeMouseMode(enabled?SDL_TRUE:SDL_FALSE) == 0;
    }

    bool SDLRenderer::isMouseCursorLocked() {
        return SDL_GetRelativeMouseMode() == SDL_TRUE;
    }

    SDLRenderer::InitBuilder SDLRenderer::init() {
        return SDLRenderer::InitBuilder(this);
    }

    glm::vec3 SDLRenderer::getLastFrameStats() {
        return {
                deltaTimeEvent,deltaTimeUpdate,deltaTimeRender
        };
    }

    void SDLRenderer::SetArrowCursor() {
        SDL_FreeCursor(cursor);
        cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
        cursorType = Cursor::Arrow;
        SDL_SetCursor(cursor);
    }

    void SDLRenderer::Begin(Cursor cursorStart) {
        if (cursor != NULL) {
            if (cursorType != Cursor::Arrow)
                LOG_ERROR("Last mouse cursor not freed in SDLRenderer::Begin");
            SDL_FreeCursor(cursor);
        }
        switch (cursorStart) {
            case Cursor::Arrow:
                cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
                break;
            case Cursor::Wait:
                cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
                break;
            case Cursor::Hand:
                cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
                break;
            case Cursor::SizeAll:
                cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
                break;
            default:
                LOG_ERROR("Invalid mouse cursor passed to SDLRenderer::Begin");
                return;
        }
        cursorType = cursorStart;
        SDL_SetCursor(cursor);
    }

    void SDLRenderer::End(Cursor cursorEnd) {
        if (cursorEnd != cursorType && cursorType != Cursor::Arrow)
            LOG_ERROR("Ending cursor not same as starting cursor in SDLRenderer");
        SetArrowCursor();
    }

    void SDLRenderer::SetMinimalRendering(bool minimalRendering) {
        this->minimalRendering = minimalRendering;
    }

    void SDLRenderer::SetAppUpdated(bool appUpdated) {
        this->appUpdated = appUpdated;
    }

    bool SDLRenderer::parseMainArgumentsForEventProcessing(
                          std::string programName,
                          int argc, char* argv[],
                          bool& recordEvents, bool& playEvents,
                          std::string& eventsFileName,
                          uint32_t& sdlWindowFlags,
                          glm::ivec2& appWindowSize) {           
        int success = true;

        // Get and process arguments passed in to the executable

        // Pass a string of options to getopt:
        //   The colon after a letter signifies that the option expects an
        //   argument. The leading colon lets you distinguish between invalid
        //   option and missing argument cases
    
        int option;
        bool isHidden = false;
        while(argc != 1 && (option = getopt(argc, argv, ":hr:p:x:y:c")) != -1) {
            switch(option) {
            case 'r':
                if (!playEvents) {
                    // optarg contains the argument for the option
                    eventsFileName = optarg;
                    recordEvents = true;
                } else {
                    std::cout << "Error: cannot simultaneously playback and"
                              << " record events -- choose either option -r"
                              << " *or* -p" << std::endl;
                    return success = false;
                }
                break;
            case 'p':
                if (!recordEvents) {
                    eventsFileName = optarg;
                    playEvents = true;
                } else {
                    std::cout << "Error: cannot simultaneously playback and"
                              << " record events -- choose either option -r"
                              << " *or* -p" << std::endl;
                    return success = false;
                }
                break;
            case 'c':
                if (playEvents) {
                    isHidden = true;
                } else {
                    std::cout << "Error: cannot select the -c option without"
                              << " first selecting the -p option." << std::endl;
                    return success = false;
                }
                break;
            case 'x':
                appWindowSize.x = atoi(optarg);
                if (appWindowSize.x < 0) {
                    std::cout << "Error: cannot specify a negative value for x"
                              << std::endl;
                    return success = false;
                }
                break;
            case 'y':
                appWindowSize.y = atoi(optarg);
                if (appWindowSize.y < 0) {
                    std::cout << "Error: cannot specify a negative value for y"
                              << std::endl;
                    return success = false;
                }
                break;
            case 'h':   // help
                printf("usage: %s [ -r filename <or> -p filename ][-c][-x pixels][-y pixels]\n",
                       programName.c_str());
                printf("where\n");
                printf("    [-r filename] record events to filename\n");
                printf("or\n");
                printf("    [-p filename] playback events from filename\n");
                printf("and\n");
                printf("    [-c] hide application while running, which can only be used together with\n");
                printf("         the -p option. The default is a visible, resizable window.\n");
                printf("and\n");
                printf("    [-x pixels_in_x_direction] set application window width in pixels\n");
                printf("    [-y pixels_in_y_direction] set application window height in pixels\n");
                return success = false;
            case ':':
                // missing option argument
                // optopt contains the option
                // argv[0] is the name of the program
                fprintf(stderr, "%s: option '-%c' requires an argument\n",
                        argv[0],   optopt);
                printf("usage: %s [ -r filename <or> -p filename ][-c][-x pixels][-y pixels]\n",
                       programName.c_str());
                return success = false;
            case '?':   // getopt default invalid option
            default:
                fprintf(stderr, "Illegal option '-%c\n", optopt);
                printf("usage: %s [ -r filename <or> -p filename ][-c][-x pixels][-y pixels]\n",
                       programName.c_str());
                return success = false;
            }
        }
        if (isHidden) {
            sdlWindowFlags = sdlWindowFlags | SDL_WINDOW_HIDDEN;
        } else {
            sdlWindowFlags = sdlWindowFlags | SDL_WINDOW_RESIZABLE;
        }

        return success = true;
    }

    bool SDLRenderer::setupEventRecorder(bool& recordingEvents,
                                         bool& playingEvents,
                                         const std::string& eventsFileName,
                                         std::string& errorMessage) {
        bool success = true;
        assert(!(recordingEvents && playingEvents));
        assert(!(m_recordingEvents && m_playingBackEvents));
        if (playingEvents && !m_playingBackEvents) {
            if (m_recordingEvents) {
                errorMessage = "Attempted to play events while recording";
                playingEvents = false;
                return success = false;
            }
            if (!readRecordedEvents(eventsFileName, errorMessage)) {
                playingEvents = false;
                return success = false;
            }
        } else if (recordingEvents && !m_recordingEvents) {
            if (m_recordingEvents) {
                errorMessage = "Attempted to record events while already recording";
                recordingEvents = false;
                return success = false;
            }
            if (m_playingBackEvents) {
                errorMessage = "Attempted to record events while playing";
                recordingEvents = false;
                return success = false;
            }
            m_recordingFileName = eventsFileName;
            // Test whether file can be opened for writing
            std::ofstream outFile(m_recordingFileName, std::ios::out);
            if(!outFile) {
                std::stringstream errorStream;
                errorStream << "File '" << m_recordingFileName
                    << "' could not be opened for writing." << std::endl;
                errorMessage = errorStream.str();
                recordingEvents = false;
                success = false;
            } else {
                outFile.close();
            }
            std::stringstream().swap(m_recordingStream);
        }
        return success;
    }

    bool SDLRenderer::startEventRecorder(bool& recordingEvents,
                                         bool& playingEvents,
                                         const std::string& eventsFileName,
                                         std::string& errorMessage) {
        bool success = true;
        if (setupEventRecorder(recordingEvents, playingEvents, eventsFileName,
                               errorMessage)) {
            if (recordingEvents) {
                startRecordingEvents();
            } else if (playingEvents) {
                startPlayingEvents();
            }
        } else {
            success = false;
        }
        return success;
    }

    void SDLRenderer::startRecordingEvents() {
        m_recordingEvents = true;
    }

    void SDLRenderer::recordFrame() {
        int x, y;
        m_recordingStream << frameNumber << " "
                          << getMouseState(&x, &y) << " "
                          << x << " " << y << " "
                          << getKeymodState() << " "
                          << "#no event"
                          << std::endl;
    }

    // Record SDL events (mouse, keyboard, etc.) to "m_recordingStream" member
    // variable and write to file in ::stopRecordingEvents(). Can read later to
    // play back events, which enables testing scripts for ImGui interface
    void SDLRenderer::recordEvent(const SDL_Event& e) {
        // SDL Events  are defined in $SDL_HOME/include/SDL_events.h
        //   typedef union SDL_Event ...
        // SDL_Keysym is defined in $SDL_HOME/include/SDL_keyboard.h
        //   typedef struct SDL_Keysym ...
        // SDL_Scancode is defined in $SDL_HOME/include/SDL_scancode.h
        //   typedef enum { SDL_SCANCODE_UNKNOWN = 0, ... } SDL_Scancode;
        //   i.e. integer (same as enums)
        // SDL_Keycode is defined in $SDL_HOME/include/SDL_keycode.h
        //   typedef Sint32 SDL_Keycode [i.e. signed (standard) int (integer)]
        //   typedef enum {KMOD_NONE=0x0000, KMOD_LSHIF=0x0001, ...} SDL_Keymod
        //     [which is returned by SDL_GetModState()] - state of 'mod' keys
        // SDL types (like Uint8) are defined in $SDL_HOME/include/SDL_stdinc.h
        //   typedef uint8_t Uint8 // An unsigned 8-bit integer, type = 1 byte
        //   typedef uint16_t Uint16 // An unsigned 16-bit integer
        //   typedef int32_t Sint32; // A signed 32-bit integer
        // In C, uint8_t is defined in header stdint.h, guaranteed to be 8 bits
        //   typedef unsigned char uint8_t;
        //   typedef unsigned short uint16_t;
        //   typedef unsigned long uint32_t;
        //   typedef unsigned long long uint64_t;
        //   typedef int int32_t;
        // To properly print an unsigned char that is used to store bits (e.g.
        // that is not being used to store characters), you need to "promote"
        // the value using the bit operation "+" (needed for all 'Uint8', i.e.
        // unsigned, 8-bit integer) variables in the event structs). An
        // alternative would be to print in hexadecimal format (example below).
        // See discussion in StackOverflow:
        // [https://]
        // stackoverflow.com/questions/15585267/cout-not-printing-unsigned-char
        // This is the example for printing in hexadecimal format:
        // std::cout << std::showbase // show the 0x prefix
        //      << std::internal      // fill between the prefix and the number
        //      << std::setfill('0'); // fill with 0s
        // std::cout << std::hex << std::setw(4) << value

        // SDL provides the SDL_GetMouseState and SDL_GetModState functions that
        // a user can call at any time -- the information to support this call
        // during playback is stored right after the frame number for all events.
        // Note that sometimes the values returned by SDL_GetMouseState are
        // different than what is provided by the various mouse events. Storing
        // the mouse state separately allows playback to provide exactly what the
        // code experienced during recording of events.
        int x, y;
        m_recordingStream << frameNumber << " "
            << getMouseState(&x, &y) << " "
            << x << " " << y << " "
            << getKeymodState() << " ";
        switch (e.type) {
            case SDL_QUIT:
                m_recordingStream
                    << e.quit.type << " "
                    << e.quit.timestamp << " "
                    << "#quit (end program)"
                    << std::endl;
                break;
            case SDL_TEXTINPUT:
                if (!m_pauseRecordingOfTextEvents) {
                    m_recordingStream
                        << e.text.type << " "
                        << e.text.timestamp << " "
                        << e.text.windowID << " "
                        << "\"" << e.text.text << "\"" << " "
                        << "#text "
                        << e.text.text
                        << std::endl;
                }
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                m_recordingStream
                    << e.key.type << " "
                    << e.key.timestamp << " "
                    << e.key.windowID << " "
                    // Promote 8-bit integers (see notes above)
                    << +e.key.state << " "
                    << +e.key.repeat << " "
                    << +e.key.padding2 << " "
                    << +e.key.padding3 << " "
                    << e.key.keysym.scancode << " "
                    << e.key.keysym.sym << " "
                    << +e.key.keysym.mod << " "
                    << "#key "
                    << (e.key.state == SDL_PRESSED ? "pressed" : "released")
                    << " '" << char(e.key.keysym.sym) << "'"
                    << std::endl;
                break;
            case SDL_MOUSEMOTION:
                m_recordingStream
                    << e.motion.type << " "
                    << e.motion.timestamp << " "
                    << e.motion.windowID << " "
                    << e.motion.which << " "
                    << e.motion.state << " "
                    << e.motion.x << " "
                    << e.motion.y << " "
                    << e.motion.xrel << " "
                    << e.motion.yrel << " "
                    << "#motion ("
                    << (e.motion.state == SDL_PRESSED ? "pressed" : "released")
                    << ")"
                    << std::endl;
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                m_recordingStream
                    << e.button.type << " "
                    << e.button.timestamp << " "
                    << e.button.windowID << " "
                    << e.button.which << " "
                    // Promote 8-bit integers (see notes above)
                    << +e.button.button << " "
                    << +e.button.state << " "
                    << +e.button.clicks << " "
                    << +e.button.padding1 << " "
                    << e.button.x << " "
                    << e.button.y << " "
                    << "#button "
                    << (e.button.state == SDL_PRESSED ? "pressed" : "released")
                    << std::endl;
                break;
            case SDL_MOUSEWHEEL:
                m_recordingStream
                    << e.wheel.type << " "
                    << e.wheel.timestamp << " "
                    << e.wheel.windowID << " "
                    << e.wheel.which << " "
                    << e.wheel.x << " "
                    << e.wheel.y << " "
                    << e.wheel.direction << " "
                    << "#wheel"
                    << std::endl;
                break;
            case SDL_CONTROLLERAXISMOTION:
            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP:
            case SDL_CONTROLLERDEVICEADDED:
            case SDL_CONTROLLERDEVICEREMOVED:
            case SDL_CONTROLLERDEVICEREMAPPED:
                m_recordingStream
                    << "#Controller event NOT RECORDED"
                    << std::endl;
                LOG_ERROR("Controller 'record event' called but not processed");
                break;
            case SDL_JOYAXISMOTION:
            case SDL_JOYBALLMOTION:
            case SDL_JOYHATMOTION:
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
            case SDL_JOYDEVICEADDED:
            case SDL_JOYDEVICEREMOVED:
                m_recordingStream
                    << "#Joystick event NOT RECORDED"
                    << std::endl;
                LOG_ERROR("Joystick 'record event' called but not processed");
                break;
            case SDL_FINGERDOWN:
            case SDL_FINGERUP:
            case SDL_FINGERMOTION:
                m_recordingStream
                    << e.tfinger.type << " "
                    << e.tfinger.timestamp << " "
                    << e.tfinger.touchId << " "
                    << e.tfinger.fingerId << " "
                    << e.tfinger.x << " "
                    << e.tfinger.y << " "
                    << e.tfinger.dx << " "
                    << e.tfinger.dy << " "
                    << e.tfinger.pressure << " "
//                    << e.tfinger.windowID << " "
                    << "#tfinger"
                    << std::endl;
                break;
            default:
                // Record all events (even empty and "non-registered" events)
                m_recordingStream
                    << "#no event"
                    << std::endl;
                break;
        }
    }

    SDL_Event SDLRenderer::getNextRecordedEvent(bool& endOfFile) {
        SDL_Event e;
        e.type = 0;
        endOfFile = false;
        int nextFrameDummy;
        std::string eventLineString;
        bool commentedLine = true;
        while (commentedLine) {
            std::getline(m_playbackStream, eventLineString);
            if (!m_playbackStream || eventLineString[0] != '#') {
                commentedLine = false;
            }
        }
        std::istringstream eventLine(eventLineString);
        if (!m_playbackStream || !eventLine) {
            if (m_playbackStream.eof()) {
                endOfFile = true;
            } else {
                LOG_ERROR("Error getting line from m_playbackStream");
            }
            return e;
        }
        // The value for nextFrame is only used for nextRecordedFramePeek(),
        // but we need to advance the stream, so store in dummy variable
        eventLine >> nextFrameDummy;
        if (!eventLine) {
            LOG_ERROR("Error getting frame number from m_playbackStream");
            return e;
        }
        // See comments about SDL_GetMouseState in SDLRenderer::recordEvent
        eventLine >> m_playbackMouseState
                  >> m_playbackMouse_x >> m_playbackMouse_y;
        if (!eventLine) {
            LOG_ERROR("Error getting mouse information from m_playbackStream");
            return e;
        }
        int keymodState;
        eventLine >> keymodState;
        m_playbackKeymodState = static_cast<SDL_Keymod>(keymodState);
        if (!eventLine) {
            LOG_ERROR("Error getting key mod state from m_playbackStream");
            return e;
        }
        eventLine >> e.type;
        if (!eventLine) {
            // No event associated with frame
            SDL_Event emptyEvent;
            emptyEvent.type = 0;
            return emptyEvent;
        }
        std::string textInput;
        switch (e.type) {
            case SDL_QUIT:
                eventLine
                    >> e.quit.timestamp;
                break;
            case SDL_TEXTINPUT:
                eventLine
                    >> e.text.timestamp
                    >> e.text.windowID
                    >> std::quoted(textInput);
                if (textInput.length() <= SDL_TEXTINPUTEVENT_TEXT_SIZE) {
                    strcpy(e.text.text, textInput.c_str());
                } else {
                    LOG_ERROR("Text from playback stream too long for SDL");
                }
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                int keyState, repeat, padding2, padding3, scancode, sym, mod;
                eventLine
                    >> e.key.timestamp
                    >> e.key.windowID
                    >> keyState
                    >> repeat
                    >> padding2
                    >> padding3
                    >> scancode
                    >> sym
                    >> mod;
                // stringstream will not correctly read into 8-bit integers
                // and SDL types. Hence, read ints and cast to the variables
                e.key.state = static_cast<Uint8>(keyState);
                e.key.repeat = static_cast<Uint8>(repeat);
                e.key.padding2 = static_cast<Uint8>(padding2);
                e.key.padding3 = static_cast<Uint8>(padding3);
                e.key.keysym.scancode = static_cast<SDL_Scancode>(scancode);
                e.key.keysym.sym = static_cast<SDL_Keycode>(sym);
                e.key.keysym.mod = static_cast<Uint16>(mod);
                break;
            case SDL_MOUSEMOTION:
                eventLine
                    >> e.motion.timestamp
                    >> e.motion.windowID
                    >> e.motion.which
                    >> e.motion.state
                    >> e.motion.x
                    >> e.motion.y
                    >> e.motion.xrel
                    >> e.motion.yrel;
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                int button, buttonState, clicks, padding1;
                eventLine
                    >> e.button.timestamp
                    >> e.button.windowID
                    >> e.button.which
                    >> button
                    >> buttonState
                    >> clicks
                    >> padding1
                    >> e.button.x
                    >> e.button.y;
                // 8-bit variables need to be explicitly cast (see note above)
                e.button.button = static_cast<Uint8>(button);
                e.button.state = static_cast<Uint8>(buttonState);
                e.button.clicks = static_cast<Uint8>(clicks);
                e.button.padding1 = static_cast<Uint8>(padding1);
                break;
            case SDL_MOUSEWHEEL:
                eventLine
                    >> e.wheel.timestamp
                    >> e.wheel.windowID
                    >> e.wheel.which
                    >> e.wheel.x
                    >> e.wheel.y
                    >> e.wheel.direction;
                break;
            case SDL_CONTROLLERAXISMOTION:
            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP:
            case SDL_CONTROLLERDEVICEADDED:
            case SDL_CONTROLLERDEVICEREMOVED:
            case SDL_CONTROLLERDEVICEREMAPPED:
                LOG_ERROR("Controller event in m_playbackStream not processed");
                break;
            case SDL_JOYAXISMOTION:
            case SDL_JOYBALLMOTION:
            case SDL_JOYHATMOTION:
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
            case SDL_JOYDEVICEADDED:
            case SDL_JOYDEVICEREMOVED:
                LOG_ERROR("Joystick event in m_playbackStream not processed");
                break;
            case SDL_FINGERDOWN:
            case SDL_FINGERUP:
            case SDL_FINGERMOTION:
                eventLine
                    >> e.tfinger.timestamp
                    >> e.tfinger.touchId
                    >> e.tfinger.fingerId
                    >> e.tfinger.x
                    >> e.tfinger.y
                    >> e.tfinger.dx
                    >> e.tfinger.dy
                    >> e.tfinger.pressure
//                    >> e.tfinger.windowID
                    ;
                break;
            default:
                LOG_ERROR("Encountered unknown event in m_playbackStream");
                break;
        }
        if (!eventLine) {
            // Log this error, but continue on. Because event playback is
            // intended to be a developer feature for testing, minimal time has
            // been invested in productizing error checking (for event playback)
            LOG_ERROR("Error reading event from m_playbackStream");
        }
        return e;
    }

    void SDLRenderer::setPauseRecordingOfTextEvents(const bool pause) {
        m_pauseRecordingOfTextEvents = pause;
    }

    bool SDLRenderer::stopRecordingEvents(std::string& errorMessage) {
        bool success = true;
        if (!m_recordingEvents) {
            errorMessage = "Not recording, but stopRecordingEvents called";
            return success = false;
        }
        std::ofstream outFile(m_recordingFileName, std::ios::out);
        if(outFile) {
            // Write out file header
            outFile << "# File containing imgui.ini file and recorded SDL"
                    << " events for playback"
                    << std::endl;
            outFile << "#" << std::endl;
            size_t imGuiSize;
            const char * imGuiStr = ImGui::SaveIniSettingsToMemory(&imGuiSize);
            outFile << "# imgui.ini size:" << std::endl;
            outFile << imGuiSize << std::endl;
            outFile << "# imgui.ini file:" << std::endl;
            outFile << imGuiStr;
            outFile << "# Recorded SDL events:" << std::endl
                    << "# Format: frame_number mouse_state mx my keymod_state"
                    << " event_data #comment" << std::endl;
            // Write out recorded events
            outFile << m_recordingStream.str();
            // Close file and clear stream
            outFile.close();
            std::stringstream().swap(m_recordingStream);
        } else {
            std::stringstream errorStream;
            errorStream << "File '" << m_recordingFileName
                << "' could not be opened for writing." << std::endl;
            errorMessage = errorStream.str();
            success = false;
        }
        m_recordingEvents = false;
        return success;
    }

    bool SDLRenderer::recordingEvents() {
        return m_recordingEvents;
    }

    void SDLRenderer::startPlayingEvents() {
        m_playingBackEvents = true;
    }

    bool SDLRenderer::readRecordedEvents(const std::string& fileName,
                                         std::string& errorMessage) {
        bool success = true;
        if (m_recordingEvents) {
            errorMessage = "Cannot read a recording while recording events";
            return success = false;
        }

        std::ifstream inFile(fileName, std::ios::in);
        if(inFile) {
            // Read Imgui character stream size
            size_t imGuiSize;
            const char * imGuiStr;
            bool endOfFile = false;
            std::string fileLineString;
            bool commentedLine = true;
            while (commentedLine) {
                std::getline(inFile, fileLineString);
                if (!inFile || fileLineString[0] != '#') {
                    commentedLine = false;
                }
            }

            std::istringstream fileLine(fileLineString);
            if (!inFile || !fileLine) {
                if (inFile.eof()) {
                    endOfFile = true;
                    errorMessage = "Events file is empty";
                } else {
                    errorMessage = "Error reading first line from events file";
                }
                return success = false;
            }
            fileLine >> imGuiSize;
            if (!fileLine) {
                errorMessage = "Error getting imgui.ini file size from events file";
                return success = false;
            }
            std::getline(inFile, fileLineString);
            if (!inFile || fileLineString[0] != '#') {
                errorMessage = "Expected '#' after reading imgui.ini file size from events file";
                return success = false;
            }
            // Read the imgui.ini character stream
            char c;
            std::stringstream imGuiStream;
            for (int i = 0; i < imGuiSize; i++) {
                if (inFile.get(c)) {
                    imGuiStream << c;
                } else {
                    errorMessage = "Error reading imgui.ini file from events file";
                    return success = false;
                }
            } 
            // The c_str from std::stringstream deallocates after statement
            // So, store in a const temporary that will exist while in scope
            const std::string& imGuiString = imGuiStream.str();
            // Load the imgui.ini character stream into ImGui
            ImGui::LoadIniSettingsFromMemory(imGuiString.c_str(), imGuiSize);

            // Should make a `nextCharPeek()` function to return next char
            // but put it back. If it is a comment, then read the line and
            // discard it. This could be called within the while loop below, and
            // will enable getting rid of the `while (commentedLine)` loop near
            // the top of the function. This will also enable being able to put
            // a comment anywhere in the code.

            // Read the rest of file into the 'm_playbackStream' member variable
            std::stringstream().swap(m_playbackStream);
            while (inFile.get(c)) {
                m_playbackStream << c;
            }
            inFile.close();
        } else {
            std::stringstream errorStream;
            errorStream << "File '" << fileName << "' could not be opened."
                        << std::endl;
            errorMessage = errorStream.str();
            success = false;
        }
        return success;
    }

    void SDLRenderer::setPausePlayingEvents(const bool pause) {
        m_pausePlaybackOfEvents = pause;
    }

    bool SDLRenderer::playingEvents() {
        return m_playingBackEvents;
    }

    bool SDLRenderer::pushRecordedEventsForNextFrameToSDL() {
        // Push events in m_playbackStream associated with next frame to SDL
        // event queue. Report any errors, but continue to push events.
        bool success = true;
        bool endOfFile = false;
        SDL_Event event;
        int nextFrame = nextRecordedFramePeek();
        m_playbackFrame = nextFrame;
        while (nextFrame == m_playbackFrame && !endOfFile) {
            event = getNextRecordedEvent(endOfFile);
            if (!isWindowHidden) {
                SDL_WarpMouseInWindow(window, m_playbackMouse_x, m_playbackMouse_y);
            }
            if (!endOfFile && SDL_PushEvent(&event) != 1) {
                LOG_ERROR("Error pushing event to queue");
                LOG_ERROR(SDL_GetError());
                SDL_ClearError();
                success = false;
            }
            nextFrame = nextRecordedFramePeek();
        }
        if (endOfFile) {
            m_playingBackEvents = false;
        } else {
            m_playbackFrame = nextFrame;
        }
        return success;
    }

    int SDLRenderer::nextRecordedFramePeek() {
        char c;
        bool getNextChar = true;
        std::vector<char> digits;
        while (m_playbackStream && getNextChar && m_playbackStream.get(c)) {
            if (isdigit(c)) {
                digits.push_back(c);
            } else {
                getNextChar = false;
                m_playbackStream.unget();
            }
        }
        std::stringstream numberString;
        for (int i = 0; i < digits.size(); i++) {
            numberString << digits[i];
            m_playbackStream.unget();
        }
        int nextFrame;
        if (!(numberString >> nextFrame)) {
            nextFrame = -99;
        }
        return nextFrame;
    }

    void SDLRenderer::captureFrame(RenderPass * renderPass, bool captureFromScreen) {
        int i = m_image.size();
        m_imageDimensions.push_back(renderPass->frameSize());
        m_image.push_back(renderPass->readRawPixels(0, 0, m_imageDimensions[i].x,
                                       m_imageDimensions[i].y, captureFromScreen));
    }

    int SDLRenderer::numCapturedImages() {
        return m_image.size();
    }

    void SDLRenderer::writeCapturedImages(std::string fileName) {
        if (m_writingImages) {
            return;
        }
        m_writingImages = true;
        stbi_flip_vertically_on_write(true);
   
        assert(m_image.size() == m_imageDimensions.size());
        if (m_image.size() > 0) {
            std::cout << "Writing images to filesystem..." << std::endl;
        }
        for (int i = 0; i < m_image.size(); i++) {
            // Keep ImGui responsive during write (process events & draw)
            SDLRenderer::instance->drawFrame();

            std::stringstream imageFileName;
            imageFileName << fileName << i+1 << ".png"; // Start numbering at 1

            int stride = Color::numChannels() * m_imageDimensions[i].x;
            stbi_write_png(imageFileName.str().c_str(),
                            m_imageDimensions[i].x, m_imageDimensions[i].y,
                            Color::numChannels(), m_image[i].data(), stride);
        }

        m_writingImages = false;
    }

    // Intercept calls to SDL_GetMouseState for Dear ImGui during playback of
    // recorded events. See comments about SDL_GetMouseState in ::recordEvent
    Uint32 SDLRenderer::getMouseState(int* x, int* y) {
        Uint32 mouseState;
        if (m_playingBackEvents) {
            if (x != nullptr && y != nullptr) {
                *x = m_playbackMouse_x;
                *y = m_playbackMouse_y;
            }
            mouseState = m_playbackMouseState;
        } else {
            mouseState = SDL_GetMouseState(x, y);
        }
        return mouseState;
    }

    // Intercept calls to SDL_GetModState for Dear ImGui during playback of
    // recorded events. See comments about SDL_GetModState in ::recordEvent
    SDL_Keymod SDLRenderer::getKeymodState() {
        SDL_Keymod keymodState;
        if (m_playingBackEvents) {
            keymodState = m_playbackKeymodState;
        } else {
            keymodState = SDL_GetModState();
        }
        return keymodState;
    }

    void
    SDLRenderer::addKeyPressed(SDL_Keycode keyCode) {
        if (!isKeyPressed(keyCode)) {
            keyPressed.push_back(keyCode);
        }
    }

    void
    SDLRenderer::removeKeyPressed(SDL_Keycode keyCode) {
        auto &v = keyPressed;
        // Use the the "erase-remove idiom" enabled by std::algorithm
        v.erase(std::remove(v.begin(), v.end(), keyCode), v.end());
    }

    bool
    SDLRenderer::isKeyPressed(SDL_Keycode keyCode) {
        for (auto key : keyPressed) {
            if (key == keyCode) {
                return true;
            }
        }
        return false;
    }

    bool
    SDLRenderer::isAnyKeyPressed() {
        if (keyPressed.size() > 0) {
            return true;
        }
        return false;
    }

    SDLRenderer::InitBuilder::~InitBuilder() {
        build();
    }

    SDLRenderer::InitBuilder::InitBuilder(SDLRenderer *sdlRenderer)
            :sdlRenderer(sdlRenderer) {
    }

    SDLRenderer::InitBuilder &SDLRenderer::InitBuilder::withSdlInitFlags(uint32_t sdlInitFlag) {
        this->sdlInitFlag = sdlInitFlag;
        return *this;
    }

    SDLRenderer::InitBuilder &SDLRenderer::InitBuilder::withSdlWindowFlags(uint32_t sdlWindowFlags) {
        this->sdlWindowFlags = sdlWindowFlags;
        return *this;
    }

    SDLRenderer::InitBuilder &SDLRenderer::InitBuilder::withVSync(bool vsync) {
        this->vsync = vsync;
        return *this;
    }

    SDLRenderer::InitBuilder &SDLRenderer::InitBuilder::withGLVersion(int majorVersion, int minorVersion) {
        this->glMajorVersion = majorVersion;
        this->glMinorVersion = minorVersion;
        return *this;
    }

    void SDLRenderer::InitBuilder::build() {
        if (sdlRenderer->running){
            return;
        }
        if (!sdlRenderer->window){
#ifdef EMSCRIPTEN
            SDL_Renderer *renderer = nullptr;
            SDL_CreateWindowAndRenderer(sdlRenderer->windowWidth, sdlRenderer->windowHeight, SDL_WINDOW_OPENGL, &sdlRenderer->window, &renderer);
#else
            SDL_Init( sdlInitFlag  );
            SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
            SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
            SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, glMajorVersion);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, glMinorVersion);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef SRE_DEBUG_CONTEXT
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif
            sdlRenderer->window = SDL_CreateWindow(sdlRenderer->windowTitle.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, sdlRenderer->windowWidth, sdlRenderer->windowHeight,sdlWindowFlags);
#endif
            sdlRenderer->r = new Renderer(sdlRenderer->window, vsync, maxSceneLights);

            sdlRenderer->SetMinimalRendering(minimalRendering);
            sdlRenderer->isWindowHidden = (SDL_GetWindowFlags(sdlRenderer->window) & SDL_WINDOW_HIDDEN);

#ifdef SRE_DEBUG_CONTEXT
            if (glDebugMessageCallback) {
                LOG_INFO("Register OpenGL debug callback ");

                std::cout << "Register OpenGL debug callback " << std::endl;
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
                glDebugMessageCallback(openglCallbackFunction, nullptr);
                GLuint unusedIds = 0;
                glDebugMessageControl(GL_DONT_CARE,
                    GL_DONT_CARE,
                    GL_DONT_CARE,
                    0,
                    &unusedIds,
                    true);

            }
#endif
        }
    }

    SDLRenderer::InitBuilder &SDLRenderer::InitBuilder::withMaxSceneLights(int maxSceneLights) {
        this->maxSceneLights = maxSceneLights;
        return *this;
    }

    SDLRenderer::InitBuilder &SDLRenderer::InitBuilder::withMinimalRendering(bool minimalRendering) {
        this->minimalRendering = minimalRendering;
        return *this;
    }

    void SDLRenderer::setWindowIcon(std::shared_ptr<Texture> tex){
        auto texRaw = tex->getRawImage();
        auto surface = SDL_CreateRGBSurfaceFrom(texRaw.data(),tex->getWidth(),tex->getHeight(),32,tex->getWidth()*4,0x00ff0000,0x0000ff00,0x000000ff,0xff000000);

        // The icon is attached to the window pointer
        SDL_SetWindowIcon(window, surface);

        // ...and the surface containing the icon pixel data is no longer required.
        SDL_FreeSurface(surface);

    }
}
