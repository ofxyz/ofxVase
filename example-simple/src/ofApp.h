#pragma once

#include "ofMain.h"
#include "ofxVase.h"

// Per-vertex data
struct VertexData {
    glm::vec2 position;
    float width = 10.0f;
    ofColor color = ofColor(100, 200, 100, 200);
};

class ofApp : public ofBaseApp {
public:
    void setup() override;
    void update() override;
    void draw() override;
    
    void keyPressed(int key) override;
    void mouseDragged(int x, int y, int button) override;
    void mousePressed(int x, int y, int button) override;
    void mouseReleased(int x, int y, int button) override;
    void mouseScrolled(int x, int y, float scrollX, float scrollY) override;
    
private:
    void rebuildMesh();
    void drawUI();
    void drawVertexEditor();
    
    // Vertex data with per-vertex width and color
    std::vector<VertexData> vertices;
    int dragIndex = -1;
    int selectedIndex = -1;  // For editing
    float dragRadius = 20.0f;
    
    // Global style options
    ofxVase::JointStyle jointStyle = ofxVase::JointStyle::Round;
    ofxVase::CapStyle capStyle = ofxVase::CapStyle::Round;
    bool showWireframe = false;
    bool showPoints = true;
    bool feather = true;
    float feathering = 1.0f;
    
    // Current polyline (used for rendering)
    ofxVase::Polyline currentPolyline;
    
    // Renderer
    ofxVase::Renderer renderer;
    
    // UI state
    bool showHelp = true;
};
