#include "ofxVase.h"

namespace ofxVase {

// ============================================================================
// Utility Functions
// ============================================================================

namespace util {

float normalize(glm::vec2& v) {
    float len = glm::length(v);
    if (len > 0.00001f) {
        v /= len;
    }
    return len;
}

void perpen(glm::vec2& v) {
    // Rotate 90 degrees clockwise (right-hand perpendicular)
    float temp = v.x;
    v.x = v.y;
    v.y = -temp;
}

void opposite(glm::vec2& v) {
    v = -v;
}

float signedArea(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3) {
    return (p2.x - p1.x) * (p3.y - p1.y) - (p3.x - p1.x) * (p2.y - p1.y);
}

void dot(const glm::vec2& a, const glm::vec2& b, glm::vec2& out) {
    out.x = a.x * b.x;
    out.y = a.y * b.y;
}

void anchorOutward(glm::vec2& v, const glm::vec2& b, const glm::vec2& c) {
    float determinant = (b.x * v.x - c.x * v.x + b.y * v.y - c.y * v.y);
    if (determinant <= 0) {
        v = -v;
    }
}

void followSigns(glm::vec2& v, const glm::vec2& a) {
    if ((v.x > 0) != (a.x > 0)) v.x = -v.x;
    if ((v.y > 0) != (a.y > 0)) v.y = -v.y;
}

int intersect(const glm::vec2& p1, const glm::vec2& p2,
              const glm::vec2& p3, const glm::vec2& p4,
              glm::vec2& out, float* pts) {
    float denom = (p4.y - p3.y) * (p2.x - p1.x) - (p4.x - p3.x) * (p2.y - p1.y);
    float numera = (p4.x - p3.x) * (p1.y - p3.y) - (p4.y - p3.y) * (p1.x - p3.x);
    float numerb = (p2.x - p1.x) * (p1.y - p3.y) - (p2.y - p1.y) * (p1.x - p3.x);
    
    const float eps = 0.00000001f;
    if (fabs(numera) < eps && fabs(numerb) < eps && fabs(denom) < eps) {
        out = (p1 + p2) * 0.5f;
        return 2; // Lines coincide
    }
    
    if (fabs(denom) < eps) {
        out = glm::vec2(0);
        return 0; // Parallel
    }
    
    float mua = numera / denom;
    float mub = numerb / denom;
    if (pts) {
        pts[0] = mua;
        pts[1] = mub;
    }
    
    out.x = p1.x + mua * (p2.x - p1.x);
    out.y = p1.y + mua * (p2.y - p1.y);
    
    bool out1 = mua < 0 || mua > 1;
    bool out2 = mub < 0 || mub > 1;
    
    if (out1 && out2) return 5;
    if (out1) return 3;
    if (out2) return 4;
    return 1;
}

ofFloatColor colorBetween(const ofFloatColor& a, const ofFloatColor& b, float t) {
    t = glm::clamp(t, 0.0f, 1.0f);
    float kt = 1.0f - t;
    return ofFloatColor(
        a.r * kt + b.r * t,
        a.g * kt + b.g * t,
        a.b * kt + b.b * t,
        a.a * kt + b.a * t
    );
}

glm::vec2 catmullRom(const glm::vec2& p0, const glm::vec2& p1, 
                     const glm::vec2& p2, const glm::vec2& p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    
    return 0.5f * (
        (2.0f * p1) +
        (-p0 + p2) * t +
        (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
        (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
    );
}

void smoothPolyline(const std::vector<glm::vec2>& points,
                    const std::vector<ofFloatColor>& colors,
                    const std::vector<float>& widths,
                    int subdivisions,
                    std::vector<glm::vec2>& outPoints,
                    std::vector<ofFloatColor>& outColors,
                    std::vector<float>& outWidths) {
    if (points.size() < 2 || subdivisions < 1) {
        outPoints = points;
        outColors = colors;
        outWidths = widths;
        return;
    }
    
    int n = (int)points.size();
    int outSize = (n - 1) * subdivisions + 1;
    outPoints.reserve(outSize);
    outColors.reserve(outSize);
    outWidths.reserve(outSize);
    
    for (int i = 0; i < n - 1; i++) {
        // Get 4 control points for Catmull-Rom
        int i0 = std::max(0, i - 1);
        int i1 = i;
        int i2 = i + 1;
        int i3 = std::min(n - 1, i + 2);
        
        const glm::vec2& p0 = points[i0];
        const glm::vec2& p1 = points[i1];
        const glm::vec2& p2 = points[i2];
        const glm::vec2& p3 = points[i3];
        
        for (int j = 0; j < subdivisions; j++) {
            float t = (float)j / subdivisions;
            
            // Interpolate position using Catmull-Rom
            outPoints.push_back(catmullRom(p0, p1, p2, p3, t));
            
            // Linear interpolate color and width
            float globalT = (i + t) / (n - 1);
            float segT = t;
            
            outColors.push_back(colorBetween(colors[i1], colors[i2], segT));
            outWidths.push_back(widths[i1] * (1.0f - segT) + widths[i2] * segT);
        }
    }
    
    // Add final point
    outPoints.push_back(points.back());
    outColors.push_back(colors.back());
    outWidths.push_back(widths.back());
}

} // namespace util

// ============================================================================
// VertexArrayHolder
// ============================================================================

void VertexArrayHolder::clear() {
    vertices.clear();
    uvs.clear();
    uvs2.clear();
    colors.clear();
    jumping = false;
}

void VertexArrayHolder::push(const glm::vec2& pos, const ofFloatColor& color, float rr) {
    push(pos, color, rr, 0.0f);
}

void VertexArrayHolder::push(const glm::vec2& pos, const ofFloatColor& color, float rrX, float rrY) {
    vertices.push_back(glm::vec3(pos, 0));
    
    // Store rr values in texture coordinates for shader-based fade
    // Also pre-multiply alpha for fallback non-shader rendering
    ofFloatColor c = color;
    // If rr is negative, this is a fade vertex (alpha = 0)
    if (rrX < 0 || rrY < 0) {
        c.a = 0.0f;
    }
    colors.push_back(c);
    
    // Pack rr values into UV for shader use
    uvs.push_back(glm::vec2(rrX, rrY));
    uvs2.push_back(glm::vec2(rrX, rrY));
    
    if (jumping) {
        jumping = false;
        repeatLastPush();
    }
}

void VertexArrayHolder::push3(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3,
                               const ofFloatColor& c1, const ofFloatColor& c2, const ofFloatColor& c3,
                               float rr1, float rr2, float rr3) {
    push(p1, c1, rr1);
    push(p2, c2, rr2);
    push(p3, c3, rr3);
}

void VertexArrayHolder::pushFade(const glm::vec2& pos, const ofFloatColor& color) {
    ofFloatColor faded = color;
    faded.a = 0;
    push(pos, faded, 0);
}

void VertexArrayHolder::push(const VertexArrayHolder& other) {
    if (glmode == other.glmode) {
        vertices.insert(vertices.end(), other.vertices.begin(), other.vertices.end());
        colors.insert(colors.end(), other.colors.begin(), other.colors.end());
        uvs.insert(uvs.end(), other.uvs.begin(), other.uvs.end());
        uvs2.insert(uvs2.end(), other.uvs2.begin(), other.uvs2.end());
    } else if (glmode == DRAW_TRIANGLES && other.glmode == DRAW_TRIANGLE_STRIP) {
        // Convert strip to triangles
        for (size_t b = 2; b < other.vertices.size(); b++) {
            for (int k = 0; k < 3; k++) {
                size_t idx = b - 2 + k;
                vertices.push_back(other.vertices[idx]);
                colors.push_back(other.colors[idx]);
                uvs.push_back(other.uvs[idx]);
                uvs2.push_back(other.uvs2[idx]);
            }
        }
    }
}

glm::vec2 VertexArrayHolder::get(int i) const {
    if (i >= 0 && i < static_cast<int>(vertices.size())) {
        return glm::vec2(vertices[i]);
    }
    return glm::vec2(0);
}

ofMesh VertexArrayHolder::toMesh() const {
    ofMesh mesh;
    mesh.setMode(glmode == DRAW_TRIANGLES ? OF_PRIMITIVE_TRIANGLES : OF_PRIMITIVE_TRIANGLE_STRIP);
    
    for (size_t i = 0; i < vertices.size(); i++) {
        mesh.addVertex(vertices[i]);
        mesh.addColor(colors[i]);
        mesh.addTexCoord(uvs[i]);
        // Note: OF mesh doesn't have built-in support for secondary UVs
        // We'll pack them differently or use a custom attribute
    }
    
    return mesh;
}

void VertexArrayHolder::jump() {
    if (glmode == DRAW_TRIANGLE_STRIP) {
        repeatLastPush();
        jumping = true;
    }
}

void VertexArrayHolder::repeatLastPush() {
    if (vertices.empty()) return;
    vertices.push_back(vertices.back());
    colors.push_back(colors.back());
    uvs.push_back(uvs.back());
    uvs2.push_back(uvs2.back());
}

// ============================================================================
// Polyline - Core Tessellation
// ============================================================================

void Polyline::determineTr(float w, float& t, float& R, float scale) {
    // Simple direct mapping: t = half-width (radius), R = feather amount
    // The width w is the full stroke width, so we use w/2 for the radius
    (void)scale;  // Not used in simple mode
    
    t = w * 0.5f;  // Half-width = radius from center to edge
    R = 1.5f;      // Fixed feather amount for anti-aliasing (when enabled)
    
    // Ensure minimum thickness for very thin lines
    if (t < 0.5f) {
        t = 0.5f;
    }
}

float Polyline::getPljRoundDangle(float t, float r, float scale) {
    float sum = (t + r) * scale;
    if (sum <= 1.44f + 1.08f) {
        return 0.6f / sum;
    } else if (sum <= 3.25f + 1.08f) {
        return 2.8f / sum;
    } else {
        return 4.2f / sum;
    }
}

void Polyline::makeTrc(const glm::vec2& P1, const glm::vec2& P2,
                       glm::vec2& T, glm::vec2& R, glm::vec2& C,
                       float w, const Options& opt,
                       float& rr, float& tt, float& dist, bool anchorMode) {
    float t = 1.0f, r = 0.0f;
    
    determineTr(w, t, r, opt.worldToScreenRatio);
    if (opt.feather && !opt.noFeatherAtCore) {
        r *= opt.feathering;
    }
    if (anchorMode) {
        t += r;
    }
    
    tt = t;
    rr = r;
    
    glm::vec2 DP = P2 - P1;
    dist = util::normalize(DP);
    C = DP * (1.0f / opt.worldToScreenRatio);
    util::perpen(DP);
    T = DP * t;
    R = DP * r;
}

Polyline::Polyline(const std::vector<glm::vec2>& points,
                   const ofFloatColor& color,
                   float width,
                   const Options& opt) {
    std::vector<ofFloatColor> colors = { color };
    std::vector<float> widths = { width };
    
    InternalOpt inopt;
    inopt.constColor = true;
    inopt.constWeight = true;
    
    int length = static_cast<int>(points.size());
    if (length < 2) return;
    
    Options localOpt = opt;
    if (static_cast<int>(localOpt.capPosition) >= 10) {
        char dec = static_cast<char>(static_cast<int>(localOpt.capPosition) - 
                                     (static_cast<int>(localOpt.capPosition) % 10));
        if (dec == static_cast<char>(CapPosition::First) || 
            dec == static_cast<char>(CapPosition::None)) {
            inopt.noCapLast = true;
        }
        if (dec == static_cast<char>(CapPosition::Last) || 
            dec == static_cast<char>(CapPosition::None)) {
            inopt.noCapFirst = true;
        }
    }
    
    // Simple case: just 2 points = single segment
    if (length == 2) {
        StAnchor SA;
        SA.P[0] = points[0];
        SA.P[1] = points[1];
        SA.C[0] = color;
        SA.C[1] = color;
        SA.W[0] = width;
        SA.W[1] = width;
        segment(SA, localOpt, !inopt.noCapFirst, !inopt.noCapLast, true);
        holder = SA.vah;
        return;
    }
    
    // Multi-point polyline
    polylineRange(points, colors, widths, localOpt, inopt, 0, length - 1, false);
    holder = inopt.holder;
}

Polyline::Polyline(const std::vector<glm::vec2>& points,
                   const std::vector<ofFloatColor>& colors,
                   const std::vector<float>& widths,
                   const Options& opt) {
    InternalOpt inopt;
    
    // Apply smoothing if requested
    if (opt.smoothing > 0 && points.size() >= 2) {
        std::vector<glm::vec2> smoothPts;
        std::vector<ofFloatColor> smoothColors;
        std::vector<float> smoothWidths;
        
        // Expand single color/width to match points
        std::vector<ofFloatColor> expandedColors = colors;
        std::vector<float> expandedWidths = widths;
        if (colors.size() == 1) {
            expandedColors.resize(points.size(), colors[0]);
        }
        if (widths.size() == 1) {
            expandedWidths.resize(points.size(), widths[0]);
        }
        
        util::smoothPolyline(points, expandedColors, expandedWidths, opt.smoothing,
                            smoothPts, smoothColors, smoothWidths);
        
        inopt.constColor = false;
        inopt.constWeight = false;
        
        int length = static_cast<int>(smoothPts.size());
        polylineRange(smoothPts, smoothColors, smoothWidths, opt, inopt, 0, length - 1, false);
    } else {
        inopt.constColor = (colors.size() == 1);
        inopt.constWeight = (widths.size() == 1);
        
        int length = static_cast<int>(points.size());
        if (length < 2) return;
        
        polylineRange(points, colors, widths, opt, inopt, 0, length - 1, false);
    }
    holder = inopt.holder;
}

Polyline::Polyline(const ofPolyline& poly,
                   const ofFloatColor& color,
                   float width,
                   const Options& opt) {
    std::vector<glm::vec2> points;
    points.reserve(poly.size());
    for (const auto& v : poly.getVertices()) {
        points.push_back(glm::vec2(v.x, v.y));
    }
    *this = Polyline(points, color, width, opt);
}

void Polyline::polylineRange(const std::vector<glm::vec2>& P,
                              const std::vector<ofFloatColor>& C,
                              const std::vector<float>& W,
                              const Options& opt, InternalOpt& inopt,
                              int from, int to, bool approx) {
    if (from > 0) from--;
    
    inopt.joinFirst = (from != 0);
    inopt.joinLast = (to != static_cast<int>(P.size()) - 1);
    inopt.noCapFirst = inopt.noCapFirst || inopt.joinFirst;
    inopt.noCapLast = inopt.noCapLast || inopt.joinLast;
    
    if (approx) {
        polylineApprox(P, C, W, opt, inopt, from, to);
    } else {
        polylineExact(P, C, W, opt, inopt, from, to);
    }
}

void Polyline::polylineApprox(const std::vector<glm::vec2>& P,
                               const std::vector<ofFloatColor>& C,
                               const std::vector<float>& W,
                               const Options& opt, InternalOpt& inopt,
                               int from, int to) {
    // Simplified approximation - just use exact for now
    polylineExact(P, C, W, opt, inopt, from, to);
}

void Polyline::polylineExact(const std::vector<glm::vec2>& P,
                              const std::vector<ofFloatColor>& C,
                              const std::vector<float>& W,
                              const Options& opt, InternalOpt& inopt,
                              int from, int to) {
    bool capFirst = !inopt.noCapFirst;
    bool capLast = !inopt.noCapLast;
    bool joinFirst = inopt.joinFirst;
    bool joinLast = inopt.joinLast;
    
    auto color = [&](int i) -> ofFloatColor {
        return C[inopt.constColor ? 0 : i];
    };
    auto weight = [&](int i) -> float {
        return W[inopt.constWeight ? 0 : i];
    };
    
    glm::vec2 mid_l, mid_n;
    ofFloatColor c_l, c_n;
    float w_l = 0, w_n = 0;
    
    // Init for first anchor
    if (joinFirst) {
        mid_l = (P[from] + P[from + 1]) * 0.5f;
        c_l = util::colorBetween(color(from), color(from + 1), 0.5f);
        w_l = (weight(from) + weight(from + 1)) * 0.5f;
    } else {
        mid_l = P[from];
        c_l = color(from);
        w_l = weight(from);
    }
    
    StAnchor SA;
    
    if (to - from + 1 == 2) {
        // Just 2 points - single segment
        SA.P[0] = P[from];
        SA.P[1] = P[from + 1];
        SA.C[0] = color(from);
        SA.C[1] = color(from + 1);
        SA.W[0] = weight(from);
        SA.W[1] = weight(from + 1);
        segment(SA, opt, capFirst, capLast, true);
    } else {
        // Multiple segments with anchors
        for (int i = from + 1; i < to; i++) {
            if (i == to - 1 && !joinLast) {
                mid_n = P[i + 1];
                c_n = color(i + 1);
                w_n = weight(i + 1);
            } else {
                mid_n = (P[i] + P[i + 1]) * 0.5f;
                c_n = util::colorBetween(color(i), color(i + 1), 0.5f);
                w_n = (weight(i) + weight(i + 1)) * 0.5f;
            }
            
            SA.P[0] = mid_l;
            SA.C[0] = c_l;
            SA.W[0] = w_l;
            SA.P[2] = mid_n;
            SA.C[2] = c_n;
            SA.W[2] = w_n;
            
            SA.P[1] = P[i];
            SA.C[1] = color(i);
            SA.W[1] = weight(i);
            
            anchor(SA, opt, (i == from + 1) && capFirst, (i == to - 1) && capLast);
            
            mid_l = mid_n;
            c_l = c_n;
            w_l = w_n;
        }
    }
    
    inopt.holder.push(SA.vah);
}

void Polyline::segment(StAnchor& SA, const Options& opt,
                        bool capFirst, bool capLast, bool core) {
    float* weight = SA.W;
    glm::vec2 P[2] = { SA.P[0], SA.P[1] };
    ofFloatColor C[2] = { SA.C[0], SA.C[1] };
    
    glm::vec2 T2, R2, bR;
    float t = 0, r = 0;
    
    bool varying_weight = weight[0] != weight[1];
    
    glm::vec2 capStart(0), capEnd(0);
    StPolyline SL[2];
    
    // First point
    {
        float dd = 0;
        makeTrc(P[0], P[1], T2, R2, bR, weight[0], opt, r, t, dd, false);
        
        if (capFirst) {
            if (opt.cap == CapStyle::Square) {
                P[0] -= bR * (t + r);
            }
            capStart = -bR;
            if (opt.feather && !opt.noFeatherAtCap) {
                capStart *= opt.feathering;
            }
        }
        
        SL[0].djoint = static_cast<char>(opt.cap);
        SL[0].t = t;
        SL[0].r = r;
        SL[0].T = T2;
        SL[0].R = R2;
        SL[0].bR = bR;
        SL[0].degenT = false;
        SL[0].degenR = false;
    }
    
    // Second point
    {
        float dd = 0;
        if (varying_weight) {
            makeTrc(P[0], P[1], T2, R2, bR, weight[1], opt, r, t, dd, false);
        }
        
        if (capLast) {
            if (opt.cap == CapStyle::Square) {
                P[1] += bR * (t + r);
            }
            capEnd = bR;
            if (opt.feather && !opt.noFeatherAtCap) {
                capEnd *= opt.feathering;
            }
        }
        
        SL[1].djoint = static_cast<char>(opt.cap);
        SL[1].t = t;
        SL[1].r = r;
        SL[1].T = T2;
        SL[1].R = R2;
        SL[1].bR = bR;
        SL[1].degenT = false;
        SL[1].degenR = false;
    }
    
    segmentLate(opt, P, C, SL, SA.vah, capStart, capEnd, core);
}

void Polyline::segmentLate(const Options& opt,
                           glm::vec2* P, ofFloatColor* C, StPolyline* SL,
                           VertexArrayHolder& tris,
                           const glm::vec2& cap1, const glm::vec2& cap2, bool core) {
    tris.setGlDrawMode(VertexArrayHolder::DRAW_TRIANGLES);
    
    glm::vec2 P_0 = P[0];
    glm::vec2 P_1 = P[1];
    
    // P1-P4: outer edge (with fade)
    // P1r-P4r: inner edge (core, full alpha)
    glm::vec2 P1, P2, P3, P4;
    glm::vec2 P1c, P2c, P3c, P4c;
    glm::vec2 P1r, P2r, P3r, P4r;
    
    float s0 = 1, s1 = 1;
    if (SL[0].t < SL[1].t) {
        s0 = (SL[0].t + SL[0].r) / (SL[1].t + SL[1].r);
    }
    if (SL[1].t < SL[0].t) {
        s1 = (SL[1].t + SL[1].r) / (SL[0].t + SL[0].r);
    }
    
    // Outer edge points (where alpha = 0 if feathering)
    P1 = P_0 + SL[0].T + SL[0].R;
    P2 = P_0 - SL[0].T - SL[0].R;
    P3 = P_1 + SL[1].T + SL[1].R;
    P4 = P_1 - SL[1].T - SL[1].R;
    
    // Inner edge points (core, full alpha)
    P1r = P_0 + SL[0].T;
    P2r = P_0 - SL[0].T;
    P3r = P_1 + SL[1].T;
    P4r = P_1 - SL[1].T;
    
    // Cap extension points
    P1c = P1 + cap1 * s0;
    P2c = P2 + cap1 * s0;
    P3c = P3 + cap2 * s1;
    P4c = P4 + cap2 * s1;
    
    // Core - solid triangles (full alpha)
    if (core) {
        // Two triangles for the solid core
        tris.push3(P1r, P3r, P2r, C[0], C[1], C[0], 0, 0, 0);
        tris.push3(P2r, P3r, P4r, C[0], C[1], C[1], 0, 0, 0);
    }
    
    // Feathering - fade triangles around the core
    if (opt.feather && SL[0].r > 0.001f) {
        // Create faded colors
        ofFloatColor C0_fade = C[0]; C0_fade.a = 0;
        ofFloatColor C1_fade = C[1]; C1_fade.a = 0;
        
        // Top fade strip (P1r -> P1, P3r -> P3)
        tris.push3(P1r, P3r, P1, C[0], C[1], C0_fade, 0, 0, 0);
        tris.push3(P1, P3r, P3, C0_fade, C[1], C1_fade, 0, 0, 0);
        
        // Bottom fade strip (P2r -> P2, P4r -> P4)
        tris.push3(P2r, P2, P4r, C[0], C0_fade, C[1], 0, 0, 0);
        tris.push3(P2, P4, P4r, C0_fade, C1_fade, C[1], 0, 0, 0);
    }
    
    // Draw caps
    for (int j = 0; j < 2; j++) {
        glm::vec2 cur_cap = j == 0 ? cap1 : cap2;
        if (glm::length(cur_cap) < 0.001f) continue;
        
        if (SL[j].djoint == static_cast<char>(CapStyle::Round)) {
            // Round cap - draw a semicircle as a triangle fan
            glm::vec2 O = j == 0 ? P_0 : P_1;
            float coreRadius = SL[j].t;
            
            // Get the direction the cap extends (outward from segment)
            glm::vec2 capDir = glm::normalize(cur_cap);
            
            // Get perpendicular to cap direction (the edge direction)
            glm::vec2 edgeDir(-capDir.y, capDir.x);
            
            // The two edge points where the cap meets the segment
            glm::vec2 edgeP1 = O + edgeDir * coreRadius;
            glm::vec2 edgeP2 = O - edgeDir * coreRadius;
            
            // Draw semicircle from edgeP1 to edgeP2, curving outward in capDir
            int segments = std::max(12, (int)(coreRadius * 0.5f));
            
            for (int i = 0; i < segments; i++) {
                // Angle from -90 to +90 degrees relative to capDir
                float t1 = (float)i / segments;
                float t2 = (float)(i + 1) / segments;
                float angle1 = glm::mix(-glm::half_pi<float>(), glm::half_pi<float>(), t1);
                float angle2 = glm::mix(-glm::half_pi<float>(), glm::half_pi<float>(), t2);
                
                // Rotate capDir by angle to get arc points
                glm::vec2 dir1(capDir.x * cos(angle1) - capDir.y * sin(angle1),
                               capDir.x * sin(angle1) + capDir.y * cos(angle1));
                glm::vec2 dir2(capDir.x * cos(angle2) - capDir.y * sin(angle2),
                               capDir.x * sin(angle2) + capDir.y * cos(angle2));
                
                glm::vec2 p1 = O + dir1 * coreRadius;
                glm::vec2 p2 = O + dir2 * coreRadius;
                
                // Triangle from center to two arc points
                tris.push3(O, p1, p2, C[j], C[j], C[j], 0, 0, 0);
            }
        } else if (SL[j].djoint == static_cast<char>(CapStyle::Rect) ||
                   SL[j].djoint == static_cast<char>(CapStyle::Square)) {
            // Square/Rect cap
            glm::vec2 O = j == 0 ? P_0 : P_1;
            glm::vec2 capDir = glm::normalize(cur_cap);
            glm::vec2 perp = glm::normalize(SL[j].T);
            
            float coreRadius = SL[j].t;
            float fullRadius = SL[j].t + SL[j].r;
            float capLen = SL[j].t + SL[j].r;
            
            // Core quad
            glm::vec2 c1 = O + perp * coreRadius;
            glm::vec2 c2 = O - perp * coreRadius;
            glm::vec2 c3 = O + perp * coreRadius + capDir * capLen;
            glm::vec2 c4 = O - perp * coreRadius + capDir * capLen;
            
            tris.push3(c1, c2, c3, C[j], C[j], C[j], 0, 0, 0);
            tris.push3(c2, c4, c3, C[j], C[j], C[j], 0, 0, 0);
            
            // Fade edges if feathering
            if (opt.feather && SL[j].r > 0.001f) {
                ofFloatColor Cj_fade = C[j];
                Cj_fade.a = 0;
                
                glm::vec2 o1 = O + perp * fullRadius;
                glm::vec2 o2 = O - perp * fullRadius;
                glm::vec2 o3 = O + perp * fullRadius + capDir * capLen;
                glm::vec2 o4 = O - perp * fullRadius + capDir * capLen;
                
                // Side fades
                tris.push3(c1, o1, c3, C[j], Cj_fade, C[j], 0, 0, 0);
                tris.push3(o1, o3, c3, Cj_fade, Cj_fade, C[j], 0, 0, 0);
                tris.push3(c2, c4, o2, C[j], C[j], Cj_fade, 0, 0, 0);
                tris.push3(o2, c4, o4, Cj_fade, C[j], Cj_fade, 0, 0, 0);
                
                // End fade
                tris.push3(c3, o3, c4, C[j], Cj_fade, C[j], 0, 0, 0);
                tris.push3(o3, o4, c4, Cj_fade, Cj_fade, C[j], 0, 0, 0);
            }
        }
    }
}

void Polyline::anchor(StAnchor& SA, const Options& opt, bool capFirst, bool capLast) {
    glm::vec2* P = SA.P;
    ofFloatColor* C = SA.C;
    float* W = SA.W;
    
    // Get directions and normals for both segments
    glm::vec2 dir1 = glm::normalize(P[1] - P[0]);
    glm::vec2 dir2 = glm::normalize(P[2] - P[1]);
    glm::vec2 norm1(-dir1.y, dir1.x);
    glm::vec2 norm2(-dir2.y, dir2.x);
    
    // Calculate thickness at the joint
    float t1 = 0, r1 = 0;
    determineTr(W[1], t1, r1, opt.worldToScreenRatio);
    
    float coreWidth = t1;           // Core (solid) width
    float fullWidth = t1 + r1;      // Full width including fade
    
    // Determine which side the joint is on (cross product)
    float cross = dir1.x * dir2.y - dir1.y * dir2.x;
    
    // Core points (solid fill) for joint
    glm::vec2 P1a_core = P[1] + norm1 * coreWidth;
    glm::vec2 P1b_core = P[1] - norm1 * coreWidth;
    glm::vec2 P2a_core = P[1] + norm2 * coreWidth;
    glm::vec2 P2b_core = P[1] - norm2 * coreWidth;
    
    // Outer points (fade ends) for joint
    glm::vec2 P1a_outer = P[1] + norm1 * fullWidth;
    glm::vec2 P1b_outer = P[1] - norm1 * fullWidth;
    glm::vec2 P2a_outer = P[1] + norm2 * fullWidth;
    glm::vec2 P2b_outer = P[1] - norm2 * fullWidth;
    
    // Draw first segment (from P[0] to P[1])
    StAnchor seg1;
    seg1.P[0] = P[0];
    seg1.P[1] = P[1];
    seg1.C[0] = C[0];
    seg1.C[1] = C[1];
    seg1.W[0] = W[0];
    seg1.W[1] = W[1];
    segment(seg1, opt, capFirst, false, true);
    SA.vah.push(seg1.vah);
    
    // Fill the joint gap with a rounded arc on the outside of the turn
    const float crossThreshold = 0.0001f;
    if (std::abs(cross) > crossThreshold) {
        // Determine which edge points to use based on turn direction
        glm::vec2 edgeA, edgeB;  // The two edge points we need to connect
        if (cross > 0) {
            // Turning left - gap is on the negative normal side (b points)
            edgeA = P1b_core;  // End of segment 1
            edgeB = P2b_core;  // Start of segment 2
        } else {
            // Turning right - gap is on the positive normal side (a points)
            edgeA = P1a_core;
            edgeB = P2a_core;
        }
        
        // Simple approach: interpolate along arc from edgeA to edgeB
        // Use spherical-ish interpolation to maintain radius
        glm::vec2 dirA = edgeA - P[1];
        glm::vec2 dirB = edgeB - P[1];
        float radiusA = glm::length(dirA);
        float radiusB = glm::length(dirB);
        float avgRadius = (radiusA + radiusB) * 0.5f;
        
        // Normalize directions
        if (radiusA > 0.001f) dirA /= radiusA;
        if (radiusB > 0.001f) dirB /= radiusB;
        
        // Calculate angle between the two directions
        float dotVal = glm::clamp(glm::dot(dirA, dirB), -1.0f, 1.0f);
        float angle = acos(dotVal);
        
        // Number of segments based on angle
        int segments = std::max(2, (int)(angle * avgRadius * 0.3f));
        
        // Draw fan of triangles
        glm::vec2 prevPoint = edgeA;
        for (int i = 1; i <= segments; i++) {
            float t = (float)i / segments;
            
            // Spherical interpolation of direction
            glm::vec2 interpDir = glm::normalize(dirA * (1.0f - t) + dirB * t);
            float interpRadius = radiusA * (1.0f - t) + radiusB * t;
            glm::vec2 currPoint = P[1] + interpDir * interpRadius;
            
            SA.vah.push3(P[1], prevPoint, currPoint, C[1], C[1], C[1], 0, 0, 0);
            prevPoint = currPoint;
        }
    }
    
    (void)P1a_outer; (void)P1b_outer; (void)P2a_outer; (void)P2b_outer;
    (void)fullWidth;
    
    // Draw second segment (from P[1] to P[2])
    StAnchor seg2;
    seg2.P[0] = P[1];
    seg2.P[1] = P[2];
    seg2.C[0] = C[1];
    seg2.C[1] = C[2];
    seg2.W[0] = W[1];
    seg2.W[1] = W[2];
    segment(seg2, opt, false, capLast, true);
    SA.vah.push(seg2.vah);
}

void Polyline::vectorsToArc(VertexArrayHolder& hold,
                            const glm::vec2& P, const ofFloatColor& C, const ofFloatColor& C2,
                            const glm::vec2& PA, const glm::vec2& PB,
                            float dangle, float r, float r2,
                            bool ignorEnds, const glm::vec2& apparentP,
                            const glm::vec2& hint, float rr, bool innerFade) {
    const float m_pi = glm::pi<float>();
    glm::vec2 A = PA * (1.0f / r);
    glm::vec2 B = PB * (1.0f / r);
    
    // For vertex alpha approach: outer edge fades, inner stays solid
    ofFloatColor C_outer = C;
    ofFloatColor C_inner = C2;
    if (!innerFade) {
        C_outer.a = 0;  // Outer edge fades to transparent
    }
    
    float angle1 = acos(glm::clamp(A.x, -1.0f, 1.0f));
    float angle2 = acos(glm::clamp(B.x, -1.0f, 1.0f));
    if (A.y > 0) angle1 = 2 * m_pi - angle1;
    if (B.y > 0) angle2 = 2 * m_pi - angle2;
    
    if (angle2 > angle1) {
        if (angle2 - angle1 > m_pi) {
            angle2 = angle2 - 2 * m_pi;
        }
    } else {
        if (angle1 - angle2 > m_pi) {
            angle1 = angle1 - 2 * m_pi;
        }
    }
    
    bool incremental = angle1 <= angle2;
    
    auto inner_arc_push = [&](float x, float y, bool reverse = false) {
        glm::vec2 PP(P.x + x * r, P.y - y * r);
        if (!reverse) {
            hold.push(PP, C_outer, 0);
            hold.push(apparentP, C_inner, 0);
        } else {
            hold.push(apparentP, C_inner, 0);
            hold.push(PP, C_outer, 0);
        }
    };
    
    if (incremental) {
        if (!ignorEnds) {
            hold.push(P + PB, C_outer, 0);
            hold.push(apparentP, C_inner, 0);
        }
        for (float a = angle2 - dangle; a > angle1; a -= dangle) {
            inner_arc_push(cos(a), sin(a));
        }
        if (!ignorEnds) {
            hold.push(P + PA, C_outer, 0);
            hold.push(apparentP, C_inner, 0);
        }
    } else {
        if (!ignorEnds) {
            hold.push(apparentP, C_inner, 0);
            hold.push(P + PB, C_outer, 0);
        }
        for (float a = angle2 + dangle; a < angle1; a += dangle) {
            inner_arc_push(cos(a), sin(a), true);
        }
        if (!ignorEnds) {
            hold.push(apparentP, C_inner, 0);
            hold.push(P + PA, C_outer, 0);
        }
    }
}

// ============================================================================
// Segment
// ============================================================================

Segment::Segment(const glm::vec2& p1, const glm::vec2& p2,
                 const ofFloatColor& c1, const ofFloatColor& c2,
                 float w1, float w2,
                 const Options& opt) {
    std::vector<glm::vec2> points = { p1, p2 };
    std::vector<ofFloatColor> colors = { c1, c2 };
    std::vector<float> widths = { w1, w2 };
    Polyline poly(points, colors, widths, opt);
    holder = poly.holder;
}

Segment::Segment(const glm::vec2& p1, const glm::vec2& p2,
                 const ofFloatColor& color, float width,
                 const Options& opt) {
    std::vector<glm::vec2> points = { p1, p2 };
    Polyline poly(points, color, width, opt);
    holder = poly.holder;
}

// ============================================================================
// Renderer
// ============================================================================

Renderer::Renderer() {}
Renderer::~Renderer() {}

void Renderer::setup() {
    if (initialized) return;
    initialized = true;
    ofLogNotice("ofxVase") << "Renderer initialized (using ofMesh)";
}

void Renderer::begin() {
    if (!initialized) setup();
    ofEnableAlphaBlending();
}

void Renderer::end() {
    // Nothing to do - mesh draws immediately
}

void Renderer::draw(const VertexArrayHolder& holder) {
    if (holder.vertices.empty()) return;
    
    // Always use mesh path - it's reliable and handles vertex colors correctly
    // OF's built-in rendering takes care of shaders automatically
    holder.toMesh().draw();
}

void Renderer::draw(const Polyline& polyline) {
    draw(polyline.holder);
}

void Renderer::draw(const Segment& segment) {
    draw(segment.holder);
}

// ============================================================================
// Simple OF-Style API Implementation
// ============================================================================

namespace {
    // Global state for simple API
    Options g_options;
    Renderer g_renderer;
    bool g_rendererInitialized = false;
    
    Renderer& getRenderer() {
        if (!g_rendererInitialized) {
            g_renderer.setup();
            g_rendererInitialized = true;
        }
        return g_renderer;
    }
}

void draw(const ofPolyline& polyline, float width) {
    ofFloatColor color = ofGetStyle().color;
    draw(polyline, ofColor(color.r * 255, color.g * 255, color.b * 255, color.a * 255), width);
}

void draw(const ofPolyline& polyline, const ofColor& color, float width) {
    if (polyline.size() < 2) return;
    
    Polyline poly(polyline, ofFloatColor(color), width, g_options);
    
    auto& renderer = getRenderer();
    renderer.begin();
    renderer.draw(poly);
    renderer.end();
}

void draw(const ofPolyline& polyline, const ofColor& color, 
          const std::vector<float>& widths) {
    if (polyline.size() < 2) return;
    
    std::vector<glm::vec2> points;
    for (size_t i = 0; i < polyline.size(); i++) {
        points.push_back(glm::vec2(polyline[i].x, polyline[i].y));
    }
    
    std::vector<ofFloatColor> colors(points.size(), ofFloatColor(color));
    
    Polyline poly(points, colors, widths, g_options);
    
    auto& renderer = getRenderer();
    renderer.begin();
    renderer.draw(poly);
    renderer.end();
}

void draw(const ofPolyline& polyline, 
          const std::vector<ofColor>& colors,
          const std::vector<float>& widths) {
    if (polyline.size() < 2) return;
    
    std::vector<glm::vec2> points;
    std::vector<ofFloatColor> floatColors;
    
    for (size_t i = 0; i < polyline.size(); i++) {
        points.push_back(glm::vec2(polyline[i].x, polyline[i].y));
        if (i < colors.size()) {
            floatColors.push_back(ofFloatColor(colors[i]));
        } else if (!colors.empty()) {
            floatColors.push_back(ofFloatColor(colors.back()));
        } else {
            floatColors.push_back(ofFloatColor(1, 1, 1, 1));
        }
    }
    
    Polyline poly(points, floatColors, widths, g_options);
    
    auto& renderer = getRenderer();
    renderer.begin();
    renderer.draw(poly);
    renderer.end();
}

void drawLine(float x1, float y1, float x2, float y2, float width) {
    drawLine(glm::vec2(x1, y1), glm::vec2(x2, y2), width);
}

void drawLine(const glm::vec2& p1, const glm::vec2& p2, float width) {
    ofFloatColor color = ofGetStyle().color;
    drawLine(p1, p2, ofColor(color.r * 255, color.g * 255, color.b * 255, color.a * 255), width);
}

void drawLine(const glm::vec2& p1, const glm::vec2& p2, 
              const ofColor& color, float width) {
    Segment seg(p1, p2, ofFloatColor(color), width, g_options);
    
    auto& renderer = getRenderer();
    renderer.begin();
    renderer.draw(seg);
    renderer.end();
}

void drawLine(const glm::vec2& p1, const glm::vec2& p2,
              float width1, float width2) {
    ofFloatColor color = ofGetStyle().color;
    drawLine(p1, p2, 
             ofColor(color.r * 255, color.g * 255, color.b * 255, color.a * 255),
             ofColor(color.r * 255, color.g * 255, color.b * 255, color.a * 255),
             width1, width2);
}

void drawLine(const glm::vec2& p1, const glm::vec2& p2,
              const ofColor& c1, const ofColor& c2,
              float width1, float width2) {
    Segment seg(p1, p2, ofFloatColor(c1), ofFloatColor(c2), width1, width2, g_options);
    
    auto& renderer = getRenderer();
    renderer.begin();
    renderer.draw(seg);
    renderer.end();
}

void setJointStyle(JointStyle style) {
    g_options.joint = style;
}

void setCapStyle(CapStyle style) {
    g_options.cap = style;
}

void setFeather(bool enabled, float amount) {
    g_options.feather = enabled;
    g_options.feathering = amount;
}

Options& getOptions() {
    return g_options;
}

} // namespace ofxVase
