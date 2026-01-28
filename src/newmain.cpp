#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Angle.hpp> // Required for sf::degrees
#include <SFML/System/Vector2.hpp> // Required for sf::Vector2u
#include "imgui.h"
#include "imgui-SFML.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdint> 
#include <cstring> 

constexpr unsigned int WINDOW_WIDTH = 1800;
constexpr unsigned int WINDOW_HEIGHT = 900;
constexpr float MIN_THICKNESS = 1.0f;
constexpr float MAX_THICKNESS = 50.0f;

enum class Tool {
    Line,
    Rectangle,
    FilledRectangle,
    Circle,
    None
};

enum class LineMode {
    SingleColor,
    Gradient
};

struct AppState {
    Tool selectedTool = Tool::None;
    sf::Color borderColor = sf::Color::Black;
    sf::Color fillColor = sf::Color::White;
    float borderThickness = 5.0f;
    LineMode lineDrawingMode = LineMode::SingleColor;

    bool showToolOptions = false;
    bool isDrawing = false;
    sf::Vector2f startPoint = {0.0f, 0.0f};
    sf::Vector2f currentPoint = {0.0f, 0.0f};

    bool showExitConfirmation = false;
    bool showAbout = false;
    
    bool showOpenDialog = false;
    bool showOpenError = false;
    bool showSaveDialog = false;
    char filePathBuffer[256]; 

    std::string filePath = "drawing.png";
};
sf::Color ImGuiColorToSFML(const float color[3], int alpha = 255) {
    return sf::Color(
        static_cast<std::uint8_t>(color[0] * 255.f),
        static_cast<std::uint8_t>(color[1] * 255.f),
        static_cast<std::uint8_t>(color[2] * 255.f),
        static_cast<std::uint8_t>(alpha)
    );
}

void DrawTemporaryShape(sf::RenderWindow& window, const AppState& state) {
    if (!state.isDrawing || state.selectedTool == Tool::None) {
        return;
    }

    sf::Vector2f p1 = state.startPoint;
    sf::Vector2f p2 = state.currentPoint;

    switch (state.selectedTool) {
        case Tool::Line: {
            if (state.lineDrawingMode == LineMode::Gradient) {
                sf::Vertex line[] = {
                    sf::Vertex(p1, state.borderColor),
                    sf::Vertex(p2, state.fillColor) 
                };
                window.draw(line, 2, sf::PrimitiveType::Lines);
            } else {
                sf::Vector2f direction = p2 - p1;
                float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                float angle = std::atan2(direction.y, direction.x) * 180.0f / M_PI;

                sf::RectangleShape line(sf::Vector2f(length, state.borderThickness));
                line.setPosition(p1);
                // SFML 3.x: Use sf::degrees() for setRotation
                line.setRotation(sf::degrees(angle));
                line.setFillColor(state.borderColor);
                window.draw(line);
            }
            break;
        }
        case Tool::Rectangle:
        case Tool::FilledRectangle: {
            sf::RectangleShape rect;
            rect.setPosition(p1);
            rect.setSize(p2 - p1);
            rect.setOutlineColor(state.borderColor);
            rect.setOutlineThickness(state.borderThickness);
            
            if (state.selectedTool == Tool::FilledRectangle) {
                rect.setFillColor(state.fillColor);
            } else {
                rect.setFillColor(sf::Color::Transparent);
            }
            window.draw(rect);
            break;
        }
        case Tool::Circle: {
            float radius = std::sqrt(std::pow(p2.x - p1.x, 2) + std::pow(p2.y - p1.y, 2));
            sf::CircleShape circle(radius);
            // SFML 3.x: setOrigin takes a Vector2f
            circle.setOrigin({radius, radius}); 
            circle.setPosition(p1);
            circle.setOutlineColor(state.borderColor);
            circle.setOutlineThickness(state.borderThickness);
            circle.setFillColor(state.fillColor); 
            window.draw(circle);
            break;
        }
        default:
            break;
    }
}

void SaveFinalShape(sf::RenderTexture& canvas, const AppState& state) {
    sf::Vector2f p1 = state.startPoint;
    sf::Vector2f p2 = state.currentPoint;

    switch (state.selectedTool) {
        case Tool::Line: {
            if (state.lineDrawingMode == LineMode::Gradient) {
                sf::Vertex line[] = {
                    sf::Vertex(p1, state.borderColor),
                    sf::Vertex(p2, state.fillColor)
                };
                canvas.draw(line, 2, sf::PrimitiveType::Lines);
            } else {
                sf::Vector2f direction = p2 - p1;
                float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                float angle = std::atan2(direction.y, direction.x) * 180.0f / M_PI;

                sf::RectangleShape line(sf::Vector2f(length, state.borderThickness));
                line.setPosition(p1);
                line.setRotation(sf::degrees(angle));
                line.setFillColor(state.borderColor);
                canvas.draw(line);
            }
            break;
        }
        case Tool::Rectangle:
        case Tool::FilledRectangle: {
            sf::RectangleShape rect;
            rect.setPosition(p1);
            rect.setSize(p2 - p1);
            rect.setOutlineColor(state.borderColor);
            rect.setOutlineThickness(state.borderThickness);
            
            if (state.selectedTool == Tool::FilledRectangle) {
                rect.setFillColor(state.fillColor);
            } else {
                rect.setFillColor(sf::Color::Transparent);
            }
            canvas.draw(rect);
            break;
        }
        case Tool::Circle: {
            float radius = std::sqrt(std::pow(p2.x - p1.x, 2) + std::pow(p2.y - p1.y, 2));
            sf::CircleShape circle(radius);
            circle.setOrigin({radius, radius});
            circle.setPosition(p1);
            circle.setOutlineColor(state.borderColor);
            circle.setOutlineThickness(state.borderThickness);
            circle.setFillColor(state.fillColor);
            canvas.draw(circle);
            break;
        }
        default:
            break;
    }

    canvas.display();
}

void RenderImGui(sf::RenderWindow& window, sf::RenderTexture& canvas, AppState& state) {
    
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New")) {
                canvas.clear(sf::Color::White);
                canvas.display();
            }
            if (ImGui::MenuItem("Open...")) {
                state.showOpenDialog = true;
                std::strncpy(state.filePathBuffer, state.filePath.c_str(), sizeof(state.filePathBuffer) - 1);
                state.filePathBuffer[sizeof(state.filePathBuffer) - 1] = '\0';
            }
            if (ImGui::MenuItem("Save As...")) {
                state.showSaveDialog = true;
                std::strncpy(state.filePathBuffer, state.filePath.c_str(), sizeof(state.filePathBuffer) - 1);
                state.filePathBuffer[sizeof(state.filePathBuffer) - 1] = '\0';
            }
            if (ImGui::MenuItem("Exit")) {
                state.showExitConfirmation = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Show Tool Options", NULL, &state.showToolOptions);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                state.showAbout = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    ImGui::Begin("Tools");

    ImGui::Text("Drawing Tool:");
    ImGui::SameLine();
    if (ImGui::RadioButton("Line", state.selectedTool == Tool::Line)) state.selectedTool = Tool::Line;
    ImGui::SameLine();
    if (ImGui::RadioButton("Rect", state.selectedTool == Tool::Rectangle)) state.selectedTool = Tool::Rectangle;
    ImGui::SameLine();
    if (ImGui::RadioButton("FRect", state.selectedTool == Tool::FilledRectangle)) state.selectedTool = Tool::FilledRectangle;
    ImGui::SameLine();
    if (ImGui::RadioButton("Circle", state.selectedTool == Tool::Circle)) state.selectedTool = Tool::Circle;

    ImGui::Separator();

    float borderCol[3] = {state.borderColor.r / 255.f, state.borderColor.g / 255.f, state.borderColor.b / 255.f};
    if (ImGui::ColorEdit3("Border Color", borderCol)) {
        state.borderColor = ImGuiColorToSFML(borderCol);
    }
    float fillCol[3] = {state.fillColor.r / 255.f, state.fillColor.g / 255.f, state.fillColor.b / 255.f};
    if (ImGui::ColorEdit3("Fill Color", fillCol)) {
        state.fillColor = ImGuiColorToSFML(fillCol);
    }

    if (state.showToolOptions && state.selectedTool != Tool::None) {
        ImGui::Separator();
        
        if (state.selectedTool != Tool::FilledRectangle) { 
            ImGui::SliderFloat("Border/Line Thickness", &state.borderThickness, MIN_THICKNESS, MAX_THICKNESS, "%.1f px");
        }
        
        if (state.selectedTool == Tool::Line) {
            ImGui::Separator();
            ImGui::Text("Line Mode:");
            if (ImGui::RadioButton("Single Color", state.lineDrawingMode == LineMode::SingleColor)) state.lineDrawingMode = LineMode::SingleColor;
            ImGui::SameLine();
            if (ImGui::RadioButton("Gradient (Border->Fill)", state.lineDrawingMode == LineMode::Gradient)) state.lineDrawingMode = LineMode::Gradient;
        }
    }
    
    ImGui::End();

    if (state.showOpenDialog) {
        ImGui::OpenPopup("Open File");
        state.showOpenDialog = false;
    }
    if (ImGui::BeginPopupModal("Open File", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter relative file path to open:");
        ImGui::InputText("Path", state.filePathBuffer, sizeof(state.filePathBuffer));
        
        if (ImGui::Button("Open", ImVec2(120, 0))) {
            state.filePath = state.filePathBuffer;
            
            std::cout << "Attempting to open file: " << state.filePath << std::endl;
            sf::Image image;
            if (image.loadFromFile(state.filePath)) { 
                sf::Texture texture;
                texture.loadFromImage(image); 
                sf::Sprite sprite(texture);
                canvas.draw(sprite);
                canvas.display();
                std::cout << "Loaded image: " << state.filePath << std::endl;
            } else {
                std::cerr << "Failed to load image from: " << state.filePath << std::endl;
                state.showOpenError = true;
            }

            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (state.showSaveDialog) {
        ImGui::OpenPopup("Save File As");
        state.showSaveDialog = false;
    }
    if (ImGui::BeginPopupModal("Save File As", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter relative file path to save:");
        ImGui::InputText("Path", state.filePathBuffer, sizeof(state.filePathBuffer));
        
        if (ImGui::Button("Save", ImVec2(120, 0))) {
            state.filePath = state.filePathBuffer;

            std::cout << "Attempting to save file to: " << state.filePath << std::endl;
            sf::Image screenshot = canvas.getTexture().copyToImage();
            if (screenshot.saveToFile(state.filePath)) { 
                std::cout << "Saved image to: " << state.filePath << std::endl;
            } else {
                std::cerr << "Failed to save image to: " << state.filePath << std::endl;
            }

            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (state.showExitConfirmation) {
        ImGui::OpenPopup("Exit Program?");
        state.showExitConfirmation = false; 
    }
    if (ImGui::BeginPopupModal("Exit Program?", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Are you sure you want to exit?");
        if (ImGui::Button("Yes", ImVec2(120, 0))) {
            window.close();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("No", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (state.showAbout) {
        ImGui::OpenPopup("About");
        state.showAbout = false; 
    }
    if (ImGui::BeginPopupModal("About", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Pracownia Komputerowa: Interfejsy Użytkownika i Biblioteki Graficzne");
        ImGui::Text("Zadanie: Biblioteka SFML + ImGui");
        ImGui::Text("Autor: Dominik Godek");
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (state.showOpenError) {
        ImGui::OpenPopup("File Open Error");
        state.showOpenError = false; 
    }
    if (ImGui::BeginPopupModal("File Open Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "ERROR: Failed to load file!");
        ImGui::Text("The file '%s' could not be opened, found, or is not a valid image format.", state.filePath.c_str());
        
        if (ImGui::Button("Acknowledge", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

int main() {
    sf::RenderWindow window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "SFML + ImGui Drawing Tool", sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window)) {
        std::cerr << "Failed to initialize ImGui-SFML!" << std::endl;
        return -1;
    }

    sf::RenderTexture canvas({WINDOW_WIDTH, WINDOW_HEIGHT});
    
    canvas.clear(sf::Color::White); // Start with a clean white canvas
    canvas.display();

    AppState state;
    std::strncpy(state.filePathBuffer, state.filePath.c_str(), sizeof(state.filePathBuffer) - 1);
    state.filePathBuffer[sizeof(state.filePathBuffer) - 1] = '\0';

    sf::Clock clock;

    while (window.isOpen()) {
        
        while (auto event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            bool mouseOnCanvas = !ImGui::GetIO().WantCaptureMouse;
            
            if (mouseOnCanvas) {
                if (auto mousePress = event->getIf<sf::Event::MouseButtonPressed>()) {
                    if (mousePress->button == sf::Mouse::Button::Left && state.selectedTool != Tool::None) {
                        state.isDrawing = true;
                        state.startPoint = window.mapPixelToCoords(mousePress->position);
                        state.currentPoint = state.startPoint;
                    }
                } 
                else if (auto mouseRelease = event->getIf<sf::Event::MouseButtonReleased>()) {
                    if (mouseRelease->button == sf::Mouse::Button::Left && state.isDrawing) {
                        state.isDrawing = false;
                        SaveFinalShape(canvas, state);
                    }
                } 
                else if (auto mouseMove = event->getIf<sf::Event::MouseMoved>()) {
                    if (state.isDrawing) {
                        state.currentPoint = window.mapPixelToCoords(mouseMove->position);
                    }
                }
            }
        }

        sf::Time deltaTime = clock.restart();
        ImGui::SFML::Update(window, deltaTime);

        RenderImGui(window, canvas, state);

        window.clear();

        window.draw(sf::Sprite(canvas.getTexture()));

        DrawTemporaryShape(window, state);

        ImGui::SFML::Render(window);

        window.display();
    }

    ImGui::SFML::Shutdown();

    return 0;
}