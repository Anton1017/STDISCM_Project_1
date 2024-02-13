#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cmath>
#include <math.h>
#include <cstdlib>
#include <ctime>
#include <iomanip>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct Particle {
    ImVec2 position;
    ImVec2 velocity;
    // Angle is computed upon addition of particle, and translated to horizontal and vertical velocity (ImVec2).
};

const int numParticles = 0;
std::vector<Particle> particles;

struct Walls {
    ImVec2 p1;
    ImVec2 p2;
};
std::vector<Walls> wall;
const int numWalls = 0;

void AdjustParticlePosition(Particle& particle) {
    float slope = particle.velocity.y / particle.velocity.x;

    if (particle.position.x < 0) {
        particle.position.x = 0;
        particle.position.y = slope * particle.position.x + particle.position.y;
        particle.velocity.x *= -1;
    } else if (particle.position.x >= 1280) {
        particle.position.x = 1279;
        particle.position.y = slope * (particle.position.x - 1279) + particle.position.y;
        particle.velocity.x *= -1;
    }

    if (particle.position.y < 0) {
        particle.position.y = 0;
        particle.position.x = particle.position.y / slope + particle.position.x;
        particle.velocity.y *= -1;
    } else if (particle.position.y >= 720) {
        particle.position.y = 719;
        particle.position.x = (particle.position.y - 719) / slope + particle.position.x;
        particle.velocity.y *= -1;
    }
}

float calculateSlope(ImVec2 p1, ImVec2 p2) {
    return (p2.y - p1.y) / (p2.x - p1.x);
}


bool doIntersect(ImVec2 p1, ImVec2 q1, ImVec2 p2, ImVec2 q2) {
    // Calculate slopes of the lines
    float slope1 = calculateSlope(p1, q1);
    float slope2 = calculateSlope(p2, q2);

    // If slopes are equal, the lines are either parallel or coincident
    if (slope1 == slope2) {
        return false;
    }

    // Calculate y-intercepts (b values) for each line
    float b1 = p1.y - slope1 * p1.x;
    float b2 = p2.y - slope2 * p2.x;

    // Calculate intersection point
    float intersectionX = (b2 - b1) / (slope1 - slope2);
    float intersectionY = slope1 * intersectionX + b1;

    // Check if the intersection point lies on both line segments
    if ((intersectionX >= std::min(p1.x, q1.x) && intersectionX <= std::max(p1.x, q1.x)) &&
        (intersectionY >= std::min(p1.y, q1.y) && intersectionY <= std::max(p1.y, q1.y)) &&
        (intersectionX >= std::min(p2.x, q2.x) && intersectionX <= std::max(p2.x, q2.x)) &&
        (intersectionY >= std::min(p2.y, q2.y) && intersectionY <= std::max(p2.y, q2.y))) {
        return true;
    }

    return false;
}

ImVec2 particleIntersectWall(Particle particle, ImVec2 wallStart, ImVec2 wallEnd) {
    // Calculate the intersection point of the particle's trajectory with the wall
    float t_intersection = (wallStart.x * (particle.position.y - wallEnd.y) + wallEnd.x * (wallStart.y - particle.position.y) +
                            particle.position.x * (wallEnd.y - wallStart.y)) /
                           (particle.velocity.x * (wallStart.y - wallEnd.y) + particle.velocity.y * (wallEnd.x - wallStart.x));

    // Calculate the intersection point
    return ImVec2{particle.position.x + t_intersection * particle.velocity.x, particle.position.y + t_intersection * particle.velocity.y};
}


void UpdateParticles(ImGuiIO& io) {
    for (auto& particle : particles) {
        // Check for collision with the walls
        for (const auto& wallSegment : wall) {
            ImVec2 wallP1 = wallSegment.p1;
            ImVec2 wallP2 = wallSegment.p2;

            // calculate projected position of particle on next frame (assuming no collision with wall)
            ImVec2 nextPosition = ImVec2(
                particle.position.x + particle.velocity.x / io.Framerate,
                particle.position.y + particle.velocity.y / io.Framerate
            );

            if (doIntersect(particle.position, nextPosition, wallP1, wallP2)) {
                ImVec2 intersectPoint = particleIntersectWall(particle, wallP1, wallP2);
                // Collision occurred, update position and reflect velocity
                particle.position.x = intersectPoint.x;
                particle.position.y = intersectPoint.y;

                // Calculate the reflection vector
                ImVec2 normal = ImVec2(wallP2.y - wallP1.y, wallP1.x - wallP2.x); // Perpendicular to the wall
                float length = std::sqrt(normal.x * normal.x + normal.y * normal.y);
                normal = ImVec2(normal.x / length, normal.y / length);

                // Reflect the velocity vector
                float dotProduct = 2.0f * (particle.velocity.x * normal.x + particle.velocity.y * normal.y);
                particle.velocity.x -= dotProduct * normal.x;
                particle.velocity.y -= dotProduct * normal.y;

                // Move the particle slightly away from the collision point
                particle.position.x += normal.x * 0.1f;
                particle.position.y += normal.y * 0.1f;
            }
        }

        // Update particle's position based on its velocity
        particle.position.x += particle.velocity.x / io.Framerate;
        particle.position.y += particle.velocity.y / io.Framerate;

        // Bounce off the walls
        if (particle.position.x <= 0 || particle.position.x > 1280 ||
            particle.position.y <= 0 || particle.position.y > 720) {
            AdjustParticlePosition(particle);
        }
    }
}




int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        return -1;
    }

    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(1920, 1080, "Particle Simulation (ImGui + GLFW)", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    glfwSwapInterval(1); // Enable vsync

    glfwMaximizeWindow(window);

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    double lastDisplayTime = glfwGetTime();
    double currentFramerate = io.Framerate;

    static int sx = 0;
    static int sy = 0;
    static int ex = 0;
    static int ey = 0;
    static float startSpeed = 10.0f;
    static float endSpeed = 10.0f;
    static float startAngle = 0.0f;
    static float endAngle = 0.0f;
    static int numAddParticles = 1;

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
            // std::cout << "Framerate: " << currentFramerate << " FPS" << std::endl;
            lastDisplayTime = currentTime;
        }
        ImGui::Text("%.3f FPS", currentFramerate);
        
        ImGui::End();
        ImGui::PopStyleVar(2);


        ImGui::SetNextWindowSize(ImVec2(1280, 720));
        ImGui::SetNextWindowPos(ImVec2(0, 0));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Particle Simulation", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        for (const auto& particle : particles) {

            drawList->AddRectFilled(
                ImVec2(particle.position.x - 1.5f, particle.position.y - 1.5f),
                ImVec2(particle.position.x + 1.5f, particle.position.y + 1.5f),
                IM_COL32(255, 255, 255, 255)
            );
        }
        for (const auto& walls : wall) {
            drawList->AddLine(
                walls.p1,
                walls.p2,
                IM_COL32(0, 0, 255, 255),
                2.0f // Line thickness
            );
        }

        ImGui::End();
        ImGui::PopStyleVar(2);

        ImGui::SetNextWindowSize(ImVec2(640, 320));
        ImGui::SetNextWindowPos(ImVec2(1281, 0));

        ImGui::Begin("[Start-End Point] Batch Adding");
        
        ImGui::Text("Particle Count: %d", particles.size());

        ImGui::SliderInt("[Start Point] - x", &sx, 0, 1279);
        ImGui::SliderInt("[Start Point] - y", &sy, 0, 719);
        ImGui::SliderInt("[End Point] - x", &ex, 0, 1279);
        ImGui::SliderInt("[End Point] - y", &ey, 0, 719);
        ImGui::InputFloat("[Start Velocity] pix/s", &startSpeed);
        ImGui::SliderFloat("[Start Angle] - degrees", &startAngle, 0.0f, 359.999f);
        ImGui::InputInt("Number of Particles", &numAddParticles);
        if (ImGui::Button("Add")) {

            float xSpacing = static_cast<float>(ex-sx) / (numAddParticles-1);
            float ySpacing = static_cast<float>(ey-sy) / (numAddParticles-1);
            float xSpacingSum = 0.0f;
            float ySpacingSum = 0.0f;
            for (int i = 0; i < numAddParticles; i++) {
                Particle particle;
                particle.position = ImVec2(static_cast<float>(sx) + xSpacingSum, 719 - (static_cast<float>(sy) + ySpacingSum));
                xSpacingSum += xSpacing;
                ySpacingSum += ySpacing;
                float angle = (-(startAngle)) * (static_cast<float>(M_PI) / 180.0f); //convert degrees to radians
                particle.velocity = ImVec2(
                    startSpeed * std::cos(angle),
                    startSpeed * std::sin(angle)
                );

                particles.push_back(particle);
            }
        }

        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::CalcTextSize("Reset").x - ImGui::GetStyle().FramePadding.x * 2 - ImGui::GetStyle().ScrollbarSize);
        ImGui::SetCursorPosY(ImGui::CalcTextSize("Reset").y * 2);
        if (ImGui::Button("Reset")) {
            particles.clear();
        }
        
        float startXCursor = static_cast<float>(sx);
        float startYCursor = static_cast<float>(720 - sy);

        drawList->AddRectFilled(
            ImVec2(startXCursor - 3.0f, startYCursor - 3.0f),
            ImVec2(startXCursor + 3.0f, startYCursor + 3.0f),
            IM_COL32(0, 255, 0, 192)
        );


        float endXCursor = static_cast<float>(ex);
        float endYCursor = static_cast<float>(720 - ey);

        drawList->AddRectFilled(
            ImVec2(endXCursor - 3.0f, endYCursor - 3.0f),
            ImVec2(endXCursor + 3.0f, endYCursor + 3.0f),
            IM_COL32(255, 0, 0, 192)
        );



        ImGui::End();



        ImGui::SetNextWindowSize(ImVec2(640, 320));
        ImGui::SetNextWindowPos(ImVec2(1281, 321));

        ImGui::Begin("[Start-End Angle] Batch Adding");

        ImGui::SliderInt("[Start Point] - x", &sx, 0, 1279);
        ImGui::SliderInt("[Start Point] - y", &sy, 0, 719);
        ImGui::InputFloat("[Start Velocity] pix/s", &startSpeed);
        ImGui::SliderFloat("[Start Angle] - degrees", &startAngle, 0.0f, 359.999f);
        ImGui::SliderFloat("[End Angle] - degrees", &endAngle, 0.0f, 359.999f);
        ImGui::InputInt("Number of Particles", &numAddParticles);
        if (ImGui::Button("Add")) {
            float angleDiff;
            if(endAngle >= startAngle)
                angleDiff = endAngle-startAngle;
            else
                angleDiff = endAngle - abs(startAngle - 360);

            float angleSpacing = static_cast<float>(angleDiff / (numAddParticles));
            float angleSpacingSum = 0.0f;
            for (int i = 0; i < numAddParticles; i++) {
                Particle particle;
                particle.position = ImVec2(static_cast<float>(sx), 719 - (static_cast<float>(sy)));
                float angle = (-(startAngle + angleSpacingSum)) * (static_cast<float>(M_PI) / 180.0f); //convert degrees to radians
                angleSpacingSum += angleSpacing;
                particle.velocity = ImVec2(
                    startSpeed * std::cos(angle),
                    startSpeed * std::sin(angle)
                );

                particles.push_back(particle);
            }
        }
        ImGui::End();


        ImGui::SetNextWindowSize(ImVec2(640, ImGui::GetIO().DisplaySize.y - 20 - 640));
        ImGui::SetNextWindowPos(ImVec2(1281, 641));
        ImGui::Begin("[Start-End Velocity] Batch Adding");

        ImGui::SliderInt("[Start Point] - x", &sx, 0, 1279);
        ImGui::SliderInt("[Start Point] - y", &sy, 0, 719);
        ImGui::InputFloat("[Start Velocity] pix/s", &startSpeed);
        ImGui::InputFloat("[End Velocity] pix/s", &endSpeed);
        ImGui::SliderFloat("Start Angle - degrees", &startAngle, 0.0f, 359.999f);
        ImGui::InputInt("Number of Particles", &numAddParticles);
        if (ImGui::Button("Add")) {

            float vSpacing = static_cast<float>(endSpeed-startSpeed) / (numAddParticles);
            float vSpacingSum = 0.0f;
            for (int i = 0; i < numAddParticles; i++) {
                Particle particle;
                particle.position = ImVec2(static_cast<float>(sx), 719 - (static_cast<float>(sy)));
                float angle = (-(startAngle)) * (static_cast<float>(M_PI) / 180.0f); //convert degrees to radians
                particle.velocity = ImVec2(
                    (startSpeed+vSpacingSum) * std::cos(angle),
                    (startSpeed+vSpacingSum) * std::sin(angle)
                );
                vSpacingSum += vSpacing;

                particles.push_back(particle);
            }
        }


        ImGui::End();


        ImGui::SetNextWindowSize(ImVec2(1280, ImGui::GetIO().DisplaySize.y - 720));
        ImGui::SetNextWindowPos(ImVec2(0, 721));
        ImGui::Begin("Wall Parameters");
        //Parameters for wall
        static int wall_x1 = 1;
        static int wall_y1 = 1;
        static int wall_x2 = 1;
        static int wall_y2 = 1;
        ImGui::Text("Wall Count: %d", wall.size());
        ImGui::Text("Endpoint 1");
        ImGui::SliderInt("X1", &wall_x1, 0, 1219);
        ImGui::SliderInt("Y1", &wall_y1, 0, 719);
        ImGui::Text("Endpoint 2");
        ImGui::SliderInt("X2", &wall_x2, 0, 1219);
        ImGui::SliderInt("Y2", &wall_y2, 0, 719);
        if (ImGui::Button("Add Wall")) {
            Walls newWall = { ImVec2(static_cast<float>(wall_x1), static_cast<float>(720 - wall_y1)), ImVec2(static_cast<float>(wall_x2), static_cast<float>(720 - wall_y2)) };
            wall.push_back(newWall);
        }
        if (ImGui::Button("Reset Wall")) {
            wall.clear();
        }

        ImVec2 flippedWallP1 = ImVec2(static_cast<float>(wall_x1), static_cast<float>(720 - wall_y1));
        ImVec2 flippedWallP2 = ImVec2(static_cast<float>(wall_x2), static_cast<float>(720 - wall_y2));
        //Endpoints 1
        drawList->AddRectFilled(
            ImVec2(flippedWallP1.x - 3.0f, flippedWallP1.y - 3.0f),
            ImVec2(flippedWallP1.x + 3.0f, flippedWallP1.y + 3.0f),
            IM_COL32(0, 0, 255, 255)
        );
        //Endpoints 2
        drawList->AddRectFilled(
            ImVec2(flippedWallP2.x - 3.0f, flippedWallP2.y - 3.0f),
            ImVec2(flippedWallP2.x + 3.0f, flippedWallP2.y + 3.0f),
            IM_COL32(0, 0, 255, 255)
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
