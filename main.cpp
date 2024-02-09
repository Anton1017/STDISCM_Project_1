// main.cpp
#include <iostream>
#include <vector>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

struct Particle {
    float x, y;
    float speed, angle;
};

std::vector<Particle> particles;
float boxWidth = 720.0f;
float boxHeight = 1280.0f;

// GLFW callback for handling GUI events
void glfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        // Normalize particle position to box coordinates
        Particle particle;
        particle.x = static_cast<float>(xpos) / boxWidth;
        particle.y = 1.0f - static_cast<float>(ypos) / boxHeight;

        particles.push_back(particle);
    }
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Create a GLFW window
    GLFWwindow* window = glfwCreateWindow(720, 1280, "Particle Physics Simulator", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Set GLFW callback for mouse button events
    glfwSetMouseButtonCallback(window, glfwMouseButtonCallback);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ImGui GUI
        ImGui::Begin("Particle Parameters");
        static float speed = 0.1f;
        static float angle = 45.0f;
        static int numParticles = 1;

        ImGui::InputFloat("Speed", &speed);
        ImGui::SliderFloat("Angle", &angle, 0.0f, 360.0f);
        ImGui::InputInt("Number of Particles", &numParticles);

        if (ImGui::Button("Add")) {
            for (int i = 0; i < numParticles; ++i) {
                Particle particle;
                particle.speed = speed;
                particle.angle = angle;

                particles.push_back(particle);
            }
        }

        ImGui::End();

        // Simulate particle physics
        for (auto& particle : particles) {
            // Update particle position based on speed and angle
            particle.x += particle.speed * std::cos(glm::radians(particle.angle));
            particle.y += particle.speed * std::sin(glm::radians(particle.angle));

            // Bounce off the walls
            if (particle.x < 0.0f || particle.x > 1.0f) {
                particle.angle = 180.0f - particle.angle;
            }
            if (particle.y < 0.0f || particle.y > 1.0f) {
                particle.angle = -particle.angle;
            }
        }

        // Render particles
        glClear(GL_COLOR_BUFFER_BIT);
        // Add your rendering code here (render particles as one-pixel points)

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
