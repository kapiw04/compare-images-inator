#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include "imgui.h"
#include "imgui-SFML.h"
#include <string>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <fstream>

// Application state
struct AppState {
    // Images
    sf::Image image1;
    sf::Image image2;
    sf::Image diffImage;
    
    sf::Texture texture1;
    sf::Texture texture2;
    sf::Texture diffTexture;
    
    bool image1Loaded = false;
    bool image2Loaded = false;
    bool diffImageGenerated = false;
    
    // File paths
    char filePath1[512] = "";
    char filePath2[512] = "";
    char savePathDiff[512] = "difference.bmp";
    
    // View settings
    int zoomLevel = 2; // Index: 0=50%, 1=100%, 2=200%, 3=400%
    float zoomFactors[4] = {0.5f, 1.0f, 2.0f, 4.0f};
    
    // Pan offset (synchronized for both images)
    sf::Vector2f panOffset = {0.0f, 0.0f};
    bool isPanning = false;
    sf::Vector2i lastMousePos;
    
    // Status message
    std::string statusMessage = "Load two BMP images to compare";
};

// Load an image from file
bool loadImage(const std::string& path, sf::Image& image, sf::Texture& texture, std::string& statusMessage) {
    // Validate path is not empty
    if (path.empty()) {
        statusMessage = "Error: Please enter a file path first";
        return false;
    }
    
    if (!image.loadFromFile(path)) {
        statusMessage = "Failed to load image: " + path;
        return false;
    }
    
    if (!texture.loadFromImage(image)) {
        statusMessage = "Failed to create texture from image: " + path;
        return false;
    }
    
    statusMessage = "Loaded: " + path;
    return true;
}

// Generate difference image
void generateDifferenceImage(AppState& state) {
    if (!state.image1Loaded || !state.image2Loaded) {
        state.statusMessage = "Load both images first!";
        return;
    }
    
    sf::Vector2u size1 = state.image1.getSize();
    sf::Vector2u size2 = state.image2.getSize();
    
    // Use the smaller dimensions
    unsigned int width = std::min(size1.x, size2.x);
    unsigned int height = std::min(size1.y, size2.y);
    
    if (width == 0 || height == 0) {
        state.statusMessage = "Invalid image dimensions!";
        return;
    }
    
    // Create difference image
    state.diffImage.resize({width, height});
    
    for (unsigned int y = 0; y < height; ++y) {
        for (unsigned int x = 0; x < width; ++x) {
            sf::Color c1 = state.image1.getPixel({x, y});
            sf::Color c2 = state.image2.getPixel({x, y});
            
            // Calculate absolute difference for each channel
            uint8_t r = static_cast<uint8_t>(std::abs(static_cast<int>(c1.r) - static_cast<int>(c2.r)));
            uint8_t g = static_cast<uint8_t>(std::abs(static_cast<int>(c1.g) - static_cast<int>(c2.g)));
            uint8_t b = static_cast<uint8_t>(std::abs(static_cast<int>(c1.b) - static_cast<int>(c2.b)));
            
            state.diffImage.setPixel({x, y}, sf::Color(r, g, b, 255));
        }
    }
    
    if (!state.diffTexture.loadFromImage(state.diffImage)) {
        state.statusMessage = "Failed to create difference texture!";
        return;
    }
    
    state.diffImageGenerated = true;
    state.statusMessage = "Difference image generated! See popup window.";
}

// Save difference image to BMP file
bool saveDifferenceImage(AppState& state) {
    if (!state.diffImageGenerated) {
        state.statusMessage = "Generate difference image first!";
        return false;
    }
    
    std::string path = state.savePathDiff;
    if (path.empty()) {
        path = "difference.bmp";
    }
    
    // Ensure .bmp extension
    if (path.length() < 4 || path.substr(path.length() - 4) != ".bmp") {
        path += ".bmp";
    }
    
    if (!state.diffImage.saveToFile(path)) {
        state.statusMessage = "Failed to save difference image!";
        return false;
    }
    
    state.statusMessage = "Saved difference image to: " + path;
    return true;
}

// Render an image in an ImGui child window with zoom and pan
void renderImageView(const char* label, sf::Texture& texture, bool loaded, 
                     float zoom, const sf::Vector2f& panOffset, float viewWidth, float viewHeight) {
    ImGui::BeginChild(label, ImVec2(viewWidth, viewHeight), true, 
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    
    if (loaded) {
        sf::Vector2u texSize = texture.getSize();
        float scaledWidth = texSize.x * zoom;
        float scaledHeight = texSize.y * zoom;
        
        // Set cursor position based on pan offset
        ImGui::SetCursorPos(ImVec2(panOffset.x, panOffset.y));
        
        // Convert SFML texture to ImGui texture
        ImTextureID texId = static_cast<ImTextureID>(static_cast<uintptr_t>(texture.getNativeHandle()));
        ImGui::Image(texId, ImVec2(scaledWidth, scaledHeight));
    } else {
        ImGui::Text("No image loaded");
        ImGui::Text("Enter path and click Load");
    }
    
    ImGui::EndChild();
}

int main() {
    sf::RenderWindow window(sf::VideoMode({1280, 800}), "Compare Images - Image Comparison Tool");
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window)) {
        return -1;
    }

    AppState state;
    sf::Clock deltaClock;
    
    while (window.isOpen()) {
        while (auto event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);
            
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            
            // Handle mouse wheel for zooming
            if (auto* scrollEvent = event->getIf<sf::Event::MouseWheelScrolled>()) {
                if (!ImGui::GetIO().WantCaptureMouse) {
                    if (scrollEvent->delta > 0 && state.zoomLevel < 3) {
                        state.zoomLevel++;
                    } else if (scrollEvent->delta < 0 && state.zoomLevel > 0) {
                        state.zoomLevel--;
                    }
                }
            }
            
            // Handle mouse drag for panning
            if (auto* buttonEvent = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (buttonEvent->button == sf::Mouse::Button::Middle ||
                    buttonEvent->button == sf::Mouse::Button::Right) {
                    state.isPanning = true;
                    state.lastMousePos = sf::Mouse::getPosition(window);
                }
            }
            
            if (auto* buttonEvent = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (buttonEvent->button == sf::Mouse::Button::Middle ||
                    buttonEvent->button == sf::Mouse::Button::Right) {
                    state.isPanning = false;
                }
            }
            
            if (event->is<sf::Event::MouseMoved>()) {
                if (state.isPanning) {
                    sf::Vector2i currentPos = sf::Mouse::getPosition(window);
                    sf::Vector2i delta = currentPos - state.lastMousePos;
                    state.panOffset.x += delta.x;
                    state.panOffset.y += delta.y;
                    state.lastMousePos = currentPos;
                }
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        // Main control panel
        ImGui::Begin("Control Panel", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        
        // Image 1 loading
        ImGui::Text("Image 1 (Left):");
        ImGui::PushID("img1");
        ImGui::InputText("Path", state.filePath1, sizeof(state.filePath1));
        if (ImGui::Button("Load BMP")) {
            if (loadImage(state.filePath1, state.image1, state.texture1, state.statusMessage)) {
                state.image1Loaded = true;
                state.diffImageGenerated = false;
            }
        }
        ImGui::PopID();
        if (state.image1Loaded) {
            sf::Vector2u size = state.image1.getSize();
            ImGui::SameLine();
            ImGui::Text("(%ux%u)", size.x, size.y);
        }
        
        ImGui::Separator();
        
        // Image 2 loading
        ImGui::Text("Image 2 (Right):");
        ImGui::PushID("img2");
        ImGui::InputText("Path", state.filePath2, sizeof(state.filePath2));
        if (ImGui::Button("Load BMP")) {
            if (loadImage(state.filePath2, state.image2, state.texture2, state.statusMessage)) {
                state.image2Loaded = true;
                state.diffImageGenerated = false;
            }
        }
        ImGui::PopID();
        if (state.image2Loaded) {
            sf::Vector2u size = state.image2.getSize();
            ImGui::SameLine();
            ImGui::Text("(%ux%u)", size.x, size.y);
        }
        
        ImGui::Separator();
        
        // Zoom controls
        ImGui::Text("Zoom Level:");
        const char* zoomLabels[] = {"50%", "100%", "200%", "400%"};
        for (int i = 0; i < 4; ++i) {
            if (i > 0) ImGui::SameLine();
            if (ImGui::RadioButton(zoomLabels[i], state.zoomLevel == i)) {
                state.zoomLevel = i;
            }
        }
        ImGui::Text("(Use mouse wheel to zoom, right-click drag to pan)");
        
        ImGui::Separator();
        
        // Difference image controls
        ImGui::Text("Difference Image:");
        if (ImGui::Button("Generate Difference")) {
            generateDifferenceImage(state);
        }
        
        ImGui::InputText("Save Path", state.savePathDiff, sizeof(state.savePathDiff));
        if (ImGui::Button("Save Difference as BMP")) {
            saveDifferenceImage(state);
        }
        
        // Reset pan button
        ImGui::Separator();
        if (ImGui::Button("Reset Pan")) {
            state.panOffset = {0.0f, 0.0f};
        }
        
        ImGui::Separator();
        
        // Status message
        ImGui::TextWrapped("Status: %s", state.statusMessage.c_str());
        
        ImGui::End();

        // Get window size for image views
        sf::Vector2u windowSize = window.getSize();
        float panelWidth = 350.0f;
        float availableWidth = windowSize.x - panelWidth - 30.0f;
        float viewWidth = availableWidth / 2.0f - 10.0f;
        float viewHeight = windowSize.y - 20.0f;
        float currentZoom = state.zoomFactors[state.zoomLevel];
        
        // Image comparison view
        ImGui::SetNextWindowPos(ImVec2(panelWidth + 10, 10));
        ImGui::SetNextWindowSize(ImVec2(availableWidth + 10, viewHeight));
        ImGui::Begin("Image Comparison", nullptr, 
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
        
        // Split view with vertical divider
        ImGui::BeginChild("LeftPane", ImVec2(viewWidth, viewHeight - 40), true,
                          ImGuiWindowFlags_HorizontalScrollbar);
        if (state.image1Loaded) {
            sf::Vector2u texSize = state.texture1.getSize();
            float scaledWidth = texSize.x * currentZoom;
            float scaledHeight = texSize.y * currentZoom;
            
            ImGui::SetCursorPos(ImVec2(state.panOffset.x + 5, state.panOffset.y + 5));
            ImTextureID texId = static_cast<ImTextureID>(static_cast<uintptr_t>(state.texture1.getNativeHandle()));
            ImGui::Image(texId, ImVec2(scaledWidth, scaledHeight));
        } else {
            ImGui::Text("Image 1");
            ImGui::Text("No image loaded");
        }
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // Vertical divider line (represented by same-line layout)
        ImGui::BeginChild("RightPane", ImVec2(viewWidth, viewHeight - 40), true,
                          ImGuiWindowFlags_HorizontalScrollbar);
        if (state.image2Loaded) {
            sf::Vector2u texSize = state.texture2.getSize();
            float scaledWidth = texSize.x * currentZoom;
            float scaledHeight = texSize.y * currentZoom;
            
            ImGui::SetCursorPos(ImVec2(state.panOffset.x + 5, state.panOffset.y + 5));
            ImTextureID texId = static_cast<ImTextureID>(static_cast<uintptr_t>(state.texture2.getNativeHandle()));
            ImGui::Image(texId, ImVec2(scaledWidth, scaledHeight));
        } else {
            ImGui::Text("Image 2");
            ImGui::Text("No image loaded");
        }
        ImGui::EndChild();
        
        ImGui::End();
        
        // Difference image window (shown when generated)
        if (state.diffImageGenerated) {
            // Center the window on screen
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
            ImGui::SetNextWindowFocus();
            ImGui::Begin("Difference Image", &state.diffImageGenerated);
            
            sf::Vector2u texSize = state.diffTexture.getSize();
            float scaledWidth = texSize.x * currentZoom;
            float scaledHeight = texSize.y * currentZoom;
            
            ImGui::BeginChild("DiffView", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::SetCursorPos(ImVec2(state.panOffset.x + 5, state.panOffset.y + 5));
            ImTextureID texId = static_cast<ImTextureID>(static_cast<uintptr_t>(state.diffTexture.getNativeHandle()));
            ImGui::Image(texId, ImVec2(scaledWidth, scaledHeight));
            ImGui::EndChild();
            
            ImGui::End();
        }

        window.clear(sf::Color(50, 50, 50));
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}