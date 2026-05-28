#include <array>
#include <climits>
#include <deque>
#include <forward_list>
#include <list>
#include <print>
#include <set>
#include <string>
#include <chrono>
#include <functional>
#include <unordered_set>
#include <map>
#include <vector>
#include <expected>
#include <chrono>
#include <inplace_vector>
#include <memory>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "imrefl.hpp"
#include "imrefl_glm.hpp"

enum class weapon
{
    none,
    sword,
    bow,
    staff,
    wand,
    axe
};

enum colour
{
    green,
    blue,
    red
};

struct player
{
    std::indirect<int> x = std::indirect<int>{5};
    std::map<int, int> data;
};

struct example
{
    enum color
    {
        red,
        orange,
        yellow,
        green,
        blue,
        violet
    };

    enum class shape
    {
        triangle,
        square,
        pentagon,
        hexagon,
        heptagon,
        octagon
    };

    [[=ImRefl::begin_region("Enumeration types")]]
    color enum_;
    shape enum_class_;
    [[=ImRefl::end_region()]]

    [[=ImRefl::begin_region("Arithmetic types")]]
    bool bool_;
    char char_;

    [[=ImRefl::begin_region("Signed integers")]]
    short short_;
    int int_;
    long int l_int_;
    long long int ll_int_;
    [[=ImRefl::end_region()]]

    [[=ImRefl::begin_region("Unsigned integers")]]
    unsigned char u_char_;
    unsigned short u_short_;
    unsigned int u_int_;
    unsigned long int u_l_int_;
    unsigned long long int u_ll_int_;
    [[=ImRefl::end_region()]]

    [[=ImRefl::begin_region("Floating point")]]
    float float_;
    double double_;
    [[=ImRefl::end_region(2)]]

    [[=ImRefl::begin_region("C types")]]
    int* raw_ptr_;
    int* null_raw_ptr_ = nullptr;
    float c_arr_[5];
    [[=ImRefl::string]] char c_char_arr_[32];
    const char* c_str_ = "This is null terminated C string";
};

int main()
{
    glfwSetErrorCallback([](int err, const char* desc) {
        std::println("GLFW error {}: {}", err, desc);
    });

    if (!glfwInit()) {
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "ImRefl Example", nullptr, nullptr);
    if (!window) {
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    //player p = {};
    example ex = {};
    int i = 714;
    ex.raw_ptr_ = &i;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Debug");
        ImRefl::Input("Example", ex);
        ImGui::End();
        ImGui::Render();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

