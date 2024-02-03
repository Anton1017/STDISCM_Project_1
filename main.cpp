#include <SFML/Graphics.hpp>
#include <cmath>
#include <cstdlib>
#include <ctime>

int main() {
    // Create a window
    sf::RenderWindow window(sf::VideoMode(1280, 720), "Particle Simulation");

    // Seed for random number generation
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // Define particle properties
    struct Particle {
        sf::Vector2f position;
        sf::Vector2f velocity;
    };

    const int numParticles = 1000;
    Particle particles[numParticles];

    // Initialize particles with random positions, speeds, and angles
    for (int i = 0; i < numParticles; ++i) {
        particles[i].position = sf::Vector2f(static_cast<float>(rand() % 1280), static_cast<float>(rand() % 720));
        float speed = static_cast<float>(rand() % 200 + 50); // Speed between 50 and 250 pixels per second
        float angle = static_cast<float>(rand() % 360); // Angle in degrees
        particles[i].velocity = sf::Vector2f(speed * std::cos(angle * 3.14f / 180.0f),
                                             speed * std::sin(angle * 3.14f / 180.0f));
    }

    // Main simulation loop
    while (window.isOpen()) {
        // Process events
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        // Clear the window
        window.clear();

        // Update and render particles
        for (int i = 0; i < numParticles; ++i) {
            // Update particle position
            particles[i].position += particles[i].velocity * 0.01f; // Adjust the time step as needed

            // Bounce off the walls if a particle goes out of bounds
            if (particles[i].position.x < 0 || particles[i].position.x > 1280) {
                particles[i].velocity.x = -particles[i].velocity.x;
            }
            if (particles[i].position.y < 0 || particles[i].position.y > 720) {
                particles[i].velocity.y = -particles[i].velocity.y;
            }

            // Draw the particle as a one-pixel rectangle
            sf::RectangleShape pixel(sf::Vector2f(1.0f, 1.0f));
            pixel.setPosition(particles[i].position);
            pixel.setFillColor(sf::Color::White);
            window.draw(pixel);
        }

        // Display the content
        window.display();
    }

    return 0;
}
