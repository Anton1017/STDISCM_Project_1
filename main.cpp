#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>

struct Particle {
    ImVec2 position;
    ImVec2 velocity;
    float angle;  // Angle in radians
};

const int numParticles = 1000;
std::vector<Particle> particles;

void InitializeParticles() {
    particles.clear();
    for (int i = 0; i < numParticles; ++i) {
        float angle = static_cast<float>(rand() % 360) * 3.14f / 180.0f;
        particles.push_back({
            ImVec2(static_cast<float>(rand() % 200), static_cast<float>(rand() % 200)),
            ImVec2(static_cast<float>(rand() % 10) * std::cos(angle),
                   static_cast<float>(rand() % 10) * std::sin(angle)),
            angle
        });
    }
}

void UpdateParticles() {
    for (auto& particle : particles) {
        particle.position.x += particle.velocity.x * 0.01f;
        particle.position.y += particle.velocity.y * 0.01f;

        // Bounce off the walls
        if (particle.position.x < 0 || particle.position.x >= 1280) {
            particle.velocity.x *= -1;
        }
        if (particle.position.y < 0 || particle.position.y >= 720) {
            particle.velocity.y *= -1;
        }
    }
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        return -1;
    }

    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Particle Simulation (ImGui + GLFW)", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Seed for random number generation
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // Set up initial particles
    InitializeParticles();

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // ImGui new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ImGui UI
        ImGui::Begin("Particle Simulation");

        if (ImGui::Button("Reset Particles")) {
            InitializeParticles();
        }

        // Draw particles within an ImGui window
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        for (const auto& particle : particles) {
            drawList->AddRectFilled(
                particle.position,
                ImVec2(particle.position.x + 1.0f, particle.position.y + 1.0f),
                IM_COL32(255, 255, 255, 255)
            );
        }

        ImGui::End();

        // Update and render particles
        UpdateParticles();

        // ImGui rendering
        ImGui::Render();
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap front and back buffers
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}
