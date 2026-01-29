#include "ofApp.h"

void ofApp::setup() {
    ofSetWindowTitle("ofxVase Example - Per-vertex Width & Color");
    ofSetFrameRate(60);
    ofBackground(40, 40, 45);
    
    // Setup renderer
    renderer.setup();
    
    // Create stylized B/lightning shape with varying widths
    float cx = ofGetWidth() / 2;
    float cy = ofGetHeight() / 2;
    
    vertices = {
        { {cx - 120, cy - 220}, 18.0f, ofColor(255, 80, 120, 255) },   // top left
        { {cx + 140, cy - 160}, 22.0f, ofColor(255, 150, 80, 255) },   // top right
        { {cx - 80,  cy - 40},  20.0f, ofColor(255, 220, 100, 255) },  // middle left
        { {cx + 160, cy + 20},  24.0f, ofColor(120, 255, 120, 255) },  // middle right
        { {cx - 100, cy + 100}, 18.0f, ofColor(80, 220, 255, 255) },   // lower left
        { {cx + 120, cy + 220}, 22.0f, ofColor(120, 255, 180, 255) },  // bottom right
    };
    
    // Default settings
    showPoints = false;
    showWireframe = false;
    feather = true;
    showHelp = false;
    
    rebuildMesh();
}

void ofApp::update() {
}

void ofApp::rebuildMesh() {
    if (vertices.size() < 2) {
        currentPolyline = ofxVase::Polyline();
        return;
    }
    
    // Build options
    ofxVase::Options opts;
    opts.joint = jointStyle;
    opts.cap = capStyle;
    opts.feather = feather;
    opts.feathering = feathering;
    opts.worldToScreenRatio = 1.0f;
    
    // Extract per-vertex data
    std::vector<glm::vec2> points;
    std::vector<ofFloatColor> colors;
    std::vector<float> widths;
    
    for (const auto& v : vertices) {
        points.push_back(v.position);
        colors.push_back(ofFloatColor(v.color));
        widths.push_back(v.width);
    }
    
    // Create polyline with per-vertex colors and widths
    currentPolyline = ofxVase::Polyline(points, colors, widths, opts);
}

void ofApp::draw() {
    // Draw filled polyline using the renderer (handles shader + custom attributes)
    renderer.begin();
    
    renderer.draw(currentPolyline);
    
    if (showWireframe) {
        renderer.end();
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        ofSetColor(255, 255, 255, 100);
        currentPolyline.getMesh().draw();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        renderer.begin();
    }
    
    renderer.end();
    
    // Draw control points
    if (showPoints) {
        for (size_t i = 0; i < vertices.size(); i++) {
            const auto& v = vertices[i];
            
            // Draw width indicator circle
            ofNoFill();
            ofSetColor(v.color, 100);
            ofDrawCircle(v.position, v.width / 2);
            
            // Draw vertex point
            ofFill();
            if (i == selectedIndex) {
                ofSetColor(255, 255, 0);  // Yellow for selected
                ofDrawCircle(v.position, 8);
            } else {
                ofSetColor(v.color);
                ofDrawCircle(v.position, 6);
            }
            
            // Draw vertex number
            ofSetColor(255);
            ofDrawBitmapString(ofToString(i), v.position.x + 10, v.position.y - 10);
        }
    }
    
    // Draw UI
    if (showHelp) {
        drawUI();
    }
    
    // Draw vertex editor if a vertex is selected
    if (selectedIndex >= 0) {
        drawVertexEditor();
    }
}

void ofApp::drawUI() {
    ofSetColor(255);
    
    int y = 30;
    int lineHeight = 18;
    
    ofDrawBitmapString("ofxVase - Per-vertex Width & Color Demo", 20, y); y += lineHeight * 2;
    
    ofDrawBitmapString("Mouse:", 20, y); y += lineHeight;
    ofDrawBitmapString("  Click+Drag - Move vertex", 20, y); y += lineHeight;
    ofDrawBitmapString("  Click - Select vertex for editing", 20, y); y += lineHeight;
    ofDrawBitmapString("  Scroll on vertex - Adjust width", 20, y); y += lineHeight;
    ofDrawBitmapString("  Right-click - Deselect", 20, y); y += lineHeight * 2;
    
    ofDrawBitmapString("Keys:", 20, y); y += lineHeight;
    ofDrawBitmapString("  1/2/3 - Joint: Miter/Bevel/Round", 20, y); y += lineHeight;
    ofDrawBitmapString("  4/5/6 - Cap: Butt/Round/Square", 20, y); y += lineHeight;
    ofDrawBitmapString("  A - Add vertex at mouse", 20, y); y += lineHeight;
    ofDrawBitmapString("  D - Delete selected vertex", 20, y); y += lineHeight;
    ofDrawBitmapString("  W - Toggle wireframe", 20, y); y += lineHeight;
    ofDrawBitmapString("  P - Toggle points", 20, y); y += lineHeight;
    ofDrawBitmapString("  F - Toggle feathering", 20, y); y += lineHeight;
    ofDrawBitmapString("  H - Toggle help", 20, y); y += lineHeight;
    ofDrawBitmapString("  R - Reset", 20, y); y += lineHeight * 2;
    
    // Current settings
    string jointStr = (jointStyle == ofxVase::JointStyle::Miter) ? "Miter" :
                      (jointStyle == ofxVase::JointStyle::Bevel) ? "Bevel" : "Round";
    string capStr = (capStyle == ofxVase::CapStyle::Butt) ? "Butt" :
                    (capStyle == ofxVase::CapStyle::Round) ? "Round" : "Square";
    
    ofDrawBitmapString("Settings:", 20, y); y += lineHeight;
    ofDrawBitmapString("  Joint: " + jointStr, 20, y); y += lineHeight;
    ofDrawBitmapString("  Cap: " + capStr, 20, y); y += lineHeight;
    ofDrawBitmapString("  Feather: " + string(feather ? "ON" : "OFF"), 20, y); y += lineHeight;
    ofDrawBitmapString("  Vertices: " + ofToString(vertices.size()), 20, y); y += lineHeight;
    
    // FPS
    ofDrawBitmapString("FPS: " + ofToString(ofGetFrameRate(), 1), ofGetWidth() - 100, 30);
}

void ofApp::drawVertexEditor() {
    if (selectedIndex < 0 || selectedIndex >= vertices.size()) return;
    
    auto& v = vertices[selectedIndex];
    
    // Draw editor panel
    float panelX = ofGetWidth() - 220;
    float panelY = 20;
    float panelW = 200;
    float panelH = 200;
    
    // Background
    ofSetColor(60, 60, 65, 230);
    ofDrawRectangle(panelX, panelY, panelW, panelH);
    ofNoFill();
    ofSetColor(100);
    ofDrawRectangle(panelX, panelY, panelW, panelH);
    ofFill();
    
    // Title
    ofSetColor(255);
    ofDrawBitmapString("Vertex " + ofToString(selectedIndex), panelX + 10, panelY + 20);
    
    // Width display
    ofDrawBitmapString("Width: " + ofToString(v.width, 1), panelX + 10, panelY + 50);
    ofDrawBitmapString("(scroll to adjust)", panelX + 10, panelY + 65);
    
    // Width bar
    ofSetColor(80);
    ofDrawRectangle(panelX + 10, panelY + 75, 180, 10);
    ofSetColor(v.color);
    ofDrawRectangle(panelX + 10, panelY + 75, v.width * 3.6f, 10);
    
    // Color preview
    ofSetColor(255);
    ofDrawBitmapString("Color:", panelX + 10, panelY + 105);
    ofSetColor(v.color);
    ofDrawRectangle(panelX + 60, panelY + 93, 50, 20);
    
    // RGB values
    ofSetColor(255);
    ofDrawBitmapString("R: " + ofToString((int)v.color.r), panelX + 10, panelY + 135);
    ofDrawBitmapString("G: " + ofToString((int)v.color.g), panelX + 70, panelY + 135);
    ofDrawBitmapString("B: " + ofToString((int)v.color.b), panelX + 130, panelY + 135);
    
    // Position
    ofDrawBitmapString("Pos: " + ofToString((int)v.position.x) + ", " + ofToString((int)v.position.y), 
                       panelX + 10, panelY + 165);
    
    // Color cycling hint
    ofSetColor(180);
    ofDrawBitmapString("C - Cycle color", panelX + 10, panelY + 190);
}

void ofApp::keyPressed(int key) {
    bool rebuild = false;
    
    switch (key) {
        case '1':
            jointStyle = ofxVase::JointStyle::Miter;
            rebuild = true;
            break;
        case '2':
            jointStyle = ofxVase::JointStyle::Bevel;
            rebuild = true;
            break;
        case '3':
            jointStyle = ofxVase::JointStyle::Round;
            rebuild = true;
            break;
        case '4':
            capStyle = ofxVase::CapStyle::Butt;
            rebuild = true;
            break;
        case '5':
            capStyle = ofxVase::CapStyle::Round;
            rebuild = true;
            break;
        case '6':
            capStyle = ofxVase::CapStyle::Square;
            rebuild = true;
            break;
        case 'w':
        case 'W':
            showWireframe = !showWireframe;
            break;
        case 'p':
        case 'P':
            showPoints = !showPoints;
            break;
        case 'f':
        case 'F':
            feather = !feather;
            rebuild = true;
            break;
        case 'h':
        case 'H':
            showHelp = !showHelp;
            break;
        case 'r':
        case 'R':
            setup();
            break;
        case 'a':
        case 'A': {
            // Add vertex at mouse position
            VertexData newVert;
            newVert.position = glm::vec2(ofGetMouseX(), ofGetMouseY());
            newVert.width = 10.0f;
            newVert.color = ofColor::fromHsb(ofRandom(255), 200, 220, 220);
            vertices.push_back(newVert);
            selectedIndex = vertices.size() - 1;
            rebuild = true;
            break;
        }
        case 'd':
        case 'D':
        case OF_KEY_DEL:
        case OF_KEY_BACKSPACE:
            // Delete selected vertex
            if (selectedIndex >= 0 && selectedIndex < vertices.size() && vertices.size() > 2) {
                vertices.erase(vertices.begin() + selectedIndex);
                selectedIndex = -1;
                rebuild = true;
            }
            break;
        case 'c':
        case 'C':
            // Cycle color of selected vertex
            if (selectedIndex >= 0 && selectedIndex < vertices.size()) {
                float h, s, b;
                vertices[selectedIndex].color.getHsb(h, s, b);
                h = fmod(h + 30, 255);
                vertices[selectedIndex].color.setHsb(h, s, b);
                rebuild = true;
            }
            break;
        case '+':
        case '=':
            // Increase selected vertex width
            if (selectedIndex >= 0 && selectedIndex < vertices.size()) {
                vertices[selectedIndex].width = std::min(vertices[selectedIndex].width + 5.0f, 200.0f);
                rebuild = true;
            }
            break;
        case '-':
        case '_':
            // Decrease selected vertex width
            if (selectedIndex >= 0 && selectedIndex < vertices.size()) {
                vertices[selectedIndex].width = std::max(vertices[selectedIndex].width - 5.0f, 0.5f);
                rebuild = true;
            }
            break;
    }
    
    if (rebuild) {
        rebuildMesh();
    }
}

void ofApp::mouseDragged(int x, int y, int button) {
    if (dragIndex >= 0 && dragIndex < static_cast<int>(vertices.size())) {
        vertices[dragIndex].position = glm::vec2(x, y);
        rebuildMesh();
    }
}

void ofApp::mousePressed(int x, int y, int button) {
    glm::vec2 mouse(x, y);
    
    if (button == OF_MOUSE_BUTTON_RIGHT) {
        // Right click to deselect
        selectedIndex = -1;
        dragIndex = -1;
        return;
    }
    
    // Find closest vertex
    dragIndex = -1;
    for (size_t i = 0; i < vertices.size(); i++) {
        if (glm::distance(mouse, vertices[i].position) < dragRadius) {
            dragIndex = static_cast<int>(i);
            selectedIndex = static_cast<int>(i);
            break;
        }
    }
    
    // If clicked in empty space, deselect
    if (dragIndex < 0) {
        selectedIndex = -1;
    }
}

void ofApp::mouseReleased(int x, int y, int button) {
    dragIndex = -1;
}

void ofApp::mouseScrolled(int x, int y, float scrollX, float scrollY) {
    glm::vec2 mouse(x, y);
    
    // Find vertex under mouse
    for (size_t i = 0; i < vertices.size(); i++) {
        if (glm::distance(mouse, vertices[i].position) < dragRadius) {
            // Adjust width with scroll
            vertices[i].width += scrollY * 5.0f;
            vertices[i].width = glm::clamp(vertices[i].width, 0.5f, 200.0f);
            selectedIndex = static_cast<int>(i);
            rebuildMesh();
            break;
        }
    }
}
