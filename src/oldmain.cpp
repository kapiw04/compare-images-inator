#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Angle.hpp> // Required for sf::degrees
#include <SFML/System/Vector2.hpp> // Required for sf::Vector2u
#include "imgui.h"
#include "imgui-SFML.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdint> // Required for std::uint8_t

// --- Configuration Constants ---
constexpr unsigned int WINDOW_WIDTH = 1800;
constexpr unsigned int WINDOW_HEIGHT = 900;
constexpr float MIN_THICKNESS = 1.0f;
constexpr float MAX_THICKNESS = 50.0f;

// --- Enums for State Management ---
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

// --- Application State Structure ---
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

    // NOTE: Dummy path for file operations
    std::string filePath = "drawing.png";
};

// --- Helper Functions ---

/**
 * @brief Converts an ImGui Color (float[3]) to an SFML Color.
 * SFML 3.x uses std::uint8_t instead of sf::Uint8.
 */
sf::Color ImGuiColorToSFML(const float color[3], int alpha = 255) {
    return sf::Color(
        static_cast<std::uint8_t>(color[0] * 255.f),
        static_cast<std::uint8_t>(color[1] * 255.f),
        static_cast<std::uint8_t>(color[2] * 255.f),
        static_cast<std::uint8_t>(alpha)
    );
}

/**
 * @brief Draws the currently selected shape (temporary) to the main window.
 */
void DrawTemporaryShape(sf::RenderWindow& window, const AppState& state) {
    if (!state.isDrawing || state.selectedTool == Tool::None) {
        return;
    }

    sf::Vector2f p1 = state.startPoint;
    sf::Vector2f p2 = state.currentPoint;

    switch (state.selectedTool) {
        case Tool::Line: {
            if (state.lineDrawingMode == LineMode::Gradient) {
                // SFML 3.x: Use sf::PrimitiveType::Lines
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

/**
 * @brief Saves the final shape to the sf::RenderTexture (the permanent canvas).
 */
void SaveFinalShape(sf::RenderTexture& canvas, const AppState& state) {
    sf::Vector2f p1 = state.startPoint;
    sf::Vector2f p2 = state.currentPoint;

    switch (state.selectedTool) {
        case Tool::Line: {
            if (state.lineDrawingMode == LineMode::Gradient) {
                 // SFML 3.x: Use sf::PrimitiveType::Lines
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
                // SFML 3.x: Use sf::degrees() for setRotation
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
            // SFML 3.x: setOrigin takes a Vector2f
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

/**
 * @brief Renders the ImGui interface (Menu Bar, Tools Window, Modals).
 */
void RenderImGui(sf::RenderWindow& window, sf::RenderTexture& canvas, AppState& state) {
    
    // --- Main Menu Bar ---
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New")) {
                canvas.clear(sf::Color::White);
                canvas.display();
            }
            if (ImGui::MenuItem("Open...")) {
                std::cout << "Opening file dialog..." << std::endl;
                sf::Image image;
                if (image.loadFromFile(state.filePath)) { 
                    sf::Texture texture;
                    texture.loadFromImage(image); 
                    sf::Sprite sprite(texture);
                    canvas.draw(sprite);
                    canvas.display();
                    std::cout << "Loaded image: " << state.filePath << std::endl;
                } else {
                    std::cerr << "Failed to load image." << std::endl;
                }
            }
            if (ImGui::MenuItem("Save As...")) {
                std::cout << "Saving file dialog..." << std::endl;
                sf::Image screenshot = canvas.getTexture().copyToImage();
                if (screenshot.saveToFile(state.filePath)) { 
                    std::cout << "Saved image to: " << state.filePath << std::endl;
                } else {
                    std::cerr << "Failed to save image." << std::endl;
                }
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

    // --- Tools Window ---
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

    // Color Pickers
    float borderCol[3] = {state.borderColor.r / 255.f, state.borderColor.g / 255.f, state.borderColor.b / 255.f};
    if (ImGui::ColorEdit3("Border Color", borderCol)) {
        state.borderColor = ImGuiColorToSFML(borderCol);
    }
    float fillCol[3] = {state.fillColor.r / 255.f, state.fillColor.g / 255.f, state.fillColor.b / 255.f};
    if (ImGui::ColorEdit3("Fill Color", fillCol)) {
        state.fillColor = ImGuiColorToSFML(fillCol);
    }

    // Conditional Tool Options
    if (state.showToolOptions && state.selectedTool != Tool::None) {
        ImGui::Separator();
        
        // Thickness Slider
        if (state.selectedTool != Tool::FilledRectangle) { 
            ImGui::SliderFloat("Border/Line Thickness", &state.borderThickness, MIN_THICKNESS, MAX_THICKNESS, "%.1f px");
        }
        
        // Line Drawing Mode Options
        if (state.selectedTool == Tool::Line) {
            ImGui::Separator();
            ImGui::Text("Line Mode:");
            if (ImGui::RadioButton("Single Color", state.lineDrawingMode == LineMode::SingleColor)) state.lineDrawingMode = LineMode::SingleColor;
            ImGui::SameLine();
            if (ImGui::RadioButton("Gradient (Border->Fill)", state.lineDrawingMode == LineMode::Gradient)) state.lineDrawingMode = LineMode::Gradient;
        }
    }
    
    ImGui::End();

    // --- Modal Dialogs ---

    // Exit Confirmation
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

    // About Window
    if (state.showAbout) {
        ImGui::OpenPopup("About");
        state.showAbout = false; 
    }
    if (ImGui::BeginPopupModal("About", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Pracownia Komputerowa: Interfejsy Użytkownika i Biblioteki Graficzne");
        ImGui::Text("Zadanie: Biblioteka SFML + ImGui");
        ImGui::Text("Autor: Janusz Malinowski (implemented by AI Assistant)");
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

/**
 * @brief Main function to initialize and run the application loop.
 */
int main() {
    // 1. Initialization
    // SFML 3.x: sf::VideoMode now takes a Vector2u
    sf::RenderWindow window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "SFML + ImGui Drawing Tool", sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    // Initialize ImGui-SFML
    if (!ImGui::SFML::Init(window)) {
        std::cerr << "Failed to initialize ImGui-SFML!" << std::endl;
        return -1;
    }

    // Create the Permanent Canvas (RenderTexture)
    // SFML 3.x: sf::RenderTexture::create is removed. Initialize via constructor.
    sf::RenderTexture canvas({WINDOW_WIDTH, WINDOW_HEIGHT});
    // Removed: if (!canvas.isAvailable()) check
    
    canvas.clear(sf::Color::White); // Start with a clean white canvas
    canvas.display();

    AppState state;
    sf::Clock clock;

    // 2. Main Loop
    while (window.isOpen()) {
        
        // SFML 3.x: sf::Window::pollEvent() now returns an optional<Event>
        while (auto event = window.pollEvent()) {
            // Process ImGui events first
            ImGui::SFML::ProcessEvent(window, *event);

            // SFML 3.x: Checking for event type with is<Type>()
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            // --- Mouse Interaction on Canvas ---
            bool mouseOnCanvas = !ImGui::GetIO().WantCaptureMouse;
            
            if (mouseOnCanvas) {
                // Handle Mouse Button Pressed
                if (auto mousePress = event->getIf<sf::Event::MouseButtonPressed>()) {
                    // SFML 3.x: sf::Mouse::Left is now sf::Mouse::Button::Left
                    if (mousePress->button == sf::Mouse::Button::Left && state.selectedTool != Tool::None) {
                        state.isDrawing = true;
                        // SFML 3.x: Event coordinates are now accessed via the 'position' member (Vector2i)
                        state.startPoint = window.mapPixelToCoords(mousePress->position);
                        state.currentPoint = state.startPoint;
                    }
                } 
                // Handle Mouse Button Released
                else if (auto mouseRelease = event->getIf<sf::Event::MouseButtonReleased>()) {
                    // SFML 3.x: sf::Mouse::Left is now sf::Mouse::Button::Left
                    if (mouseRelease->button == sf::Mouse::Button::Left && state.isDrawing) {
                        state.isDrawing = false;
                        SaveFinalShape(canvas, state);
                    }
                } 
                // Handle Mouse Moved
                else if (auto mouseMove = event->getIf<sf::Event::MouseMoved>()) {
                    if (state.isDrawing) {
                        // SFML 3.x: Event coordinates are now accessed via the 'position' member (Vector2i)
                        state.currentPoint = window.mapPixelToCoords(mouseMove->position);
                    }
                }
            }
        }

        // ImGui Update
        sf::Time deltaTime = clock.restart();
        ImGui::SFML::Update(window, deltaTime);

        // Build and render ImGui components
        RenderImGui(window, canvas, state);

        // --- Drawing ---
        window.clear();

        // 1. Draw Permanent Canvas Content
        window.draw(sf::Sprite(canvas.getTexture()));

        // 2. Draw Temporary Shape
        DrawTemporaryShape(window, state);

        // 3. Draw ImGui Interface
        ImGui::SFML::Render(window);

        window.display();
    }

    // 3. Cleanup
    ImGui::SFML::Shutdown();

    return 0;
}

