#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <cstdint>
#include <thread>
#include <mutex>
#include "main_test.h"

#include "extern/opl_dosbox/opl.h"
#include "extern/imgui/imgui.h"
#include "extern/imgui/examples/imgui_impl_sdl.h"
#include "extern/imgui/examples/imgui_impl_opengl2.h"

#define AMPLITUDE       28000
#define SAMPLE_RATE     44100
//#define SAMPLE_RATE     49716
#define RADIANS(x)      (2.0f * M_PI * (x))

static std::mutex audio_mutex;
static OPLChipClass opl(SAMPLE_RATE);

static SDL_Window *window = nullptr;
static SDL_GLContext gl_context = nullptr;

static std::thread play_thread;

void adlib_out(unsigned short addr, unsigned char val)
{
    std::lock_guard<std::mutex> lock(audio_mutex);

    printf("%X -> %X\n", addr, val);
    opl.adlib_write(addr, val, 0);
}

static void audio_callback(void *data, uint8_t *raw_buf, int bytes)
{
    std::lock_guard<std::mutex> lock(audio_mutex);

    OPLChipClass *opl = (OPLChipClass *)data;
    opl->adlib_getsample((Bit16s *)raw_buf, bytes / 2);
}

static void sdl_init()
{
    if(0 != SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER)) {
        SDL_Log(":%s", SDL_GetError());
        exit(1);
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    window = SDL_CreateWindow("SynthEd", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync
}

static void ui_init()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL2_Init();
}

static void ui_dockerspace()
{
    ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetWorkPos());
    ImGui::SetNextWindowSize(viewport->GetWorkSize());
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    if(ImGui::BeginMenuBar()) {
        if(ImGui::BeginMenu("File")) {
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    ImGui::End();
}

static void ui_mainloop()
{
    static bool show_demo = false;

    ui_dockerspace();

    ImGui::Begin("Test");
    if(ImGui::Button("Play demo song")) {
        if(play_thread.joinable()) {
            play_thread.join();
        }
        play_thread = std::thread([](){
            test_play();
        });
    }
    ImGui::Checkbox("Show Demo Window", &show_demo);
    if(show_demo) {
        ImGui::ShowDemoWindow(&show_demo);
    }
    ImGui::End();

    test_piano();
    test_editor();
}

int main()
{
    sdl_init();
    ui_init();

    opl.adlib_init(SAMPLE_RATE, true);

    SDL_AudioSpec requested;
    requested.freq = SAMPLE_RATE;
    requested.format = AUDIO_S16SYS;
    requested.channels = 1;
    requested.samples = 2048;
    requested.callback = audio_callback;
    requested.userdata = &opl;

    SDL_AudioSpec got;
    if(0 != SDL_OpenAudio(&requested, &got))
        return 2;
    if(got.format != requested.format)
        return 3;

    SDL_PauseAudio(0); //TODO: do this selectively

    bool running = true;
    while(running) {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                running = false;
        }

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        ui_mainloop();

        ImGuiIO &io = ImGui::GetIO();
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
        glFlush();
        glFinish();
        glClear(GL_COLOR_BUFFER_BIT);

        fflush(stdout); // QtCreatorデバッグ対策
    }

    if(play_thread.joinable()) {
        play_thread.join();
    }

    SDL_PauseAudio(1);
    SDL_CloseAudio();

    return 0;
}
