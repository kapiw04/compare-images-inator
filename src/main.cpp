#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>
#include "imgui.h"
#include "imgui-SFML.h"
#include <iostream>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

    bool showToolOptions = true; 
    bool isDrawing = false;
    sf::Vector2f startPoint = {0.0f, 0.0f};
    sf::Vector2f currentPoint = {0.0f, 0.0f};

    bool showExitConfirmation = false;
    bool showAbout = false;
    bool showOpenDialog = false;
    bool showOpenError = false;
    bool showSaveDialog = false;

    char filePathBuffer[256] = "drawing.png"; 
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


class DrawingApp {
public:
    DrawingApp();
    void run();

private:
    sf::RenderWindow m_window;
    sf::RenderTexture m_canvas;
    sf::Clock m_clock;
    AppState m_state;

    void processEvents();
    void update(sf::Time deltaTime);
    void render();

    void drawShape(sf::RenderTarget& target);
    void saveFinalShape();

    void renderImGui();
    void renderMenuBar();
    void renderToolPanel();
    void renderOpenDialog();
    void renderSaveDialog();
    void renderExitConfirmation();
    void renderAboutModal();
    void renderOpenErrorModal();

    void openFile();
    void saveFile();
};


DrawingApp::DrawingApp()
    : m_window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Paint", sf::Style::Titlebar | sf::Style::Close),
      m_canvas(sf::RenderTexture({WINDOW_WIDTH, WINDOW_HEIGHT}))
{
    m_window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(m_window)) {
        std::cerr << "Failed to initialize ImGui-SFML!" << std::endl;
    }

    m_canvas.clear(sf::Color::White);
    m_canvas.display();

    std::strncpy(m_state.filePathBuffer, m_state.filePath.c_str(), sizeof(m_state.filePathBuffer) - 1);
    m_state.filePathBuffer[sizeof(m_state.filePathBuffer) - 1] = '\0';
}

void DrawingApp::run() {
    while (m_window.isOpen()) {
        processEvents();

        sf::Time deltaTime = m_clock.restart();
        update(deltaTime);

        render();
    }

    ImGui::SFML::Shutdown();
}

void DrawingApp::processEvents() {
    while (auto event = m_window.pollEvent()) {
        ImGui::SFML::ProcessEvent(m_window, *event);

        if (event->is<sf::Event::Closed>()) {
            m_window.close();
        }

        bool mouseOnCanvas = !ImGui::GetIO().WantCaptureMouse;
        
        if (mouseOnCanvas) {
            if (auto mousePress = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mousePress->button == sf::Mouse::Button::Left && m_state.selectedTool != Tool::None) {
                    m_state.isDrawing = true;
                    m_state.startPoint = m_window.mapPixelToCoords(mousePress->position);
                    m_state.currentPoint = m_state.startPoint;
                }
            }
            else if (auto mouseRelease = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (mouseRelease->button == sf::Mouse::Button::Left && m_state.isDrawing) {
                    m_state.isDrawing = false;
                    saveFinalShape();
                }
            }
            else if (auto mouseMove = event->getIf<sf::Event::MouseMoved>()) {
                if (m_state.isDrawing) {
                    m_state.currentPoint = m_window.mapPixelToCoords(mouseMove->position);
                }
            }
        }
    }
}

void DrawingApp::update(sf::Time deltaTime) {
    ImGui::SFML::Update(m_window, deltaTime);
}

void DrawingApp::drawShape(sf::RenderTarget& target) {
    if (m_state.selectedTool == Tool::None) {
        return;
    }

    sf::Vector2f p1 = m_state.startPoint;
    sf::Vector2f p2 = m_state.currentPoint;

    float minX = std::min(p1.x, p2.x);
    float minY = std::min(p1.y, p2.y);
    float width = std::abs(p2.x - p1.x);
    float height = std::abs(p2.y - p1.y);
    float radius = std::sqrt(std::pow(p2.x - p1.x, 2) + std::pow(p2.y - p1.y, 2));

    switch (m_state.selectedTool) {
        case Tool::Line: {
            if (m_state.lineDrawingMode == LineMode::Gradient) {
                sf::Vertex line[] = {
                    sf::Vertex(p1, m_state.borderColor),
                    sf::Vertex(p2, m_state.fillColor)
                };
                target.draw(line, 2, sf::PrimitiveType::Lines);
            } else {
                sf::Vector2f direction = p2 - p1;
                float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                float angle = std::atan2(direction.y, direction.x) * 180.0f / M_PI;

                sf::RectangleShape line(sf::Vector2f(length, m_state.borderThickness));
                line.setPosition(p1);
                line.setRotation(sf::degrees(angle));
                line.setFillColor(m_state.borderColor);
                target.draw(line);
            }
            break;
        }
        case Tool::Rectangle:
        case Tool::FilledRectangle: {
            sf::RectangleShape rect(sf::Vector2f(width, height));
            rect.setPosition({minX, minY});
            rect.setOutlineColor(m_state.borderColor);
            rect.setOutlineThickness(m_state.borderThickness);
            
            if (m_state.selectedTool == Tool::FilledRectangle) {
                rect.setFillColor(m_state.fillColor);
            } else {
                rect.setFillColor(sf::Color::Transparent);
                rect.setPosition({minX + m_state.borderThickness / 2.0f, minY + m_state.borderThickness / 2.0f});
                rect.setSize({width - m_state.borderThickness, height - m_state.borderThickness});
            }
            target.draw(rect);
            break;
        }
        case Tool::Circle: {
            sf::CircleShape circle(radius);
            circle.setOrigin({radius, radius}); 
            circle.setPosition(p1);
            circle.setOutlineColor(m_state.borderColor);
            circle.setOutlineThickness(m_state.borderThickness);
            circle.setFillColor(m_state.fillColor);
            target.draw(circle);
            break;
        }
        default:
            break;
    }
}

void DrawingApp::saveFinalShape() {
    if (m_state.selectedTool != Tool::None) {
        drawShape(m_canvas);
        m_canvas.display();
    }
}

void DrawingApp::openFile() {
    m_state.filePath = m_state.filePathBuffer;
    
    std::cout << "Attempting to open file: " << m_state.filePath << std::endl;
    sf::Image image;
    if (image.loadFromFile(m_state.filePath)) {
        sf::Texture texture;
        if (texture.loadFromImage(image)) {
            m_canvas.clear(sf::Color::White); // Clear before drawing new content
            sf::Sprite sprite(texture);
            m_canvas.draw(sprite);
            m_canvas.display();
            std::cout << "Loaded image: " << m_state.filePath << std::endl;
        } else {
             std::cerr << "Failed to load texture from image, possibly size mismatch or corrupted data: " << m_state.filePath << std::endl;
             m_state.showOpenError = true;
        }
    } else {
        std::cerr << "Failed to load image from: " << m_state.filePath << std::endl;
        m_state.showOpenError = true;
    }
}

void DrawingApp::saveFile() {
    m_state.filePath = m_state.filePathBuffer;

    std::cout << "Attempting to save file to: " << m_state.filePath << std::endl;
    sf::Image screenshot = m_canvas.getTexture().copyToImage();
    if (screenshot.saveToFile(m_state.filePath)) {
        std::cout << "Saved image to: " << m_state.filePath << std::endl;
    } else {
        std::cerr << "Failed to save image to: " << m_state.filePath << std::endl;
    }
}


void DrawingApp::renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New")) {
                m_canvas.clear(sf::Color::White);
                m_canvas.display();
            }
            if (ImGui::MenuItem("Open...")) {
                m_state.showOpenDialog = true;
                std::strncpy(m_state.filePathBuffer, m_state.filePath.c_str(), sizeof(m_state.filePathBuffer) - 1);
                m_state.filePathBuffer[sizeof(m_state.filePathBuffer) - 1] = '\0';
            }
            if (ImGui::MenuItem("Save As...")) {
                m_state.showSaveDialog = true;
                std::strncpy(m_state.filePathBuffer, m_state.filePath.c_str(), sizeof(m_state.filePathBuffer) - 1);
                m_state.filePathBuffer[sizeof(m_state.filePathBuffer) - 1] = '\0';
            }
            if (ImGui::MenuItem("Exit")) {
                m_state.showExitConfirmation = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Show Tool Options", NULL, &m_state.showToolOptions);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                m_state.showAbout = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void DrawingApp::renderToolPanel() {
    ImGui::Begin("Tools and Colors");

    ImGui::Text("Drawing Tool:");
    ImGui::SameLine();
    if (ImGui::RadioButton("Line", m_state.selectedTool == Tool::Line)) m_state.selectedTool = Tool::Line;
    ImGui::SameLine();
    if (ImGui::RadioButton("Rect", m_state.selectedTool == Tool::Rectangle)) m_state.selectedTool = Tool::Rectangle;
    ImGui::SameLine();
    if (ImGui::RadioButton("F. Rect", m_state.selectedTool == Tool::FilledRectangle)) m_state.selectedTool = Tool::FilledRectangle;
    ImGui::SameLine();
    if (ImGui::RadioButton("Circle", m_state.selectedTool == Tool::Circle)) m_state.selectedTool = Tool::Circle;

    ImGui::Separator();

    float borderCol[3] = {m_state.borderColor.r / 255.f, m_state.borderColor.g / 255.f, m_state.borderColor.b / 255.f};
    if (ImGui::ColorEdit3("Border Color", borderCol)) {
        m_state.borderColor = ImGuiColorToSFML(borderCol);
    }
    
    float fillCol[3] = {m_state.fillColor.r / 255.f, m_state.fillColor.g / 255.f, m_state.fillColor.b / 255.f};
    if (ImGui::ColorEdit3("Fill Color", fillCol)) {
        m_state.fillColor = ImGuiColorToSFML(fillCol);
    }

    if (m_state.showToolOptions && m_state.selectedTool != Tool::None) {
        ImGui::Separator();
        
        if (m_state.selectedTool != Tool::FilledRectangle) {
             ImGui::SliderFloat("Border/Line Thickness", &m_state.borderThickness, MIN_THICKNESS, MAX_THICKNESS, "%.1f px");
        } else {
             ImGui::SliderFloat("Outline Thickness", &m_state.borderThickness, MIN_THICKNESS, MAX_THICKNESS, "%.1f px");
        }

        if (m_state.selectedTool == Tool::Line) {
            ImGui::Separator();
            ImGui::Text("Line Mode:");
            if (ImGui::RadioButton("Single Color", m_state.lineDrawingMode == LineMode::SingleColor)) m_state.lineDrawingMode = LineMode::SingleColor;
            ImGui::SameLine();
            if (ImGui::RadioButton("Gradient (Border->Fill)", m_state.lineDrawingMode == LineMode::Gradient)) m_state.lineDrawingMode = LineMode::Gradient;
        }
    }
    
    ImGui::End();
}

void DrawingApp::renderOpenDialog() {
    if (m_state.showOpenDialog) {
        ImGui::OpenPopup("Open File");
        m_state.showOpenDialog = false;
    }
    if (ImGui::BeginPopupModal("Open File", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter relative file path to open:");
        ImGui::InputText("Path", m_state.filePathBuffer, sizeof(m_state.filePathBuffer));
        
        if (ImGui::Button("Open", ImVec2(120, 0))) {
            openFile();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void DrawingApp::renderSaveDialog() {
    if (m_state.showSaveDialog) {
        ImGui::OpenPopup("Save File As");
        m_state.showSaveDialog = false;
    }
    if (ImGui::BeginPopupModal("Save File As", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter relative file path to save:");
        ImGui::InputText("Path", m_state.filePathBuffer, sizeof(m_state.filePathBuffer));
        
        if (ImGui::Button("Save", ImVec2(120, 0))) {
            saveFile();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void DrawingApp::renderExitConfirmation() {
    if (m_state.showExitConfirmation) {
        ImGui::OpenPopup("Exit Program?");
        m_state.showExitConfirmation = false;
    }
    if (ImGui::BeginPopupModal("Exit Program?", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Are you sure you want to exit?");
        if (ImGui::Button("Yes", ImVec2(120, 0))) {
            m_window.close();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("No", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void DrawingApp::renderAboutModal() {
    if (m_state.showAbout) {
        ImGui::OpenPopup("About");
        m_state.showAbout = false;
    }
    if (ImGui::BeginPopupModal("About", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Paint tool");
        ImGui::Separator();
        ImGui::Text("Author: Dominik Godek");
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void DrawingApp::renderOpenErrorModal() {
    if (m_state.showOpenError) {
        ImGui::OpenPopup("File Open Error");
        m_state.showOpenError = false;
    }
    if (ImGui::BeginPopupModal("File Open Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "ERROR: Failed to load file!");
        ImGui::Text("The file '%s' could not be opened, found, or is not a valid image format.", m_state.filePath.c_str());
        
        if (ImGui::Button("Acknowledge", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void DrawingApp::renderImGui() {
    renderMenuBar();
    renderToolPanel();
    
    // Render Modals
    renderOpenDialog();
    renderSaveDialog();
    renderExitConfirmation();
    renderAboutModal();
    renderOpenErrorModal();
}

void DrawingApp::render() {
    m_window.clear();

    m_window.draw(sf::Sprite(m_canvas.getTexture()));

    if (m_state.isDrawing) {
        drawShape(m_window);
    }
    
    renderImGui();
    ImGui::SFML::Render(m_window);

    m_window.display();
}


int main() {
    DrawingApp app;
    app.run();
    return 0;
}