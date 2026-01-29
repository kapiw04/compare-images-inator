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
    sf::Image selectionImage;  // Combined selection from both images
    
    sf::Texture texture1;
    sf::Texture texture2;
    sf::Texture diffTexture;
    sf::Texture selectionTexture;
    
    bool image1Loaded = false;
    bool image2Loaded = false;
    bool diffImageGenerated = false;
    bool selectionImageGenerated = false;
    
    // File paths
    char filePath1[512] = "";
    char filePath2[512] = "";
    char savePathDiff[512] = "difference.bmp";
    char savePathSelection[512] = "selection.bmp";
    
    // View settings - arbitrary zoom
    float zoomLevel = 1.0f;  // Continuous zoom level (default 100%)
    float zoomMin = 0.1f;    // 10% minimum
    float zoomMax = 10.0f;   // 1000% maximum
    float zoomStep = 0.1f;   // Zoom step for mouse wheel
    
    // Relative zoom adjustment for different-sized images
    float relativeZoom2 = 1.0f;  // Relative zoom for image 2 vs image 1
    bool autoMatchSizes = true;  // Automatically match image sizes
    
    // Pan offset (synchronized for both images)
    sf::Vector2f panOffset = {0.0f, 0.0f};
    bool isPanning = false;
    sf::Vector2i lastMousePos;
    
    // Area selection
    bool isSelecting = false;
    bool hasSelection = false;
    sf::Vector2f selectionStart = {0.0f, 0.0f};  // In image coordinates
    sf::Vector2f selectionEnd = {0.0f, 0.0f};    // In image coordinates
    int activePane = 0;  // 0 = none, 1 = left, 2 = right
    
    // Status message
    std::string statusMessage = "Load two images to compare";
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

// Calculate relative zoom for image 2 to match image 1's apparent size
void calculateRelativeZoom(AppState& state) {
    if (!state.image1Loaded || !state.image2Loaded) {
        state.relativeZoom2 = 1.0f;
        return;
    }
    
    sf::Vector2u size1 = state.image1.getSize();
    sf::Vector2u size2 = state.image2.getSize();
    
    if (size2.x == 0 || size2.y == 0) {
        state.relativeZoom2 = 1.0f;
        return;
    }
    
    // Calculate the ratio to make image2 appear the same size as image1
    float ratioX = static_cast<float>(size1.x) / static_cast<float>(size2.x);
    float ratioY = static_cast<float>(size1.y) / static_cast<float>(size2.y);
    
    // Use the average ratio to maintain proportions
    state.relativeZoom2 = (ratioX + ratioY) / 2.0f;
}

// Get normalized selection rectangle (min/max coordinates)
void getNormalizedSelection(const AppState& state, sf::Vector2f& minCoord, sf::Vector2f& maxCoord) {
    minCoord.x = std::min(state.selectionStart.x, state.selectionEnd.x);
    minCoord.y = std::min(state.selectionStart.y, state.selectionEnd.y);
    maxCoord.x = std::max(state.selectionStart.x, state.selectionEnd.x);
    maxCoord.y = std::max(state.selectionStart.y, state.selectionEnd.y);
}

// Extract selected area from both images and combine side-by-side
void extractAndCombineSelection(AppState& state) {
    if (!state.image1Loaded || !state.image2Loaded) {
        state.statusMessage = "Load both images first!";
        return;
    }
    
    if (!state.hasSelection) {
        state.statusMessage = "No area selected! Left-click and drag to select an area.";
        return;
    }
    
    sf::Vector2f minCoord, maxCoord;
    getNormalizedSelection(state, minCoord, maxCoord);
    
    sf::Vector2u size1 = state.image1.getSize();
    sf::Vector2u size2 = state.image2.getSize();
    
    // Clamp selection to image 1 bounds
    unsigned int x1 = static_cast<unsigned int>(std::max(0.0f, minCoord.x));
    unsigned int y1 = static_cast<unsigned int>(std::max(0.0f, minCoord.y));
    unsigned int x2 = static_cast<unsigned int>(std::min(static_cast<float>(size1.x), maxCoord.x));
    unsigned int y2 = static_cast<unsigned int>(std::min(static_cast<float>(size1.y), maxCoord.y));
    
    unsigned int selWidth = x2 - x1;
    unsigned int selHeight = y2 - y1;
    
    if (selWidth == 0 || selHeight == 0) {
        state.statusMessage = "Selection is empty!";
        return;
    }
    
    // For image 2, we need to calculate the corresponding region
    // taking into account the relative zoom factor
    float relZoom = state.autoMatchSizes ? state.relativeZoom2 : 1.0f;
    
    unsigned int x1_img2 = static_cast<unsigned int>(std::max(0.0f, minCoord.x / relZoom));
    unsigned int y1_img2 = static_cast<unsigned int>(std::max(0.0f, minCoord.y / relZoom));
    unsigned int x2_img2 = static_cast<unsigned int>(std::min(static_cast<float>(size2.x), maxCoord.x / relZoom));
    unsigned int y2_img2 = static_cast<unsigned int>(std::min(static_cast<float>(size2.y), maxCoord.y / relZoom));
    
    unsigned int selWidth2 = x2_img2 - x1_img2;
    unsigned int selHeight2 = y2_img2 - y1_img2;
    
    if (selWidth2 == 0 || selHeight2 == 0) {
        selWidth2 = selWidth;
        selHeight2 = selHeight;
        x2_img2 = std::min(x1_img2 + selWidth2, size2.x);
        y2_img2 = std::min(y1_img2 + selHeight2, size2.y);
        selWidth2 = x2_img2 - x1_img2;
        selHeight2 = y2_img2 - y1_img2;
    }
    
    // Create combined image (both selections side by side)
    unsigned int combinedWidth = selWidth + selWidth2;
    unsigned int combinedHeight = std::max(selHeight, selHeight2);
    
    state.selectionImage.resize({combinedWidth, combinedHeight});
    
    // Fill with black background
    for (unsigned int y = 0; y < combinedHeight; ++y) {
        for (unsigned int x = 0; x < combinedWidth; ++x) {
            state.selectionImage.setPixel({x, y}, sf::Color::Black);
        }
    }
    
    // Copy selection from image 1 (left side)
    for (unsigned int y = 0; y < selHeight; ++y) {
        for (unsigned int x = 0; x < selWidth; ++x) {
            if (x1 + x < size1.x && y1 + y < size1.y) {
                sf::Color pixel = state.image1.getPixel({x1 + x, y1 + y});
                state.selectionImage.setPixel({x, y}, pixel);
            }
        }
    }
    
    // Copy selection from image 2 (right side)
    for (unsigned int y = 0; y < selHeight2; ++y) {
        for (unsigned int x = 0; x < selWidth2; ++x) {
            if (x1_img2 + x < size2.x && y1_img2 + y < size2.y) {
                sf::Color pixel = state.image2.getPixel({x1_img2 + x, y1_img2 + y});
                state.selectionImage.setPixel({selWidth + x, y}, pixel);
            }
        }
    }
    
    if (!state.selectionTexture.loadFromImage(state.selectionImage)) {
        state.statusMessage = "Failed to create selection texture!";
        return;
    }
    
    state.selectionImageGenerated = true;
    state.statusMessage = "Selection extracted! See popup window.";
}

// Save selection image to file (supports multiple formats based on extension)
bool saveSelectionImage(AppState& state) {
    if (!state.selectionImageGenerated) {
        state.statusMessage = "Extract selection first!";
        return false;
    }
    
    std::string path = state.savePathSelection;
    if (path.empty()) {
        path = "selection.bmp";
    }
    
    if (!state.selectionImage.saveToFile(path)) {
        state.statusMessage = "Failed to save selection image!";
        return false;
    }
    
    state.statusMessage = "Saved selection image to: " + path;
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
            
            // Handle mouse wheel for zooming - arbitrary zoom levels
            if (auto* scrollEvent = event->getIf<sf::Event::MouseWheelScrolled>()) {
                if (!ImGui::GetIO().WantCaptureMouse) {
                    float oldZoom = state.zoomLevel;
                    if (scrollEvent->delta > 0) {
                        state.zoomLevel = std::min(state.zoomMax, state.zoomLevel + state.zoomStep);
                    } else if (scrollEvent->delta < 0) {
                        state.zoomLevel = std::max(state.zoomMin, state.zoomLevel - state.zoomStep);
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
        if (ImGui::Button("Load Image")) {
            if (loadImage(state.filePath1, state.image1, state.texture1, state.statusMessage)) {
                state.image1Loaded = true;
                state.diffImageGenerated = false;
                state.selectionImageGenerated = false;
                calculateRelativeZoom(state);
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
        if (ImGui::Button("Load Image")) {
            if (loadImage(state.filePath2, state.image2, state.texture2, state.statusMessage)) {
                state.image2Loaded = true;
                state.diffImageGenerated = false;
                state.selectionImageGenerated = false;
                calculateRelativeZoom(state);
            }
        }
        ImGui::PopID();
        if (state.image2Loaded) {
            sf::Vector2u size = state.image2.getSize();
            ImGui::SameLine();
            ImGui::Text("(%ux%u)", size.x, size.y);
        }
        
        ImGui::Separator();
        
        // Zoom controls - continuous arbitrary zoom
        ImGui::Text("Zoom Level:");
        ImGui::SliderFloat("##zoom", &state.zoomLevel, state.zoomMin, state.zoomMax, "%.1fx");
        ImGui::SameLine();
        ImGui::Text("(%.0f%%)", state.zoomLevel * 100.0f);
        
        // Quick zoom buttons
        if (ImGui::Button("25%")) state.zoomLevel = 0.25f;
        ImGui::SameLine();
        if (ImGui::Button("50%")) state.zoomLevel = 0.5f;
        ImGui::SameLine();
        if (ImGui::Button("100%")) state.zoomLevel = 1.0f;
        ImGui::SameLine();
        if (ImGui::Button("200%")) state.zoomLevel = 2.0f;
        ImGui::SameLine();
        if (ImGui::Button("400%")) state.zoomLevel = 4.0f;
        
        ImGui::Text("(Use mouse wheel to zoom, right-click drag to pan)");
        
        ImGui::Separator();
        
        // Different size image handling
        ImGui::Text("Size Matching:");
        ImGui::Checkbox("Auto-match different image sizes", &state.autoMatchSizes);
        if (state.autoMatchSizes && state.image1Loaded && state.image2Loaded) {
            ImGui::Text("Relative zoom for Image 2: %.2fx", state.relativeZoom2);
        }
        
        ImGui::Separator();
        
        // Area selection controls
        ImGui::Text("Area Selection:");
        ImGui::Text("(Left-click and drag on images to select)");
        if (state.hasSelection) {
            sf::Vector2f minCoord, maxCoord;
            getNormalizedSelection(state, minCoord, maxCoord);
            ImGui::Text("Selection: (%.0f,%.0f) to (%.0f,%.0f)", 
                        minCoord.x, minCoord.y, maxCoord.x, maxCoord.y);
            
            if (ImGui::Button("Extract Selection")) {
                extractAndCombineSelection(state);
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear Selection")) {
                state.hasSelection = false;
                state.selectionImageGenerated = false;
            }
        } else {
            ImGui::TextDisabled("No selection");
        }
        
        if (state.selectionImageGenerated) {
            ImGui::InputText("Selection Save Path", state.savePathSelection, sizeof(state.savePathSelection));
            if (ImGui::Button("Save Selection")) {
                saveSelectionImage(state);
            }
        }
        
        ImGui::Separator();
        
        // Difference image controls
        ImGui::Text("Difference Image:");
        if (ImGui::Button("Generate Difference")) {
            generateDifferenceImage(state);
        }
        
        ImGui::InputText("Diff Save Path", state.savePathDiff, sizeof(state.savePathDiff));
        if (ImGui::Button("Save Difference")) {
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
        float panelWidth = 400.0f;  // Increased for new controls
        float availableWidth = windowSize.x - panelWidth - 30.0f;
        float viewWidth = availableWidth / 2.0f - 10.0f;
        float viewHeight = windowSize.y - 20.0f;
        float currentZoom = state.zoomLevel;
        float zoom2 = state.autoMatchSizes ? currentZoom * state.relativeZoom2 : currentZoom;
        
        // Store pane positions for selection handling
        ImVec2 leftPanePos, rightPanePos;
        ImVec2 leftPaneSize, rightPaneSize;
        
        // Image comparison view
        ImGui::SetNextWindowPos(ImVec2(panelWidth + 10, 10));
        ImGui::SetNextWindowSize(ImVec2(availableWidth + 10, viewHeight));
        ImGui::Begin("Image Comparison", nullptr, 
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
        
        // Split view with vertical divider
        ImGui::BeginChild("LeftPane", ImVec2(viewWidth, viewHeight - 40), true,
                          ImGuiWindowFlags_HorizontalScrollbar);
        leftPanePos = ImGui::GetWindowPos();
        leftPaneSize = ImGui::GetWindowSize();
        
        if (state.image1Loaded) {
            sf::Vector2u texSize = state.texture1.getSize();
            float scaledWidth = texSize.x * currentZoom;
            float scaledHeight = texSize.y * currentZoom;
            
            ImVec2 imagePos = ImVec2(state.panOffset.x + 5, state.panOffset.y + 5);
            ImGui::SetCursorPos(imagePos);
            ImTextureID texId = static_cast<ImTextureID>(static_cast<uintptr_t>(state.texture1.getNativeHandle()));
            ImGui::Image(texId, ImVec2(scaledWidth, scaledHeight));
            
            // Handle selection on left pane
            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                ImVec2 mousePos = ImGui::GetMousePos();
                ImVec2 windowPos = ImGui::GetWindowPos();
                float imgX = (mousePos.x - windowPos.x - imagePos.x) / currentZoom;
                float imgY = (mousePos.y - windowPos.y - imagePos.y) / currentZoom;
                state.selectionStart = {imgX, imgY};
                state.selectionEnd = {imgX, imgY};
                state.isSelecting = true;
                state.activePane = 1;
            }
            
            if (state.isSelecting && state.activePane == 1 && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                ImVec2 mousePos = ImGui::GetMousePos();
                ImVec2 windowPos = ImGui::GetWindowPos();
                float imgX = (mousePos.x - windowPos.x - imagePos.x) / currentZoom;
                float imgY = (mousePos.y - windowPos.y - imagePos.y) / currentZoom;
                state.selectionEnd = {imgX, imgY};
            }
            
            if (state.isSelecting && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                state.isSelecting = false;
                state.hasSelection = true;
                state.activePane = 0;
            }
            
            // Draw selection rectangle overlay
            if (state.hasSelection || state.isSelecting) {
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                sf::Vector2f minCoord, maxCoord;
                getNormalizedSelection(state, minCoord, maxCoord);
                
                ImVec2 windowPos = ImGui::GetWindowPos();
                float rectX1 = windowPos.x + imagePos.x + minCoord.x * currentZoom;
                float rectY1 = windowPos.y + imagePos.y + minCoord.y * currentZoom;
                float rectX2 = windowPos.x + imagePos.x + maxCoord.x * currentZoom;
                float rectY2 = windowPos.y + imagePos.y + maxCoord.y * currentZoom;
                
                drawList->AddRect(ImVec2(rectX1, rectY1), ImVec2(rectX2, rectY2), 
                                  IM_COL32(255, 0, 0, 255), 0.0f, 0, 2.0f);
                drawList->AddRectFilled(ImVec2(rectX1, rectY1), ImVec2(rectX2, rectY2), 
                                        IM_COL32(255, 0, 0, 50));
            }
        } else {
            ImGui::Text("Image 1");
            ImGui::Text("No image loaded");
        }
        ImGui::EndChild();
        
        ImGui::SameLine();
        
        // Vertical divider line (represented by same-line layout)
        ImGui::BeginChild("RightPane", ImVec2(viewWidth, viewHeight - 40), true,
                          ImGuiWindowFlags_HorizontalScrollbar);
        rightPanePos = ImGui::GetWindowPos();
        rightPaneSize = ImGui::GetWindowSize();
        
        if (state.image2Loaded) {
            sf::Vector2u texSize = state.texture2.getSize();
            float scaledWidth = texSize.x * zoom2;
            float scaledHeight = texSize.y * zoom2;
            
            ImVec2 imagePos = ImVec2(state.panOffset.x + 5, state.panOffset.y + 5);
            ImGui::SetCursorPos(imagePos);
            ImTextureID texId = static_cast<ImTextureID>(static_cast<uintptr_t>(state.texture2.getNativeHandle()));
            ImGui::Image(texId, ImVec2(scaledWidth, scaledHeight));
            
            // Draw corresponding selection rectangle on image 2
            if (state.hasSelection || state.isSelecting) {
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                sf::Vector2f minCoord, maxCoord;
                getNormalizedSelection(state, minCoord, maxCoord);
                
                // Adjust coordinates for relative zoom
                float relZoom = state.autoMatchSizes ? state.relativeZoom2 : 1.0f;
                
                ImVec2 windowPos = ImGui::GetWindowPos();
                float rectX1 = windowPos.x + imagePos.x + (minCoord.x / relZoom) * zoom2;
                float rectY1 = windowPos.y + imagePos.y + (minCoord.y / relZoom) * zoom2;
                float rectX2 = windowPos.x + imagePos.x + (maxCoord.x / relZoom) * zoom2;
                float rectY2 = windowPos.y + imagePos.y + (maxCoord.y / relZoom) * zoom2;
                
                drawList->AddRect(ImVec2(rectX1, rectY1), ImVec2(rectX2, rectY2), 
                                  IM_COL32(0, 255, 0, 255), 0.0f, 0, 2.0f);
                drawList->AddRectFilled(ImVec2(rectX1, rectY1), ImVec2(rectX2, rectY2), 
                                        IM_COL32(0, 255, 0, 50));
            }
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
        
        // Selection image window (shown when extracted)
        if (state.selectionImageGenerated) {
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(900, 500), ImGuiCond_Appearing);
            ImGui::SetNextWindowFocus();
            ImGui::Begin("Selection Comparison", &state.selectionImageGenerated);
            
            sf::Vector2u texSize = state.selectionTexture.getSize();
            float scaledWidth = texSize.x * currentZoom;
            float scaledHeight = texSize.y * currentZoom;
            
            ImGui::Text("Left: Image 1 selection | Right: Image 2 selection");
            ImGui::Separator();
            
            ImGui::BeginChild("SelectionView", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::SetCursorPos(ImVec2(5, 5));
            ImTextureID texId = static_cast<ImTextureID>(static_cast<uintptr_t>(state.selectionTexture.getNativeHandle()));
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