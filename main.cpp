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
            ImVec2(static_cast<float>(rand() % 10 + 1) * std::cos(angle),
                   static_cast<float>(rand() % 10 + 1) * std::sin(angle)),
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
    GLFWwindow* window = glfwCreateWindow(1920, 720, "Particle Simulation (ImGui + GLFW)", NULL, NULL);
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
        ImGui::ShowDemoWindow(); // Show demo window! :)


        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowBorderSize = 1.0f;  // Set the window border size to 1 pixel

        // Set other window-related styles as needed
        style.FrameBorderSize = 1.0f;   // Set the size of the frame border
        style.FrameRounding = 0.0f;     // Disable rounding of frame corners
        style.WindowRounding = 0.0f;    // Disable rounding of window corners
        style.ScrollbarSize = 0.0f;    // Set the size of scrollbars
        style.GrabMinSize = 0.0f;      // Set the minimum size of the resizing grip

        ImGui::SetNextWindowSize(ImVec2(1280, 720));
        ImGui::SetNextWindowPos(ImVec2(0, 0));

        // ImGui UI
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Particle Simulation", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

        // Draw particles within an ImGui window
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        for (const auto& particle : particles) {
            		// Yellow is content region min/max
			ImVec2 vMin = ImGui::GetWindowContentRegionMin();
			ImVec2 vMax = ImGui::GetWindowContentRegionMax();

			vMin.x += ImGui::GetWindowPos().x;
			vMin.y += ImGui::GetWindowPos().y;
			vMax.x += ImGui::GetWindowPos().x;
			vMax.y += ImGui::GetWindowPos().y;

            drawList->AddRectFilled(
                particle.position,
                ImVec2(particle.position.x + 1.0f, particle.position.y + 1.0f),
                IM_COL32(255, 255, 255, 255)
            );
        }
        ImGui::End();
        ImGui::PopStyleVar(2);

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
