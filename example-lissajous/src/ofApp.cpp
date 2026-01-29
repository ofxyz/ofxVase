#include "ofApp.h"

void ofApp::setup() {
    ofSetWindowTitle("ofxVase");
    ofSetFrameRate(60);
    ofBackground(30, 30, 35);
    
    renderer.setup();
}

void ofApp::update() {
    phase += phaseSpeed * ofGetLastFrameTime();
}

void ofApp::draw() {
    float cx = ofGetWidth() / 2.0f;
    float cy = ofGetHeight() / 2.0f;
    
    // Build Lissajous curve (pre-allocate for performance)
    std::vector<glm::vec2> points;
    std::vector<ofFloatColor> colors;
    std::vector<float> widths;
    points.reserve(numPoints + 1);
    colors.reserve(numPoints + 1);
    widths.reserve(numPoints + 1);
    
    for (int i = 0; i <= numPoints; i++) {
        float t = (float)i / numPoints * glm::two_pi<float>();
        float x = cx + amplitude * sin(freqA * t + phase);
        float y = cy + amplitude * sin(freqB * t);
        points.push_back(glm::vec2(x, y));
        
        // Rainbow color
        float hue = fmod(t / glm::two_pi<float>() + phase * 0.1f, 1.0f);
        ofFloatColor c;
        c.setHsb(hue, 0.8f, 1.0f, 1.0f);
        colors.push_back(c);
        
        // Width
        float w = baseWidth;
        if (animateWidth) {
            w += widthVariation * (0.5f + 0.5f * sin(t * 3 + phase * 2));
        }
        widths.push_back(w);
    }
    
    // Draw
    ofxVase::Options opts;
    opts.joint = ofxVase::JointStyle::Round;
    opts.cap = ofxVase::CapStyle::Round;
    opts.feather = false;  // Disabled until feathering is fixed
    opts.smoothing = smoothing;  // Catmull-Rom subdivision
    
    ofxVase::Polyline poly(points, colors, widths, opts);
    lastVertexCount = poly.holder.getCount();
    
    // Draw using renderer
    ofEnableAlphaBlending();
    ofDisableDepthTest();  // Important: disable depth test for proper alpha blending
    renderer.begin();
    renderer.draw(poly);
    renderer.end();
    ofEnableDepthTest();  // Re-enable if you need it for other 3D content
    
    
    if (showHelp) {
        ofSetColor(255);
        int y = 30;
        int lh = 18;
        
        ofDrawBitmapString("ofxVase - Animated Lissajous Curve", 20, y); y += lh * 2;
        
        ofDrawBitmapString("Freq A: " + ofToString(freqA, 1) + " (Q/A)", 20, y); y += lh;
        ofDrawBitmapString("Freq B: " + ofToString(freqB, 1) + " (W/S)", 20, y); y += lh;
        ofDrawBitmapString("Speed:  " + ofToString(phaseSpeed, 1) + " (E/D)", 20, y); y += lh;
        ofDrawBitmapString("Width:  " + ofToString(baseWidth, 0) + " (R/F)", 20, y); y += lh;
        ofDrawBitmapString("Var:    " + ofToString(widthVariation, 0) + " (T/G)", 20, y); y += lh;
        ofDrawBitmapString("Points: " + ofToString(numPoints) + " (Y/U)", 20, y); y += lh;
        ofDrawBitmapString("Smooth: " + ofToString(smoothing) + " (I/O)", 20, y); y += lh;
        
        ofDrawBitmapString("Space - Pause/Play", 20, y); y += lh;
        ofDrawBitmapString("V - Width animation", 20, y); y += lh;
        ofDrawBitmapString("1-5 - Presets", 20, y); y += lh;
        ofDrawBitmapString("H - Hide help", 20, y); y += lh;
        
        ofDrawBitmapString("FPS: " + ofToString(ofGetFrameRate(), 0), ofGetWidth() - 80, 30);
        ofDrawBitmapString("Verts: " + ofToString(lastVertexCount), ofGetWidth() - 100, 50);
    }
}

void ofApp::keyPressed(int key) {
    switch (key) {
        // Frequency A
        case 'q': case 'Q': freqA += 0.5f; break;
        case 'a': case 'A': freqA = std::max(0.5f, freqA - 0.5f); break;
        
        // Frequency B
        case 'w': case 'W': freqB += 0.5f; break;
        case 's': case 'S': freqB = std::max(0.5f, freqB - 0.5f); break;
        
        // Phase speed
        case 'e': case 'E': phaseSpeed += 0.2f; break;
        case 'd': case 'D': phaseSpeed = std::max(0.0f, phaseSpeed - 0.2f); break;
        
        // Base width
        case 'r': case 'R': baseWidth += 2.0f; break;
        case 'f': case 'F': baseWidth = std::max(1.0f, baseWidth - 2.0f); break;
        
        // Width variation
        case 't': case 'T': widthVariation += 5.0f; break;
        case 'g': case 'G': widthVariation = std::max(0.0f, widthVariation - 5.0f); break;
        
        // Point count
        case 'y': case 'Y': numPoints += 10; break;
        case 'u': case 'U': numPoints = std::max(10, numPoints - 10); break;
        
        // Smoothing (Catmull-Rom subdivisions)
        case 'i': case 'I': smoothing += 1; break;
        case 'o': case 'O': smoothing = std::max(0, smoothing - 1); break;
        
        
        // Toggles
        case ' ': phaseSpeed = (phaseSpeed > 0) ? 0 : 0.5f; break;
        case 'v': case 'V': animateWidth = !animateWidth; break;
        case 'h': case 'H': showHelp = !showHelp; break;
        
        // Presets
        case '1': freqA = 1; freqB = 2; break;  // Figure-8
        case '2': freqA = 3; freqB = 2; break;  // Pretzel
        case '3': freqA = 3; freqB = 4; break;  // Complex
        case '4': freqA = 5; freqB = 4; break;  // Intricate
        case '5': freqA = 7; freqB = 6; break;  // Very complex
    }
}
