#pragma once

#include "ofMain.h"
#include "ofxVase.h"

class ofApp : public ofBaseApp {
public:
    void setup() override;
    void update() override;
    void draw() override;
    void keyPressed(int key) override;
    
private:
    // Lissajous parameters
    float freqA = 3.0f;
    float freqB = 4.0f;
    float phase = 0.0f;
    float phaseSpeed = 0.5f;
    
    // Curve settings
    int numPoints = 100;  // Base control points
    int smoothing = 4;    // Catmull-Rom subdivisions (0 = off)
    float amplitude = 250.0f;
    float baseWidth = 3.0f;
    float widthVariation = 15.0f;
    bool animateWidth = true;
    
    // Renderer
    ofxVase::Renderer renderer;
    int lastVertexCount = 0;
    
    // UI
    bool showHelp = true;
};
