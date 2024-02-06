#include <SFML/Graphics.hpp>
#include <iostream>

int main()
{
    sf::RenderWindow window(sf::VideoMode(1500, 720), "STDISCM_Project_1", sf::Style::Close | sf::Style::Titlebar);

    while (window.isOpen()) 
    {
        sf::Event evnt;

        while (window.pollEvent(evnt)) 
        {
            if (evnt.type == evnt.Closed) {
                window.close();
            }
        }
    }

    return 0;
}

