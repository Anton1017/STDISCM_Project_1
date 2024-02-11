#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <vector>
#include <cmath>
#include <math.h>
#include <cstdlib>
#include <ctime>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct Particle {
    ImVec2 position;
    ImVec2 velocity;
    float angle;  // Angle in radians
};

const int numParticles = 0;
std::vector<Particle> particles;

struct Walls {
    ImVec2 p1;
    ImVec2 p2;
};
std::vector<Walls> wall;
const int numWalls = 0;
//Example user-defined line
Walls userDefinedLine = { ImVec2(200.00, 200.00), ImVec2(400.00, 400.00) };

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

void UpdateParticles(ImGuiIO& io) {
    for (auto& particle : particles) {
        // use framerate to keep speed constant
        particle.position.x += particle.velocity.x / io.Framerate;
        particle.position.y += particle.velocity.y / io.Framerate;

        // Bounce off the walls
        if (particle.position.x <= 0 || particle.position.x >= 1280) {
            particle.velocity.x *= -1;
        }
        if (particle.position.y <= 0 || particle.position.y >= 720) {
            particle.velocity.y *= -1;
        }
        //TODO Checks if there is collision
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

    glfwSwapInterval(1); // Enable vsync

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

    double lastDisplayTime = glfwGetTime();
    double currentFramerate = io.Framerate;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // ImGui new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        // ImGui::ShowDemoWindow(); // Show demo window! :)

        double currentTime = glfwGetTime();

        ImGui::SetNextWindowSize(ImVec2(100, 20));
        ImGui::SetNextWindowPos(ImVec2(
            ImGui::GetIO().DisplaySize.x - 100,
            ImGui::GetIO().DisplaySize.y - 20));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Framerate", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
        if (currentTime - lastDisplayTime >= 0.5) { //0.5s
            currentFramerate = io.Framerate;
            std::cout << "Framerate: " << currentFramerate << " FPS" << std::endl;
            lastDisplayTime = currentTime;
        }
        ImGui::Text("%.3f FPS", currentFramerate);
        
        ImGui::End();
        ImGui::PopStyleVar(2);


        ImGui::SetNextWindowSize(ImVec2(1280, 720));
        ImGui::SetNextWindowPos(ImVec2(0, 0));

        // ImGui UI
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Particle Simulation", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

        // Draw particles within an ImGui window
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        for (const auto& particle : particles) {

            drawList->AddRectFilled(
                ImVec2(particle.position.x - 1.5f, particle.position.y - 1.5f),
                ImVec2(particle.position.x + 1.5f, particle.position.y + 1.5f),
                IM_COL32(255, 255, 255, 255)
            );
        }
        ImGui::End();
        ImGui::PopStyleVar(2);

        ImGui::SetNextWindowSize(ImVec2(640, ImGui::GetIO().DisplaySize.y - 20));
        ImGui::SetNextWindowPos(ImVec2(1281, 0));

        ImGui::Begin("Particle Simulation Parameters");
        static int x = 1;
        static int y = 1;
        static float speed = 10.0f;
        static float angle = 0.0f;
        static int numAddParticles = 1;
        ImGui::Text("Particle Count: %d", particles.size());

        ImGui::SliderInt("Initial Position - x", &x, 1, 1280);
        ImGui::SliderInt("Initial Position - y", &y, 1, 720);
        ImGui::InputFloat("Speed - pixels/sec.", &speed);
        ImGui::SliderFloat("Angle - degrees", &angle, 0.0f, 360.0f);
        ImGui::InputInt("Number of Particles", &numAddParticles);
        if (ImGui::Button("Add")) {

            int spacing = numAddParticles;
            for (int i = 0; i < numAddParticles; ++i) {
                Particle particle;
                particle.position = ImVec2(static_cast<float>((i * 4 + x) % 1280), static_cast<float>(abs((-i * 4 + (720 - y)) % 720)));
                particle.angle = (-(angle)) * (static_cast<float>(M_PI) / 180.0f); //convert degrees to radians
                particle.velocity = ImVec2(
                    speed * std::cos(particle.angle),
                    speed * std::sin(particle.angle)
                );

                particles.push_back(particle);
            }
        }
        if (ImGui::Button("Reset")) {
            particles.clear();
        }
        //Parameters for wall
        static int wall_x1 = 1;
        static int wall_y1 = 1;
        static int wall_x2 = 1;
        static int wall_y2 = 1;
        ImGui::Text("Wall Count: %d", wall.size());
        ImGui::Text("Endpoint 1");
        ImGui::SliderInt("X1", &wall_x1, 1, 1280);
        ImGui::SliderInt("Y1", &wall_y1, 1, 720);
        ImGui::Text("Endpoint 2");
        ImGui::SliderInt("X2", &wall_x2, 1, 1280);
        ImGui::SliderInt("Y2", &wall_y2, 1, 720);
        if (ImGui::Button("Add Wall")) {
            Walls newWall = { ImVec2(static_cast<float>(wall_x1), static_cast<float>(wall_y1)), ImVec2(static_cast<float>(wall_x2), static_cast<float>(wall_y2)) };
            wall.push_back(newWall);
        }
        if (ImGui::Button("Reset Wall")) {
            wall.clear();
        }

        float currXCursor = static_cast<float>(x);
        float currYCursor = static_cast<float>(720 - y);

        drawList->AddRectFilled(
            ImVec2(currXCursor - 3.0f, currYCursor - 3.0f),
            ImVec2(currXCursor + 3.0f, currYCursor + 3.0f),
            IM_COL32(255, 255, 0, 192)
        );

        ImGui::End();


        // Update and render particles
        UpdateParticles(io);

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
