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
using namespace std::chrono_literals;
using namespace std::chrono;
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
    [[=ImRefl::end_region()]]

    [[=ImRefl::begin_region("std:: types")]]
    std::string string_;
    std::string_view string_view_ = "This is a string_view";
    std::pair<int, float> pair_;
    std::tuple<int, float, bool> tuple_;
    std::optional<int> optional_ = 0;
    std::variant<int, float, bool> variant_;
    std::indirect<int> indirect_ = std::indirect<int>{};
    std::bitset<5> bitset_;
    std::unique_ptr<int> unique_ptr_;
    std::shared_ptr<int> shared_ptr_;
    std::weak_ptr<int> weak_ptr_;
    std::function<void()> function_;
    std::source_location source_location_ = std::source_location::current();
    std::complex<float> complex_;
    std::expected<bool, int> expected_;

    [[=ImRefl::begin_region("Container types")]]
    std::array<int, 5> array_;
    std::span<int, 5> span_ = array_;
    std::vector<int> vector_;
    std::deque<int> deque_;
    std::list<int> list_;
    std::forward_list<int> forward_list_;
    std::set<int> set_;
    std::unordered_set<int> unordered_set_;
    std::multiset<int> multiset_;
    std::unordered_multiset<int> unordered_multiset_;
    std::map<int, float> map_;
    std::unordered_map<int, float> unordered_map_;
    std::multimap<int, float> multimap_;
    std::unordered_multimap<int, float> unordered_multimap_;
    std::inplace_vector<int, 5> inplace_vector_;
    [[=ImRefl::end_region()]]

    [[=ImRefl::begin_region("chrono:: types")]]
    system_clock::time_point time_point_ = std::chrono::system_clock::now();
    duration<double> duration_ = 500ms;
    year_month_day year_month_day_ = 1984y / February / 17;
    hh_mm_ss<decltype(duration_)> hh_mm_ss_{days(3) + hours(4) + minutes(17) + seconds(48)};
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

    int i = 0;
    ex.raw_ptr_ = &i;
    ex.shared_ptr_ = std::make_shared<int>(i);
    *ex.shared_ptr_ = i;
    ex.weak_ptr_ = ex.shared_ptr_;
    ex.unique_ptr_ = std::make_unique<int>(i);

    auto func = []() {
        static size_t n = 0;
        ++n;
        std::println("Function called {} time{}", n, n == 1 ? "" : "s");
    };
    ex.function_ = func;

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

