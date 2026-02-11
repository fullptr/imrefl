#include <imgui.h>
#include "imrefl.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

#include <experimental/meta>
#include <print>


struct foo
{
    int x_pos;
    double health;
    float gravity;
};

enum class fruits
{
    apple,
    banana,
    strawberry
};

void DrawFruitCombo(fruits& f)

{
	    const char* currentFruitName = "";
    switch (f)
    {
        case fruits::apple:      currentFruitName = "apple"; break;
        case fruits::banana:     currentFruitName = "banana"; break;
        case fruits::strawberry: currentFruitName = "strawberry"; break;
    }
	if (ImGui::BeginCombo("fruits", currentFruitName)) {
		if (ImGui::Selectable("apple", f == fruits::apple)) f = fruits::apple;
		if (ImGui::Selectable("banana", f == fruits::banana)) f = fruits::banana;
		if (ImGui::Selectable("strawberry", f == fruits::strawberry)) f = fruits::strawberry;
		ImGui::EndCombo();
	}
}

void glfw_error_callback(int error, const char* description)
{
    std::println("GLFW error {}: {}", error, description);
}

int main()
{
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit()) {
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "ImGui + GLFW", nullptr, nullptr);
    if (!window) {
        return 1;
    }

    glfwMakeContextCurrent(window);
    //glfwSwapInterval(1); // vsync
    std::println("{}\n", std::meta::display_string_of(^^foo));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    foo f = {};
    fruits fr = fruits::apple;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Test");
	ImRefl::Input(f);
	DrawFruitCombo(fr);
	ImGui::Text("value %d", static_cast<int>(fr));
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

