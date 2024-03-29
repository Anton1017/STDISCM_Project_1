#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "BS_thread_pool.hpp" // BS::thread_pool from https://github.com/bshoshany/thread-pool
#include "BS_thread_pool_utils.hpp"

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

// default thread count is 4 (single and dual-core systems may be assigned with 4 threads)
int threadpool_size = std::thread::hardware_concurrency() > 2 ? std::thread::hardware_concurrency() : 4; 
#define THREADPOOL_SIZE threadpool_size - 1 // save one thread for rendering
#define THREADING_THRESHOLD 5000 // Obtained from testing, point on which single-threaded performance starts to drop in FPS

struct Particle {
    ImVec2 position;
    ImVec2 velocity;
    // Angle is computed upon addition of particle, and translated to horizontal and vertical velocity (ImVec2).
};

const int numParticles = 0;
std::vector<Particle> particles;
BS::thread_pool pool(THREADPOOL_SIZE);

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
    if (p2.x - p1.x == 0.0f)
        // if vertical line
        return std::numeric_limits<float>::infinity();
    return (p2.y - p1.y) / (p2.x - p1.x);
}

bool doIntersect(ImVec2 p1, ImVec2 q1, ImVec2 p2, ImVec2 q2) {
    // Calculate slopes of the line and particle trajectory.
    float slope1 = calculateSlope(p1, q1);
    float slope2 = calculateSlope(p2, q2);

    // Check for vertical lines
    if (std::isinf(slope1) && std::isinf(slope2)) {
        // Both line and particle trajectory are vertical and never intersect
        return false;
    }

    // Calculate y-intercepts (b) for each line
    float b1 = p1.y - slope1 * p1.x;
    float b2 = p2.y - slope2 * p2.x;

    // Calculate intersection point
    float intersectionX;
    float intersectionY;

    if (std::isinf(slope1)) {
        // Line is vertical
        intersectionX = p1.x;
        intersectionY = slope2 * intersectionX + b2;
    } else if (std::isinf(slope2)) {
        // Particle trajectory is vertical
        intersectionX = p2.x;
        intersectionY = slope1 * intersectionX + b1;
    } else {
        // Neither line nor particle is vertical
        intersectionX = (b2 - b1) / (slope1 - slope2);
        intersectionY = slope1 * intersectionX + b1;
    }

    // Check if the intersection point lies on both line and particle trajectory
    if (!std::isnan(intersectionX) && !std::isnan(intersectionY) &&
        (intersectionX >= std::min(p1.x, q1.x) && intersectionX <= std::max(p1.x, q1.x)) &&
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

std::vector<std::pair<int,int>> getJobList() {
    int particlesSize = static_cast<int>(particles.size());
    std::vector<std::pair<int,int>> jobList;

    int threadJobChunk = particlesSize / THREADPOOL_SIZE;
    int i = 0;
    int j = 0;
    if(threadJobChunk < THREADING_THRESHOLD){
        while(particlesSize != 0){
            if(particlesSize - THREADING_THRESHOLD > 0){
                j += THREADING_THRESHOLD - 1;
                jobList.push_back(std::pair(i,j));
                particlesSize -= THREADING_THRESHOLD;
                i =+ j + 1;
                ++j;
            }else{
                j += particlesSize - 1;
                jobList.push_back(std::pair(i,j));
                particlesSize = 0;
            }
        }
    } else{
        int jobRemainder = particlesSize % THREADPOOL_SIZE;
        while(particlesSize != 0){
            if(particlesSize - threadJobChunk > 0){
                j += threadJobChunk - 1;
                if(jobRemainder > 0){
                    ++j;
                    --jobRemainder;
                    --particlesSize;
                }
                jobList.push_back(std::pair(i,j));
                particlesSize -= threadJobChunk;
                i = j + 1;
                ++j;
            }else{
                j += particlesSize - 1;
                jobList.push_back(std::pair(i,j));
                particlesSize = 0;
            }
        }
    }
    return jobList;
}


void UpdateParticles(ImGuiIO& io, ImDrawList* drawList) {
    float frameRate = io.Framerate;
    std::vector<std::pair<int,int>> jobList = getJobList();

    for (auto& job : jobList){
        pool.detach_task( // Assign to threadpool
            [&frameRate, &job]
            {
                for (int i = job.first; i <= job.second; i++) {
                    // Check for collision with the walls
                    for (const auto& wallSegment : wall) {
                        ImVec2 wallP1 = wallSegment.p1;
                        ImVec2 wallP2 = wallSegment.p2;

                        // calculate projected position of particle on next frame (assuming no collision with wall)
                        ImVec2 nextPosition = ImVec2(
                            particles[i].position.x + particles[i].velocity.x / frameRate,
                            particles[i].position.y + particles[i].velocity.y / frameRate
                        );

                        if (doIntersect(particles[i].position, nextPosition, wallP1, wallP2)) {
                            ImVec2 intersectPoint = particleIntersectWall(particles[i], wallP1, wallP2);
                            // Collision occurred, update position and reflect velocity
                            particles[i].position.x = intersectPoint.x;
                            particles[i].position.y = intersectPoint.y;

                            // Calculate the reflection vector based on the wall's normal
                            ImVec2 wallVector = ImVec2(wallP2.y - wallP1.y, wallP1.x - wallP2.x); // Perpendicular to the wall
                            float length = std::sqrt(wallVector.x * wallVector.x + wallVector.y * wallVector.y);
                            wallVector = ImVec2(wallVector.x / length, wallVector.y / length);

                            // Reflect the velocity vector
                            float dotProduct = 2.0f * (particles[i].velocity.x * wallVector.x + particles[i].velocity.y * wallVector.y);
                            particles[i].velocity.x -= dotProduct * wallVector.x;
                            particles[i].velocity.y -= dotProduct * wallVector.y;

                            // Move the particle slightly away from the collision point
                            particles[i].position.x += wallVector.x * 0.1f;
                            particles[i].position.y += wallVector.y * 0.1f;
                        }
                    }

                    // Update particle's position based on its velocity
                    particles[i].position.x += particles[i].velocity.x / frameRate;
                    particles[i].position.y += particles[i].velocity.y / frameRate;

                    // Bounce off the walls
                    if (particles[i].position.x <= 0 || particles[i].position.x > 1280 ||
                        particles[i].position.y <= 0 || particles[i].position.y > 720) {
                        AdjustParticlePosition(particles[i]);
                    }
                }
                // std::cout << "fin " << job.first << " " << job.second << std::endl;
            }
                
        );
    }
    pool.detach_task(
        [&drawList]{
            for (const auto& particle : particles) {

                drawList->AddRectFilled(
                    ImVec2(particle.position.x - 1.5f, particle.position.y - 1.5f),
                    ImVec2(particle.position.x + 1.5f, particle.position.y + 1.5f),
                    IM_COL32(255, 255, 255, 255)
                );
            }
        }
    );
    pool.wait();
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
    std::cout << "Threadpool size: " << THREADPOOL_SIZE << std::endl;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // ImGui new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        // ImGui::ShowDemoWindow();

        double currentTime = glfwGetTime();

        ImGui::SetNextWindowSize(ImVec2(100, 20));
        ImGui::SetNextWindowPos(ImVec2(
            ImGui::GetIO().DisplaySize.x - 100,
            ImGui::GetIO().DisplaySize.y - 20));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Framerate", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
        if (currentTime - lastDisplayTime >= 0.5) { //every 0.5 secpmds
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
        //Parameters for wall, endpoints for the twopoints
        static int wall_x1 = 1;
        static int wall_y1 = 1;
        static int wall_x2 = 1;
        static int wall_y2 = 1;
        ImGui::Text("Wall Count: %d", wall.size());
        ImGui::Text("Endpoint 1");
        ImGui::SliderInt("X1", &wall_x1, 0, 1279);
        ImGui::SliderInt("Y1", &wall_y1, 0, 719);
        ImGui::Text("Endpoint 2");
        ImGui::SliderInt("X2", &wall_x2, 0, 1279);
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
        UpdateParticles(io, drawList);

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
