#pragma once
/*
 * ofxVase - Modern polyline tessellation for OpenFrameworks
 * 
 * High-quality variable-width polyline rendering using triangle mesh tessellation.
 * Based on the VASE renderer algorithm by TSANG, Hao Fung (tyt2y7).
 * 
 * Key features:
 * - Per-vertex color and width support
 * - Smooth rounded joints (prevents gaps at turns)
 * - Round, Square, and Butt end caps
 * - Joint styles: Miter, Bevel, Round
 * - Catmull-Rom spline smoothing
 * 
 * License: BSD 3-Clause (see LICENSE.txt)
 */

#include "ofMain.h"

namespace ofxVase {

// ============================================================================
// Configuration Types
// ============================================================================

enum class JointStyle : char {
    Miter = 0,
    Bevel = 1,
    Round = 2
};

enum class CapStyle : char {
    Butt   = 0,
    Round  = 1,
    Square = 2,
    Rect   = 3
};

enum class CapPosition : char {
    Both  = 0,
    First = 10,
    Last  = 20,
    None  = 30
};

struct Options {
    JointStyle joint = JointStyle::Round;
    CapStyle cap = CapStyle::Round;
    CapPosition capPosition = CapPosition::Both;
    bool feather = true;
    float feathering = 1.0f;
    bool noFeatherAtCap = false;
    bool noFeatherAtCore = false;
    float worldToScreenRatio = 1.0f;  // For resolution-independent rendering
    int smoothing = 0;  // 0 = no smoothing, >0 = subdivisions per segment (Catmull-Rom)
    
    Options() = default;
    
    Options& setJoint(JointStyle j) { joint = j; return *this; }
    Options& setCap(CapStyle c) { cap = c; return *this; }
    Options& setCapPosition(CapPosition p) { capPosition = p; return *this; }
    Options& setFeather(bool f, float amount = 1.0f) { 
        feather = f; 
        feathering = amount; 
        return *this; 
    }
    Options& setScale(float s) { worldToScreenRatio = s; return *this; }
    Options& setSmoothing(int subdivisions) { smoothing = subdivisions; return *this; }
};

// ============================================================================
// Vertex Array Holder - Accumulates mesh data
// ============================================================================

class VertexArrayHolder {
public:
    // Draw modes (renamed to avoid OpenGL macro conflicts)
    static constexpr int DRAW_TRIANGLES = 0;
    static constexpr int DRAW_TRIANGLE_STRIP = 1;
    
    int glmode = DRAW_TRIANGLES;
    
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;      // UV.x, UV.y for position in stroke
    std::vector<glm::vec2> uvs2;     // UV.z, UV.w for fade control (rr values)
    std::vector<ofFloatColor> colors;
    
    void clear();
    int getCount() const { return static_cast<int>(vertices.size()); }
    void setGlDrawMode(int mode) { glmode = mode; }
    
    // Push a vertex with fade ratio (rr = 0 means full alpha, rr > 0 means fade)
    void push(const glm::vec2& pos, const ofFloatColor& color, float rr = 0.0f);
    void push(const glm::vec2& pos, const ofFloatColor& color, float rrX, float rrY);
    void push3(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3,
               const ofFloatColor& c1, const ofFloatColor& c2, const ofFloatColor& c3,
               float rr1 = 0, float rr2 = 0, float rr3 = 0);
    void pushFade(const glm::vec2& pos, const ofFloatColor& color);  // Fully faded
    
    // Merge another holder
    void push(const VertexArrayHolder& other);
    
    // Get vertex at index
    glm::vec2 get(int i) const;
    
    // Convert to ofMesh
    ofMesh toMesh() const;
    
    // Jump for triangle strip (degenerate triangles)
    void jump();
    
private:
    bool jumping = false;
    void repeatLastPush();
};

// ============================================================================
// Polyline - Main tessellation class
// ============================================================================

class Polyline {
public:
    VertexArrayHolder holder;
    
    Polyline() = default;
    
    // Uniform color and width
    Polyline(const std::vector<glm::vec2>& points, 
             const ofFloatColor& color, 
             float width,
             const Options& opt = Options());
    
    // Per-vertex colors and widths
    Polyline(const std::vector<glm::vec2>& points,
             const std::vector<ofFloatColor>& colors,
             const std::vector<float>& widths,
             const Options& opt = Options());
    
    // From ofPolyline
    Polyline(const ofPolyline& poly,
             const ofFloatColor& color,
             float width,
             const Options& opt = Options());
    
    // Get the resulting mesh
    ofMesh getMesh() const { return holder.toMesh(); }
    
    // Append another polyline
    void append(const Polyline& other) { holder.push(other.holder); }
    
private:
    struct InternalOpt {
        bool constColor = false;
        bool constWeight = false;
        bool noCapFirst = false;
        bool noCapLast = false;
        bool joinFirst = false;
        bool joinLast = false;
        std::vector<float> segmentLength;
        VertexArrayHolder holder;
    };
    
    struct StPolyline {
        glm::vec2 vP;      // Vector to intersection point
        glm::vec2 T;       // Core thickness
        glm::vec2 R;       // Fading edge
        glm::vec2 bR;      // Out stepping vector
        glm::vec2 T1;      // Alternate vector
        float t = 0, r = 0;
        bool degenT = false;
        bool degenR = false;
        bool preFull = false;
        glm::vec2 PT;
        char djoint = 0;
    };
    
    struct StAnchor {
        glm::vec2 P[3];
        ofFloatColor C[3];
        float W[3];
        StPolyline SL[3];
        VertexArrayHolder vah;
    };
    
    // Core tessellation methods
    static void determineTr(float w, float& t, float& R, float scale);
    static float getPljRoundDangle(float t, float r, float scale);
    
    static void makeTrc(const glm::vec2& P1, const glm::vec2& P2,
                        glm::vec2& T, glm::vec2& R, glm::vec2& C,
                        float w, const Options& opt,
                        float& rr, float& tt, float& dist, bool anchorMode);
    
    void polylineRange(const std::vector<glm::vec2>& P,
                       const std::vector<ofFloatColor>& C,
                       const std::vector<float>& W,
                       const Options& opt, InternalOpt& inopt,
                       int from, int to, bool approx);
    
    void polylineApprox(const std::vector<glm::vec2>& P,
                        const std::vector<ofFloatColor>& C,
                        const std::vector<float>& W,
                        const Options& opt, InternalOpt& inopt,
                        int from, int to);
    
    void polylineExact(const std::vector<glm::vec2>& P,
                       const std::vector<ofFloatColor>& C,
                       const std::vector<float>& W,
                       const Options& opt, InternalOpt& inopt,
                       int from, int to);
    
    static void segment(StAnchor& SA, const Options& opt,
                        bool capFirst, bool capLast, bool core);
    
    static void segmentLate(const Options& opt,
                            glm::vec2* P, ofFloatColor* C, StPolyline* SL,
                            VertexArrayHolder& tris,
                            const glm::vec2& cap1, const glm::vec2& cap2, bool core);
    
    static void anchor(StAnchor& SA, const Options& opt, bool capFirst, bool capLast);
    static void anchorLate(const Options& opt,
                           glm::vec2* P, ofFloatColor* C, StPolyline* SL,
                           VertexArrayHolder& tris, bool capFirst, bool capLast);
    
    static void vectorsToArc(VertexArrayHolder& hold,
                             const glm::vec2& P, const ofFloatColor& C, const ofFloatColor& C2,
                             const glm::vec2& PA, const glm::vec2& PB,
                             float dangle, float r, float r2,
                             bool ignorEnds, const glm::vec2& apparentP,
                             const glm::vec2& hint, float rr, bool innerFade);
};

// ============================================================================
// Segment - Single line segment with varying width/color
// ============================================================================

class Segment {
public:
    VertexArrayHolder holder;
    
    Segment(const glm::vec2& p1, const glm::vec2& p2,
            const ofFloatColor& c1, const ofFloatColor& c2,
            float w1, float w2,
            const Options& opt = Options());
    
    Segment(const glm::vec2& p1, const glm::vec2& p2,
            const ofFloatColor& color, float width,
            const Options& opt = Options());
    
    ofMesh getMesh() const { return holder.toMesh(); }
};

// ============================================================================
// Renderer - Batch rendering helper
// ============================================================================

class Renderer {
public:
    Renderer();
    ~Renderer();
    
    // Initialize renderer
    void setup();
    
    // Begin/end rendering block (enables alpha blending)
    void begin();
    void end();
    
    // Draw polylines and segments
    void draw(const VertexArrayHolder& holder);
    void draw(const Polyline& polyline);
    void draw(const Segment& segment);
    
    // For compatibility - always returns true since mesh rendering works
    bool isShaderLoaded() const { return true; }
    bool getUseVbo() const { return false; }
    void setUseVbo(bool) { /* no-op, always uses mesh */ }
    
private:
    bool initialized = false;
};

// ============================================================================
// Utility Functions
// ============================================================================

namespace util {
    // Vector operations matching C# Vec2Ext
    float normalize(glm::vec2& v);
    void perpen(glm::vec2& v);  // Perpendicular (rotate 90 CCW)
    void opposite(glm::vec2& v);
    float signedArea(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3);
    void dot(const glm::vec2& a, const glm::vec2& b, glm::vec2& out);
    void anchorOutward(glm::vec2& v, const glm::vec2& b, const glm::vec2& c);
    void followSigns(glm::vec2& v, const glm::vec2& a);
    int intersect(const glm::vec2& p1, const glm::vec2& p2,
                  const glm::vec2& p3, const glm::vec2& p4,
                  glm::vec2& out, float* pts = nullptr);
    
    // Color interpolation
    ofFloatColor colorBetween(const ofFloatColor& a, const ofFloatColor& b, float t);
    
    // Catmull-Rom spline interpolation
    glm::vec2 catmullRom(const glm::vec2& p0, const glm::vec2& p1, 
                         const glm::vec2& p2, const glm::vec2& p3, float t);
    
    // Smooth a polyline using Catmull-Rom splines
    void smoothPolyline(const std::vector<glm::vec2>& points,
                        const std::vector<ofFloatColor>& colors,
                        const std::vector<float>& widths,
                        int subdivisions,
                        std::vector<glm::vec2>& outPoints,
                        std::vector<ofFloatColor>& outColors,
                        std::vector<float>& outWidths);
}

// ============================================================================
// Simple OF-Style API
// ============================================================================

// Draw an ofPolyline with current OF color and specified width
void draw(const ofPolyline& polyline, float width = 2.0f);

// Draw an ofPolyline with specified color and width
void draw(const ofPolyline& polyline, const ofColor& color, float width = 2.0f);

// Draw with per-vertex widths
void draw(const ofPolyline& polyline, const ofColor& color, 
          const std::vector<float>& widths);

// Draw with per-vertex colors and widths
void draw(const ofPolyline& polyline, 
          const std::vector<ofColor>& colors,
          const std::vector<float>& widths);

// Draw a simple line segment
void drawLine(float x1, float y1, float x2, float y2, float width = 2.0f);
void drawLine(const glm::vec2& p1, const glm::vec2& p2, float width = 2.0f);
void drawLine(const glm::vec2& p1, const glm::vec2& p2, 
              const ofColor& color, float width = 2.0f);

// Draw line with varying width (tapered)
void drawLine(const glm::vec2& p1, const glm::vec2& p2,
              float width1, float width2);
void drawLine(const glm::vec2& p1, const glm::vec2& p2,
              const ofColor& c1, const ofColor& c2,
              float width1, float width2);

// Global settings
void setJointStyle(JointStyle style);
void setCapStyle(CapStyle style);
void setFeather(bool enabled, float amount = 1.0f);
Options& getOptions();  // For advanced customization

} // namespace ofxVase
