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
 * - Vertex-alpha anti-aliasing via outset fade polygons
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
    float worldToScreenRatio = 1.0f;
    int smoothing = 0;
    float miterLimit = 4.0f;
    
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
    Options& setMiterLimit(float limit) { miterLimit = limit; return *this; }
};

// ============================================================================
// Vertex Array Holder - Accumulates mesh data
// ============================================================================

class VertexArrayHolder {
public:
    static constexpr int DRAW_TRIANGLES = 0;
    static constexpr int DRAW_TRIANGLE_STRIP = 1;
    
    int glmode = DRAW_TRIANGLES;
    
    std::vector<glm::vec3> vertices;
    std::vector<ofFloatColor> colors;
    
    void clear();
    int getCount() const { return static_cast<int>(vertices.size()); }
    void setGlDrawMode(int mode) { glmode = mode; }
    
    int push(const glm::vec2& pos, const ofFloatColor& color);
    
    // Push with alpha forced to 0 (for anti-aliased outer edges)
    int pushF(const glm::vec2& pos, const ofFloatColor& color);
    
    void push3(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3,
               const ofFloatColor& c1, const ofFloatColor& c2, const ofFloatColor& c3);
    
    void push4(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3, const glm::vec2& p4,
               const ofFloatColor& c1, const ofFloatColor& c2, const ofFloatColor& c3, const ofFloatColor& c4);
    
    // Merge another holder
    void push(const VertexArrayHolder& other);
    
    glm::vec2 get(int i) const;
    
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
    
    Polyline(const std::vector<glm::vec2>& points, 
             const ofFloatColor& color, 
             float width,
             const Options& opt = Options());
    
    Polyline(const std::vector<glm::vec2>& points,
             const std::vector<ofFloatColor>& colors,
             const std::vector<float>& widths,
             const Options& opt = Options());
    
    Polyline(const ofPolyline& poly,
             const ofFloatColor& color,
             float width,
             const Options& opt = Options());
    
    ofMesh getMesh() const { return holder.toMesh(); }
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
        glm::vec2 vP{0};     // vector to intersection point (outward, core)
        glm::vec2 vR{0};     // fading vector at sharp end (outward)
        glm::vec2 T{0};      // core thickness vector (outward)
        glm::vec2 R{0};      // fading edge vector
        glm::vec2 bR{0};     // out stepping vector, same direction as cap
        glm::vec2 T1{0};     // alternate core vector
        glm::vec2 R1{0};     // alternate fade vector
        float t = 0, r = 0;
        bool degenT = false;  // core degenerated
        bool degenR = false;  // fade degenerated
        bool preFull = false;
        glm::vec2 PT{0};     // degeneration point (core)
        glm::vec2 PR{0};     // degeneration point (fade)
        float pt = 0;        // parameter at intersection
        bool R_full_degen = false;
        char djoint = 0;
    };
    
    struct StAnchor {
        glm::vec2 P[3];
        ofFloatColor C[3];
        float W[3];
        StPolyline SL[3];
        VertexArrayHolder vah;
        glm::vec2 capStart{0}, capEnd{0};
    };
    
    static void determineTr(float w, float& t, float& R, float scale);
    static float getPljRoundDangle(float t, float r, float scale);
    
    static void makeTrc(const glm::vec2& P1, const glm::vec2& P2,
                        glm::vec2& T, glm::vec2& R, glm::vec2& C,
                        float w, const Options& opt,
                        float& rr, float& tt, float& dist);
    
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
    
    static void brushArc(VertexArrayHolder& tris,
                         const glm::vec2& center, const ofFloatColor& col,
                         float t, float r,
                         const glm::vec2& N_start, const glm::vec2& N_end,
                         float wsr);
    
    static void segment(StAnchor& SA, const Options& opt,
                        bool capFirst, bool capLast, bool core);
    
    static void segmentLate(const Options& opt,
                            glm::vec2* P, ofFloatColor* C, StPolyline* SL,
                            VertexArrayHolder& tris,
                            const glm::vec2& cap1, const glm::vec2& cap2, bool core);
    
    static void anchor(StAnchor& SA, const Options& opt, bool capFirst, bool capLast);
    
    static void anchorLate(const Options& opt,
                           glm::vec2* P, ofFloatColor* C, StPolyline* SL,
                           VertexArrayHolder& tris,
                           const glm::vec2& cap1, const glm::vec2& cap2);
    
    static void anchorCap(const Options& opt,
                          glm::vec2* P, ofFloatColor* C, StPolyline* SL,
                          VertexArrayHolder& tris,
                          const glm::vec2& cap1, const glm::vec2& cap2);
    
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
// Renderer - Batch rendering helper (enables alpha blending)
// ============================================================================

class Renderer {
public:
    Renderer();
    ~Renderer();
    
    void setup();
    
    void begin();
    void end();
    
    void draw(const VertexArrayHolder& holder);
    void draw(const Polyline& polyline);
    void draw(const Segment& segment);
    
private:
    bool initialized = false;
};

// ============================================================================
// Utility Functions
// ============================================================================

namespace util {
    float normalize(glm::vec2& v);
    void perpen(glm::vec2& v);
    void opposite(glm::vec2& v);
    float signedArea(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3);
    void dot(const glm::vec2& a, const glm::vec2& b, glm::vec2& out);
    bool anchorOutward(glm::vec2& v, const glm::vec2& b, const glm::vec2& c, bool reverse = false);
    void followSigns(glm::vec2& v, const glm::vec2& a);
    int intersect(const glm::vec2& p1, const glm::vec2& p2,
                  const glm::vec2& p3, const glm::vec2& p4,
                  glm::vec2& out, float* pts = nullptr);
    bool intersecting(const glm::vec2& a, const glm::vec2& b,
                      const glm::vec2& c, const glm::vec2& d);
    
    ofFloatColor colorBetween(const ofFloatColor& a, const ofFloatColor& b, float t);
    
    glm::vec2 catmullRom(const glm::vec2& p0, const glm::vec2& p1, 
                         const glm::vec2& p2, const glm::vec2& p3, float t);
    
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

void draw(const ofPolyline& polyline, float width = 2.0f);
void draw(const ofPolyline& polyline, const ofColor& color, float width = 2.0f);
void draw(const ofPolyline& polyline, const ofColor& color, 
          const std::vector<float>& widths);
void draw(const ofPolyline& polyline, 
          const std::vector<ofColor>& colors,
          const std::vector<float>& widths);

void drawLine(float x1, float y1, float x2, float y2, float width = 2.0f);
void drawLine(const glm::vec2& p1, const glm::vec2& p2, float width = 2.0f);
void drawLine(const glm::vec2& p1, const glm::vec2& p2, 
              const ofColor& color, float width = 2.0f);
void drawLine(const glm::vec2& p1, const glm::vec2& p2,
              float width1, float width2);
void drawLine(const glm::vec2& p1, const glm::vec2& p2,
              const ofColor& c1, const ofColor& c2,
              float width1, float width2);

void setJointStyle(JointStyle style);
void setCapStyle(CapStyle style);
void setFeather(bool enabled, float amount = 1.0f);
Options& getOptions();

} // namespace ofxVase
