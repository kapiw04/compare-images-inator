#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include "imgui.h"
#include "imgui-SFML.h"

int main() {
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Hello World - SFML + ImGui");
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window)) {
        return -1;
    }

    sf::Clock deltaClock;
    
    while (window.isOpen()) {
        while (auto event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);
            
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        // ImGui window
        ImGui::Begin("Hello, World!");
        ImGui::Text("This is a simple SFML + ImGui application.");
        if (ImGui::Button("Click me!")) {
            ImGui::OpenPopup("Clicked!");
        }
        if (ImGui::BeginPopup("Clicked!")) {
            ImGui::Text("Button was clicked!");
            ImGui::EndPopup();
        }
        ImGui::End();

        window.clear(sf::Color(100, 100, 100));
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}