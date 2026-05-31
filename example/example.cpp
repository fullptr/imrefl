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

int i = 49;

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
    const color const_enum_;
    shape enum_class_;
    const shape const_enum_class_;
    [[=ImRefl::end_region()]]

    [[=ImRefl::begin_region("Arithmetic types")]]
    bool bool_;
    const bool const_bool_ = true;
    char char_;
    const char const_char_ = '!';

    [[=ImRefl::separator("Signed integers")]]
    short short_;
    const short const_short_;
    int int_;
    const int const_int_;
    long int l_int_;
    const long int const_l_int_;
    long long int ll_int_;
    const long long int const_ll_int_;

    [[=ImRefl::separator("Unsigned integers")]]
    unsigned char uchar_;
    const unsigned char const_uchar_;
    unsigned short ushort_;
    const unsigned short const_ushort_;
    unsigned int uint_;
    const unsigned int const_uint_;
    unsigned long int ul_int_;
    const unsigned long int const_ul_int_;
    unsigned long long int ull_int_;
    const decltype(ull_int_) const_ull_int_;

    [[=ImRefl::separator("Floating point")]]
    float float_;
    const float const_float_;
    double double_;
    const double const_double_;
    [[=ImRefl::end_region()]]

    [[=ImRefl::begin_region("C types")]]
    int* raw_ptr_ = &i;
    const int* const_raw_ptr_ = &i;
    int* null_raw_ptr_ = nullptr;
    float c_arr_[5];
    const float const_c_arr_[5] = {0.f, 1.f, 2.f, 3.f, 4.f};
    [[=ImRefl::string]] char c_char_arr_[32];
    const char* c_str_ = "This is null terminated C string";
    [[=ImRefl::end_region()]]

    [[=ImRefl::begin_region("std:: types")]]
    std::string string_;
    const std::string const_string_ = "This is a const-qualified string";
    std::string_view string_view_ = "This is a string_view";
    std::pair<int, float> pair_;
    const decltype(pair_) const_pair_ = {3, 0.14f};
    std::tuple<int, float, bool> tuple_;
    const decltype(tuple_) const_tuple_ = {4, 1.6f, true};
    std::optional<int> optional_ = 0;
    const decltype(optional_) const_optional_ = 8;
    std::variant<int, float, bool> variant_;
    const decltype(variant_) const_variant_ = false;
    std::expected<bool, int> expected_;
    const decltype(expected_) const_expected_ = true;
    std::indirect<int> indirect_ = std::indirect<int>{};
    const decltype(indirect_) const_indirect_ = std::indirect<int>{73};

    [[=ImRefl::separator("Pointer types")]]
    std::unique_ptr<int> unique_ptr_ = std::make_unique<int>(i);
    std::shared_ptr<int> shared_ptr_ = std::make_shared<int>(i);
    std::weak_ptr<int> weak_ptr_ = shared_ptr_;

    [[=ImRefl::separator()]]
    std::function<void()> function_;
    std::source_location source_location_ = std::source_location::current();
    std::complex<float> complex_;
    const decltype(complex_) const_complex_ = {2.45, 6.34};
    std::bitset<5> bitset_;
    const decltype(bitset_) const_bitset_ = 0b10101;

    [[=ImRefl::begin_region("Container types")]]
    std::array<int, 5> array_;
    const decltype(array_) const_array_ = {4, 3, 2, 1, 0};
    std::span<int, 5> span_ = array_;
    std::span<const int, 5> const_span_ = const_array_;
    std::vector<int> vector_;
    const decltype(vector_) const_vector_ = {3, 6, 2, 7};
    std::deque<int> deque_;
    const decltype(deque_) const_deque_ = {7, 4, 9, 6};
    std::list<int> list_;
    const decltype(list_) const_list_ = {67, 9, 47, 2};
    std::forward_list<int> forward_list_;
    const decltype(forward_list_) const_forward_list_ = {76, 854, 234, 8};

    [[=ImRefl::separator("Set types")]]
    std::set<int> set_;
    const decltype(set_) const_set_ = {63, 75, 2, 5};
    std::unordered_set<int> unordered_set_;
    const decltype(unordered_set_) const_unordered_set_ = {34, 745, 9, 3};
    std::multiset<int> multiset_;
    const decltype(multiset_) const_multiset_ = {45, 67, 45, 93};
    std::unordered_multiset<int> unordered_multiset_;
    const decltype(unordered_multiset_) const_unordered_multiset_ = {75, 988, 988, 7};

    [[=ImRefl::separator("Map types")]]
    std::map<int, float> map_;
    const decltype(map_) const_map_ = {{4, 5.7f}, {8, 11.4f}};
    std::unordered_map<int, float> unordered_map_;
    const decltype(unordered_map_) const_unordered_map_ = {{{10, 34.7f}, {5, 17.35f}}};
    std::multimap<int, float> multimap_;
    const decltype(multimap_) const_multimap_ = {{5, 64.4f}, {5, 28.2f}};
    std::unordered_multimap<int, float> unordered_multimap_;
    const decltype(unordered_multimap_) const_unordered_multimap_ = {{19, 0.02f}, {19, 4.254f}};
    std::inplace_vector<int, 5> inplace_vector_;
    const decltype(inplace_vector_) const_inplace_vector_ = {3, 7, 23, 9, 10};
    [[=ImRefl::end_region()]]

    [[=ImRefl::begin_region("chrono:: types")]]
    system_clock::time_point time_point_ = system_clock::now();
    const decltype(time_point_) const_time_point_ = time_point_;
    duration<double> duration_ = time_point_ - const_time_point_;
    const decltype(duration_) const_duration_ = 18952ms;
    year_month_day year_month_day_ = 1989y / November / 9;
    const year_month_day const_year_month_day_ = 1815y / February / 17;
    hh_mm_ss<decltype(duration_)> hh_mm_ss_{hours(4) + minutes(17) + seconds(48)};
    const decltype(hh_mm_ss_) const_hh_mm_ss_{hours(7) + minutes(48) + seconds(19)};
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
    
    example ex = {};

    auto func = []() {
        static size_t n = 0;
        ++n;
        std::println("Function called {} time{}", n, n == 1 ? "" : "s");
    };
    ex.function_ = func;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ex.time_point_ = system_clock::now();
        ex.duration_ = ex.time_point_ - ex.const_time_point_;

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

