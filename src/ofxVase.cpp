#include "ofxVase.h"

namespace ofxVase {

// ============================================================================
// Utility Functions
// ============================================================================

namespace util {

float normalize(glm::vec2& v) {
    float len = glm::length(v);
    if (len > 0.0000001f) {
        v /= len;
    }
    return len;
}

void perpen(glm::vec2& v) {
    // Anti-clockwise 90 degrees: (x,y) -> (-y, x)
    float y_value = v.y;
    v.y = v.x;
    v.x = -y_value;
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

bool anchorOutward(glm::vec2& v, const glm::vec2& b, const glm::vec2& c, bool reverse) {
    float determinant = (b.x * v.x - c.x * v.x + b.y * v.y - c.y * v.y);
    if ((determinant > 0) == (!reverse)) {
        return false;
    } else {
        v = -v;
        return true;
    }
}

void followSigns(glm::vec2& v, const glm::vec2& a) {
    if ((v.x > 0) != (a.x > 0)) v.x = -v.x;
    if ((v.y > 0) != (a.y > 0)) v.y = -v.y;
}

void sameSideOfLine(glm::vec2& V, const glm::vec2& ref, const glm::vec2& a, const glm::vec2& b) {
    float sign1 = signedArea(a + ref, a, b);
    float sign2 = signedArea(a + V, a, b);
    if ((sign1 >= 0) != (sign2 >= 0)) {
        V = -V;
    }
}

int intersect(const glm::vec2& p1, const glm::vec2& p2,
              const glm::vec2& p3, const glm::vec2& p4,
              glm::vec2& out, float* pts) {
    double denom = (double)(p4.y - p3.y) * (p2.x - p1.x) - (double)(p4.x - p3.x) * (p2.y - p1.y);
    double numera = (double)(p4.x - p3.x) * (p1.y - p3.y) - (double)(p4.y - p3.y) * (p1.x - p3.x);
    double numerb = (double)(p2.x - p1.x) * (p1.y - p3.y) - (double)(p2.y - p1.y) * (p1.x - p3.x);
    
    const double eps = 0.0000000001;
    if (fabs(numera) < eps && fabs(numerb) < eps && fabs(denom) < eps) {
        out = (p1 + p2) * 0.5f;
        return 2;
    }
    
    if (fabs(denom) < eps) {
        out = glm::vec2(0);
        return 0;
    }
    
    float mua = (float)(numera / denom);
    float mub = (float)(numerb / denom);
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

bool intersecting(const glm::vec2& a, const glm::vec2& b,
                  const glm::vec2& c, const glm::vec2& d) {
    return (signedArea(a, b, c) > 0) != (signedArea(a, b, d) > 0);
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
        int i0 = std::max(0, i - 1);
        int i1 = i;
        int i2 = i + 1;
        int i3 = std::min(n - 1, i + 2);
        
        for (int j = 0; j < subdivisions; j++) {
            float t = (float)j / subdivisions;
            outPoints.push_back(catmullRom(points[i0], points[i1], points[i2], points[i3], t));
            outColors.push_back(colorBetween(colors[i1], colors[i2], t));
            outWidths.push_back(widths[i1] * (1.0f - t) + widths[i2] * t);
        }
    }
    
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
    colors.clear();
    jumping = false;
}

int VertexArrayHolder::push(const glm::vec2& pos, const ofFloatColor& color) {
    int cur = (int)vertices.size();
    vertices.push_back(glm::vec3(pos, 0));
    colors.push_back(color);
    
    if (jumping) {
        jumping = false;
        repeatLastPush();
    }
    return cur;
}

int VertexArrayHolder::pushF(const glm::vec2& pos, const ofFloatColor& color) {
    ofFloatColor c = color;
    c.a = 0;
    return push(pos, c);
}

void VertexArrayHolder::push3(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3,
                               const ofFloatColor& c1, const ofFloatColor& c2, const ofFloatColor& c3) {
    push(p1, c1);
    push(p2, c2);
    push(p3, c3);
}

void VertexArrayHolder::push4(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3, const glm::vec2& p4,
                               const ofFloatColor& c1, const ofFloatColor& c2, const ofFloatColor& c3, const ofFloatColor& c4) {
    push3(p1, p2, p3, c1, c2, c3);
    push3(p3, p2, p4, c3, c2, c4);
}

void VertexArrayHolder::push(const VertexArrayHolder& other) {
    if (glmode == other.glmode) {
        vertices.insert(vertices.end(), other.vertices.begin(), other.vertices.end());
        colors.insert(colors.end(), other.colors.begin(), other.colors.end());
    } else if (glmode == DRAW_TRIANGLES && other.glmode == DRAW_TRIANGLE_STRIP) {
        for (size_t b = 2; b < other.vertices.size(); b++) {
            vertices.push_back(other.vertices[b-2]); colors.push_back(other.colors[b-2]);
            vertices.push_back(other.vertices[b-1]); colors.push_back(other.colors[b-1]);
            vertices.push_back(other.vertices[b]);   colors.push_back(other.colors[b]);
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
    
    if (vertices.empty()) return mesh;
    
    for (size_t i = 0; i < vertices.size(); i++) {
        mesh.addVertex(vertices[i]);
        mesh.addColor(colors[i]);
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
}

// ============================================================================
// Polyline - Core Tessellation (ported from C# reference)
// ============================================================================

void Polyline::determineTr(float w, float& t, float& R, float scale) {
    // Calibrated piecewise lookup table from the VASE reference
    w *= scale;
    float f = w - floorf(w);
    
    if (w >= 0.0f && w < 1.0f) {
        t = 0.05f;
        R = 0.768f;
    } else if (w >= 1.0f && w < 2.0f) {
        t = 0.05f + f * 0.33f;
        R = 0.768f + 0.312f * f;
    } else if (w >= 2.0f && w < 3.0f) {
        t = 0.38f + f * 0.58f;
        R = 1.08f;
    } else if (w >= 3.0f && w < 4.0f) {
        t = 0.96f + f * 0.48f;
        R = 1.08f;
    } else if (w >= 4.0f && w < 5.0f) {
        t = 1.44f + f * 0.46f;
        R = 1.08f;
    } else if (w >= 5.0f && w < 6.0f) {
        t = 1.9f + f * 0.6f;
        R = 1.08f;
    } else if (w >= 6.0f) {
        t = 2.5f + (w - 6.0f) * 0.50f;
        R = 1.08f;
    } else {
        t = 0.05f;
        R = 0.768f;
    }
    
    t /= scale;
    R /= scale;
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
                       float& rr, float& tt, float& dist) {
    float t = 1.0f, r = 0.0f;
    
    determineTr(w, t, r, opt.worldToScreenRatio);
    if (opt.feather && !opt.noFeatherAtCore) {
        r *= opt.feathering;
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

// ============================================================================
// Constructors
// ============================================================================

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
    
    // Use the smart approx/exact switching from the reference
    int A = 0, B = 0;
    bool on = false;
    for (int i = 1; i < length - 1; i++) {
        glm::vec2 V1 = points[i] - points[i-1];
        glm::vec2 V2 = points[i+1] - points[i];
        float len = 0;
        len += util::normalize(V1) * 0.5f;
        len += util::normalize(V2) * 0.5f;
        float costho = V1.x * V2.x + V1.y * V2.y;
        const float m_pi = glm::pi<float>();
        float cos_a = cosf(15 * m_pi / 180);
        float cos_b = cosf(10 * m_pi / 180);
        float cos_c = cosf(25 * m_pi / 180);
        bool approx = false;
        if ((width * localOpt.worldToScreenRatio < 7 && costho > cos_a) ||
            (costho > cos_b) ||
            (len < width && costho > cos_c)) {
            approx = true;
        }
        if (approx && !on) {
            A = i;
            on = true;
            if (A == 1) A = 0;
            if (A > 1) {
                polylineRange(points, colors, widths, localOpt, inopt, B, A, false);
            }
        } else if (!approx && on) {
            B = i;
            on = false;
            polylineRange(points, colors, widths, localOpt, inopt, A, B, true);
        }
    }
    if (on && B < length - 1) {
        B = length - 1;
        polylineRange(points, colors, widths, localOpt, inopt, A, B, true);
    } else if (!on && A < length - 1) {
        A = length - 1;
        polylineRange(points, colors, widths, localOpt, inopt, B, A, false);
    }
    holder = inopt.holder;
}

Polyline::Polyline(const std::vector<glm::vec2>& points,
                   const std::vector<ofFloatColor>& colors,
                   const std::vector<float>& widths,
                   const Options& opt) {
    InternalOpt inopt;
    
    if (opt.smoothing > 0 && points.size() >= 2) {
        std::vector<glm::vec2> smoothPts;
        std::vector<ofFloatColor> smoothColors;
        std::vector<float> smoothWidths;
        
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

// ============================================================================
// Polyline range and routing
// ============================================================================

void Polyline::polylineRange(const std::vector<glm::vec2>& P,
                              const std::vector<ofFloatColor>& C,
                              const std::vector<float>& W,
                              const Options& opt, InternalOpt& inopt,
                              int from, int to, bool approx) {
    InternalOpt localInopt = inopt;
    if (from > 0) from--;
    
    localInopt.joinFirst = (from != 0);
    localInopt.joinLast = (to != static_cast<int>(P.size()) - 1);
    localInopt.noCapFirst = inopt.noCapFirst || localInopt.joinFirst;
    localInopt.noCapLast = inopt.noCapLast || localInopt.joinLast;
    
    if (approx) {
        polylineApprox(P, C, W, opt, localInopt, from, to);
    } else {
        polylineExact(P, C, W, opt, localInopt, from, to);
    }
    
    inopt.holder.push(localInopt.holder);
}

void Polyline::polylineApprox(const std::vector<glm::vec2>& P,
                               const std::vector<ofFloatColor>& C,
                               const std::vector<float>& W,
                               const Options& opt, InternalOpt& inopt,
                               int from, int to) {
    if (to - from + 1 < 2) return;
    bool capFirst = !inopt.noCapFirst;
    bool capLast = !inopt.noCapLast;
    bool joinFirst = inopt.joinFirst;
    bool joinLast = inopt.joinLast;
    
    VertexArrayHolder vcore, vfadeo, vfadei;
    vcore.setGlDrawMode(VertexArrayHolder::DRAW_TRIANGLE_STRIP);
    vfadeo.setGlDrawMode(VertexArrayHolder::DRAW_TRIANGLE_STRIP);
    vfadei.setGlDrawMode(VertexArrayHolder::DRAW_TRIANGLE_STRIP);
    
    auto color = [&](int i) -> ofFloatColor {
        return C[inopt.constColor ? 0 : i];
    };
    auto weight = [&](int i) -> float {
        return W[inopt.constWeight ? 0 : i];
    };
    
    auto polyStep = [&](int i, const glm::vec2& pp, float ww, const ofFloatColor& cc) {
        float t = 0, r = 0;
        determineTr(ww, t, r, opt.worldToScreenRatio);
        if (opt.feather && !opt.noFeatherAtCore) {
            r *= opt.feathering;
        }
        glm::vec2 V = P[i] - P[i-1];
        util::perpen(V);
        util::normalize(V);
        glm::vec2 F = V * r;
        V *= t;
        vcore.push(pp + V, cc);
        vcore.push(pp - V, cc);
        vfadeo.push(pp + V, cc);
        vfadeo.pushF(pp + V + F, cc);
        vfadei.push(pp - V, cc);
        vfadei.pushF(pp - V - F, cc);
    };
    
    for (int i = from + 1; i < to; i++) {
        polyStep(i, P[i], weight(i), color(i));
    }
    
    // Last mid point
    glm::vec2 P_las, P_fir;
    ofFloatColor C_las, C_fir;
    float W_las = 0, W_fir = 0;
    
    // Compute last midpoint
    float tLas = 0.5f;
    P_las = (P[to-1] + P[to]) * tLas;
    C_las = util::colorBetween(color(to-1), color(to), tLas);
    W_las = (weight(to-1) + weight(to)) * tLas;
    polyStep(to, P_las, W_las, C_las);
    
    // First cap
    StAnchor SA;
    float tFir = joinFirst ? 0.5f : 0.0f;
    if (tFir == 0.0f) {
        P_fir = P[from]; C_fir = color(from); W_fir = weight(from);
    } else {
        P_fir = (P[from] + P[from+1]) * tFir;
        C_fir = util::colorBetween(color(from), color(from+1), tFir);
        W_fir = (weight(from) + weight(from+1)) * tFir;
    }
    SA.P[0] = P_fir;
    SA.P[1] = P[from + 1];
    SA.C[0] = C_fir;
    SA.C[1] = color(from + 1);
    SA.W[0] = W_fir;
    SA.W[1] = weight(from + 1);
    segment(SA, opt, capFirst, false, true);
    
    // Last cap
    if (!joinLast) {
        SA.P[0] = P_las;
        SA.P[1] = P[to];
        SA.C[0] = C_las;
        SA.C[1] = color(to);
        SA.W[0] = W_las;
        SA.W[1] = weight(to);
        segment(SA, opt, false, capLast, true);
    }
    
    inopt.holder.push(vcore);
    inopt.holder.push(vfadeo);
    inopt.holder.push(vfadei);
    inopt.holder.push(SA.vah);
}

void Polyline::brushArc(VertexArrayHolder& tris,
                         const glm::vec2& center, const ofFloatColor& col,
                         float t, float r,
                         const glm::vec2& N_start, const glm::vec2& N_end,
                         float wsr) {
    const float pi2 = glm::pi<float>() * 2.0f;
    
    float a_start = atan2f(N_start.y, N_start.x);
    float a_end = atan2f(N_end.y, N_end.x);
    float diff = a_end - a_start;
    if (diff < 0) diff += pi2;
    if (diff > pi2) diff -= pi2;
    if (diff < 0.001f) return;
    
    float dangle = getPljRoundDangle(t, r, wsr);
    int steps = std::max(1, static_cast<int>(diff / dangle));
    
    ofFloatColor fadeCol = col;
    fadeCol.a = 0.0f;
    float R = t + r;
    
    for (int j = 0; j < steps; j++) {
        float a1 = a_start + diff * j / steps;
        float a2 = a_start + diff * (j + 1) / steps;
        
        glm::vec2 d1(cosf(a1), sinf(a1));
        glm::vec2 d2(cosf(a2), sinf(a2));
        
        glm::vec2 p1 = center + t * d1;
        glm::vec2 p2 = center + t * d2;
        
        tris.push3(center, p1, p2, col, col, col);
        
        glm::vec2 f1 = center + R * d1;
        glm::vec2 f2 = center + R * d2;
        tris.push(p1, col);
        tris.push(p2, col);
        tris.pushF(f1, col);
        tris.push(p2, col);
        tris.pushF(f1, col);
        tris.pushF(f2, col);
    }
}

void Polyline::polylineExact(const std::vector<glm::vec2>& P,
                              const std::vector<ofFloatColor>& C,
                              const std::vector<float>& W,
                              const Options& opt, InternalOpt& inopt,
                              int from, int to) {
    bool capFirst = !inopt.noCapFirst;
    bool capLast = !inopt.noCapLast;
    
    auto color = [&](int i) -> ofFloatColor {
        return C[inopt.constColor ? 0 : i];
    };
    auto weight = [&](int i) -> float {
        return W[inopt.constWeight ? 0 : i];
    };
    
    int n = to - from + 1;
    if (n < 2) return;
    
    struct VtxInfo {
        glm::vec2 pos;
        float t, r;
        ofFloatColor col;
    };
    
    std::vector<VtxInfo> V(n);
    for (int i = 0; i < n; i++) {
        int idx = from + i;
        V[i].pos = P[idx];
        V[i].col = color(idx);
        determineTr(weight(idx), V[i].t, V[i].r, opt.worldToScreenRatio);
        if (opt.feather && !opt.noFeatherAtCore)
            V[i].r *= opt.feathering;
    }
    
    struct SegTan {
        glm::vec2 N_top, N_bot;
        bool degenerate = false;
    };
    std::vector<SegTan> seg(n - 1);
    
    for (int i = 0; i < n - 1; i++) {
        glm::vec2 D = V[i+1].pos - V[i].pos;
        float d = glm::length(D);
        
        float R1 = V[i].t + V[i].r;
        float R2 = V[i+1].t + V[i+1].r;
        
        if (d < 0.001f || d < fabsf(R1 - R2)) {
            seg[i].N_top = glm::vec2(0, 1);
            seg[i].N_bot = glm::vec2(0, -1);
            seg[i].degenerate = true;
            continue;
        }
        
        glm::vec2 u = D / d;
        glm::vec2 nn(-u.y, u.x);
        
        float sinA = glm::clamp((R1 - R2) / d, -0.999f, 0.999f);
        float cosA = sqrtf(1.0f - sinA * sinA);
        
        seg[i].N_top = sinA * u + cosA * nn;
        seg[i].N_bot = sinA * u - cosA * nn;
    }
    
    auto lineIsect = [](glm::vec2 p1, glm::vec2 d1,
                        glm::vec2 p2, glm::vec2 d2,
                        glm::vec2& out) -> bool {
        float det = d1.x * d2.y - d1.y * d2.x;
        if (fabsf(det) < 1e-8f) return false;
        glm::vec2 dp = p2 - p1;
        float t = (dp.x * d2.y - dp.y * d2.x) / det;
        out = p1 + t * d1;
        return true;
    };

    struct MiterInfo {
        glm::vec2 core_inner, fade_inner;
        bool valid = false;
        bool top_is_inner = false;
    };
    std::vector<MiterInfo> M(n);

    for (int i = 1; i < n - 1; i++) {
        if (seg[i-1].degenerate || seg[i].degenerate) continue;

        auto& ps = seg[i-1];
        auto& ns = seg[i];

        glm::vec2 d_prev = V[i].pos - V[i-1].pos;
        glm::vec2 d_next = V[i+1].pos - V[i].pos;
        float cross = d_prev.x * d_next.y - d_prev.y * d_next.x;
        M[i].top_is_inner = (cross > 0);

        glm::vec2 N_inner_prev = M[i].top_is_inner ? ps.N_top : ps.N_bot;
        glm::vec2 N_inner_next = M[i].top_is_inner ? ns.N_top : ns.N_bot;

        auto edgeDir = [&](int si, glm::vec2 N) -> glm::vec2 {
            return (V[si+1].pos + V[si+1].t * N) - (V[si].pos + V[si].t * N);
        };
        auto edgeDirR = [&](int si, glm::vec2 N) -> glm::vec2 {
            float R0 = V[si].t + V[si].r;
            float R1 = V[si+1].t + V[si+1].r;
            return (V[si+1].pos + R1 * N) - (V[si].pos + R0 * N);
        };

        float maxLen = 3.0f * (V[i].t + V[i].r);

        glm::vec2 pA = V[i].pos + V[i].t * N_inner_prev;
        glm::vec2 dA = edgeDir(i-1, N_inner_prev);
        glm::vec2 pB = V[i].pos + V[i].t * N_inner_next;
        glm::vec2 dB = edgeDir(i, N_inner_next);
        glm::vec2 isect;
        if (lineIsect(pA, dA, pB, dB, isect)) {
            float dist = glm::length(isect - V[i].pos);
            glm::vec2 avgInner = N_inner_prev + N_inner_next;
            float side = glm::dot(isect - V[i].pos, avgInner);
            if (side > 0 && dist < maxLen) {
                M[i].core_inner = isect;
                M[i].valid = true;

                float R = V[i].t + V[i].r;
                glm::vec2 fpA = V[i].pos + R * N_inner_prev;
                glm::vec2 fdA = edgeDirR(i-1, N_inner_prev);
                glm::vec2 fpB = V[i].pos + R * N_inner_next;
                glm::vec2 fdB = edgeDirR(i, N_inner_next);
                if (!lineIsect(fpA, fdA, fpB, fdB, M[i].fade_inner)) {
                    glm::vec2 dir = glm::normalize(isect - V[i].pos);
                    M[i].fade_inner = V[i].pos + R * dir;
                }
            }
        }
    }

    VertexArrayHolder tris;
    tris.setGlDrawMode(VertexArrayHolder::DRAW_TRIANGLES);

    auto drawDisc = [&](const VtxInfo& v) {
        const float pi2 = glm::pi<float>() * 2.0f;
        float dangle = getPljRoundDangle(v.t, v.r, opt.worldToScreenRatio);
        int steps = std::max(8, static_cast<int>(pi2 / dangle));
        float R = v.t + v.r;
        for (int j = 0; j < steps; j++) {
            float a1 = pi2 * j / steps;
            float a2 = pi2 * (j + 1) / steps;
            glm::vec2 d1(cosf(a1), sinf(a1));
            glm::vec2 d2(cosf(a2), sinf(a2));
            glm::vec2 p1 = v.pos + v.t * d1;
            glm::vec2 p2 = v.pos + v.t * d2;
            tris.push3(v.pos, p1, p2, v.col, v.col, v.col);
            glm::vec2 f1 = v.pos + R * d1;
            glm::vec2 f2 = v.pos + R * d2;
            tris.push(p1, v.col);
            tris.push(p2, v.col);
            tris.pushF(f1, v.col);
            tris.push(p2, v.col);
            tris.pushF(f1, v.col);
            tris.pushF(f2, v.col);
        }
    };

    auto drawSegBody = [&](int i) {
        if (seg[i].degenerate) {
            drawDisc(V[i+1]);
            return;
        }
        auto& v1 = V[i];
        auto& v2 = V[i+1];
        auto& st = seg[i];

        glm::vec2 T1t = v1.pos + v1.t * st.N_top;
        glm::vec2 T1b = v1.pos + v1.t * st.N_bot;
        glm::vec2 T2t = v2.pos + v2.t * st.N_top;
        glm::vec2 T2b = v2.pos + v2.t * st.N_bot;

        float R1 = v1.t + v1.r;
        float R2 = v2.t + v2.r;
        glm::vec2 F1t = v1.pos + R1 * st.N_top;
        glm::vec2 F1b = v1.pos + R1 * st.N_bot;
        glm::vec2 F2t = v2.pos + R2 * st.N_top;
        glm::vec2 F2b = v2.pos + R2 * st.N_bot;

        if (i > 0 && M[i].valid) {
            if (M[i].top_is_inner) { T1t = M[i].core_inner; F1t = M[i].fade_inner; }
            else                   { T1b = M[i].core_inner; F1b = M[i].fade_inner; }
        }
        if (i < n-2 && M[i+1].valid) {
            if (M[i+1].top_is_inner) { T2t = M[i+1].core_inner; F2t = M[i+1].fade_inner; }
            else                     { T2b = M[i+1].core_inner; F2b = M[i+1].fade_inner; }
        }

        tris.push3(T1t, T2t, T2b, v1.col, v2.col, v2.col);
        tris.push3(T1t, T2b, T1b, v1.col, v2.col, v1.col);

        tris.push(T1t, v1.col);  tris.push(T2t, v2.col);  tris.pushF(F1t, v1.col);
        tris.push(T2t, v2.col);  tris.pushF(F1t, v1.col); tris.pushF(F2t, v2.col);

        tris.push(T1b, v1.col);  tris.push(T2b, v2.col);  tris.pushF(F1b, v1.col);
        tris.push(T2b, v2.col);  tris.pushF(F1b, v1.col); tris.pushF(F2b, v2.col);
    };

    auto drawJointFill = [&](int i) {
        auto& v = V[i];
        auto& ps = seg[i-1];
        auto& ns = seg[i];

        // Fill both-side gaps (center to tangent points)
        glm::vec2 pt1 = v.pos + v.t * ps.N_top;
        glm::vec2 pt2 = v.pos + v.t * ns.N_top;
        tris.push3(v.pos, pt1, pt2, v.col, v.col, v.col);

        glm::vec2 pb1 = v.pos + v.t * ps.N_bot;
        glm::vec2 pb2 = v.pos + v.t * ns.N_bot;
        tris.push3(v.pos, pb1, pb2, v.col, v.col, v.col);

        // Miter connecting triangle (tangent_prev → miter → tangent_next)
        if (M[i].valid) {
            glm::vec2 ip = M[i].top_is_inner ? (v.pos + v.t * ps.N_top) : (v.pos + v.t * ps.N_bot);
            glm::vec2 in_ = M[i].top_is_inner ? (v.pos + v.t * ns.N_top) : (v.pos + v.t * ns.N_bot);
            tris.push3(ip, M[i].core_inner, in_, v.col, v.col, v.col);
        }
    };

    // Draw in stroke order: segment → joint → disc (disc on top to cover gradient bleed)
    for (int i = 0; i < n; i++) {
        if (i > 0) {
            drawSegBody(i - 1);
        }

        if (i > 0 && i < n - 1 && !seg[i-1].degenerate && !seg[i].degenerate) {
            drawJointFill(i);
        }

        drawDisc(V[i]);
    }

    inopt.holder.push(tris);
}

// ============================================================================
// Segment
// ============================================================================

void Polyline::segment(StAnchor& SA, const Options& opt,
                        bool capFirst, bool capLast, bool core) {
    float* weight = SA.W;
    glm::vec2 P[2] = { SA.P[0], SA.P[1] };
    ofFloatColor C[3] = { SA.C[0], SA.C[1] };
    
    glm::vec2 T2, R2, bR;
    float t = 0, r = 0;
    
    bool varying_weight = weight[0] != weight[1];
    
    glm::vec2 capStart(0), capEnd(0);
    StPolyline SL[2];
    
    {
        float dd = 0;
        makeTrc(P[0], P[1], T2, R2, bR, weight[0], opt, r, t, dd);
        
        if (capFirst) {
            if (opt.cap == CapStyle::Square) {
                P[0] -= bR * (t + r);
            }
            capStart = bR;
            util::opposite(capStart);
            if (opt.feather && !opt.noFeatherAtCap) {
                capStart *= opt.feathering;
            }
        }
        
        SL[0].djoint = static_cast<char>(opt.cap);
        SL[0].t = t;
        SL[0].r = r;
        SL[0].T = T2;
        SL[0].R = R2;
        SL[0].bR = bR * 0.01f;
        SL[0].degenT = false;
    }
    
    {
        float dd = 0;
        if (varying_weight) {
            makeTrc(P[0], P[1], T2, R2, bR, weight[1], opt, r, t, dd);
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
        SL[1].bR = bR * 0.01f;
        SL[1].degenT = false;
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
    if (SL[0].djoint == static_cast<char>(CapStyle::Butt) ||
        SL[0].djoint == static_cast<char>(CapStyle::Square))
        P_0 -= cap1;
    if (SL[1].djoint == static_cast<char>(CapStyle::Butt) ||
        SL[1].djoint == static_cast<char>(CapStyle::Square))
        P_1 -= cap2;
    
    // Core edge points
    glm::vec2 P1 = P_0 + SL[0].T;
    glm::vec2 P2 = P_0 - SL[0].T;
    glm::vec2 P3 = P_1 + SL[1].T;
    glm::vec2 P4 = P_1 - SL[1].T;
    
    // Fade edge points (core + R)
    glm::vec2 P1r = P1 + SL[0].R;
    glm::vec2 P2r = P2 - SL[0].R;
    glm::vec2 P3r = P3 + SL[1].R;
    glm::vec2 P4r = P4 - SL[1].R;
    
    // Cap extension points
    glm::vec2 P1c = P1r + cap1;
    glm::vec2 P2c = P2r + cap1;
    glm::vec2 P3c = P3r + cap2;
    glm::vec2 P4c = P4r + cap2;
    
    if (core) {
        // Core quad (opaque)
        tris.push3(P1, P2, P3, C[0], C[0], C[1]);
        tris.push3(P2, P3, P4, C[0], C[1], C[1]);
        
        // Top fade strip (P1r/P3r transparent)
        tris.push(P1, C[0]);
        tris.pushF(P1r, C[0]);
        tris.push(P3, C[1]);
        tris.pushF(P1r, C[0]);
        tris.push(P3, C[1]);
        tris.pushF(P3r, C[1]);
        
        // Bottom fade strip (P2r/P4r transparent)
        tris.push(P2, C[0]);
        tris.pushF(P2r, C[0]);
        tris.push(P4, C[1]);
        tris.pushF(P2r, C[0]);
        tris.push(P4, C[1]);
        tris.pushF(P4r, C[1]);
    }
    
    // Caps
    for (int j = 0; j < 2; j++) {
        glm::vec2 cur_cap = j == 0 ? cap1 : cap2;
        if (glm::length(cur_cap) < 0.001f) continue;
        
        if (SL[j].djoint == static_cast<char>(CapStyle::Round)) {
            VertexArrayHolder cap;
            cap.setGlDrawMode(VertexArrayHolder::DRAW_TRIANGLE_STRIP);
            glm::vec2 O = P[j];
            float dangle = getPljRoundDangle(SL[j].t, SL[j].r, opt.worldToScreenRatio);
            
            glm::vec2 bRcap = SL[j].bR;
            util::followSigns(bRcap, j == 0 ? cap1 : cap2);
            
            glm::vec2 app_P = O + SL[j].T;
            
            // Core arc
            vectorsToArc(cap, O, C[j], C[j],
                         SL[j].T + bRcap, -SL[j].T + bRcap,
                         dangle, SL[j].t, 0.0f, false, app_P,
                         j == 0 ? cap1 : cap2, 0, false);
            cap.push(O - SL[j].T, C[j]);
            cap.push(app_P, C[j]);
            
            cap.jump();
            
            // Fade arc
            ofFloatColor C2 = C[j]; C2.a = 0.0f;
            glm::vec2 a1 = O + SL[j].T;
            glm::vec2 a2 = O + SL[j].T * (1.0f / SL[j].t) * (SL[j].t + SL[j].r);
            glm::vec2 b1 = O - SL[j].T;
            glm::vec2 b2 = O - SL[j].T * (1.0f / SL[j].t) * (SL[j].t + SL[j].r);
            
            cap.push(a1, C[j]);
            cap.push(a2, C2);
            vectorsToArc(cap, O, C[j], C2,
                         SL[j].T + bRcap, -SL[j].T + bRcap,
                         dangle, SL[j].t, SL[j].t + SL[j].r, false, O,
                         j == 0 ? cap1 : cap2, 0, false);
            cap.push(b1, C[j]);
            cap.push(b2, C2);
            
            tris.push(cap);
        } else if (SL[j].djoint == static_cast<char>(CapStyle::Rect) ||
                   SL[j].djoint == static_cast<char>(CapStyle::Square)) {
            VertexArrayHolder cap;
            cap.setGlDrawMode(VertexArrayHolder::DRAW_TRIANGLE_STRIP);
            
            glm::vec2 Pj, Pjr, Pjc, Pk, Pkr, Pkc;
            if (j == 0) {
                Pj = P1; Pjr = P1r; Pjc = P1c;
                Pk = P2; Pkr = P2r; Pkc = P2c;
            } else {
                Pj = P3; Pjr = P3r; Pjc = P3c;
                Pk = P4; Pkr = P4r; Pkc = P4c;
            }
            
            cap.pushF(Pkr, C[j]);
            cap.pushF(Pkc, C[j]);
            cap.push(Pk, C[j]);
            cap.pushF(Pjc, C[j]);
            cap.push(Pj, C[j]);
            cap.pushF(Pjr, C[j]);
            
            tris.push(cap);
        }
    }
}

// ============================================================================
// Anchor - Joint handling (ported from C# reference)
// ============================================================================

void Polyline::anchor(StAnchor& SA, const Options& opt, bool capFirst, bool capLast) {
    glm::vec2* P = SA.P;
    ofFloatColor* C = SA.C;
    float* weight = SA.W;
    StPolyline* SL = SA.SL;
    
    for (int i = 0; i < 3; i++) SL[i] = StPolyline{};
    SA.vah.setGlDrawMode(VertexArrayHolder::DRAW_TRIANGLES);
    SA.capStart = glm::vec2(0);
    SA.capEnd = glm::vec2(0);
    
    const float cos_cri_angle = 0.979386f;
    bool varying_weight = !(weight[0] == weight[1] && weight[1] == weight[2]);
    
    float combined_weight = weight[1] + (opt.feather ? opt.feathering : 0.0f);
    if (combined_weight < 1.6f) {
        segment(SA, opt, capFirst, false, true);
        Options opt2 = opt;
        opt2.cap = (opt.joint == JointStyle::Round) ? CapStyle::Round : CapStyle::Butt;
        SA.P[0] = SA.P[1]; SA.P[1] = SA.P[2];
        SA.C[0] = SA.C[1]; SA.C[1] = SA.C[2];
        SA.W[0] = SA.W[1]; SA.W[1] = SA.W[2];
        segment(SA, opt2, false, capLast, true);
        return;
    }
    
    glm::vec2 T1, T2, T21, T31;
    glm::vec2 R1, R2, R21, R31;
    
    for (int i = 0; i < 3; i++) {
        if (weight[i] >= 0.0f && weight[i] < 1.0f) {
            C[i].a *= weight[i];
        }
    }
    
    // Point 0 (start)
    {
        int i = 0;
        glm::vec2 cap1, cap0;
        float r = 0, t = 0, d = 0;
        makeTrc(P[i], P[i+1], T2, R2, cap1, weight[i], opt, r, t, d);
        if (varying_weight) {
            makeTrc(P[i], P[i+1], T31, R31, cap0, weight[i+1], opt, d, d, d);
        } else {
            T31 = T2;
            R31 = R2;
        }
        util::anchorOutward(R2, P[i+1], P[i+2]);
        util::followSigns(T2, R2);
        
        SL[i].bR = cap1;
        
        if (capFirst) {
            if (opt.cap == CapStyle::Square) {
                P[0] -= cap1 * (t + r);
            }
            util::opposite(cap1);
            if (opt.feather && !opt.noFeatherAtCap)
                cap1 *= opt.feathering;
            SA.capStart = cap1;
        }
        
        SL[i].djoint = static_cast<char>(opt.cap);
        SL[i].T = T2;
        SL[i].R = R2;
        SL[i].t = t;
        SL[i].r = r;
        SL[i].degenT = false;
        SL[i].degenR = false;
        
        SL[i+1].T1 = T31;
        SL[i+1].R1 = R31;
    }
    
    // Cap last point
    if (capLast) {
        int i = 2;
        glm::vec2 cap2, dummyT, dummyR;
        float t = 0, r = 0, d = 0;
        makeTrc(P[i-1], P[i], dummyT, dummyR, cap2, weight[i], opt, r, t, d);
        if (opt.cap == CapStyle::Square) {
            P[2] += cap2 * (t + r);
        }
        SL[i].bR = cap2;
        if (opt.feather && !opt.noFeatherAtCap)
            cap2 *= opt.feathering;
        SA.capEnd = cap2;
    }
    
    // Point 1 (joint)
    {
        int i = 1;
        float r = 0, t = 0;
        glm::vec2 P_cur = P[i];
        glm::vec2 P_nxt = P[i+1];
        glm::vec2 P_las = P[i-1];
        if (opt.cap == CapStyle::Butt || opt.cap == CapStyle::Square) {
            P_nxt -= SA.capEnd;
            P_las -= SA.capStart;
        }
        
        {
            glm::vec2 bR, cap0;
            float length_cur = 0, length_nxt = 0, d = 0;
            makeTrc(P_las, P_cur, T1, R1, cap0, weight[i-1], opt, d, d, length_cur);
            if (varying_weight) {
                makeTrc(P_las, P_cur, T21, R21, cap0, weight[i], opt, d, d, d);
            } else {
                T21 = T1;
                R21 = R1;
            }
            
            makeTrc(P_cur, P_nxt, T2, R2, bR, weight[i], opt, r, t, length_nxt);
            if (varying_weight) {
                makeTrc(P_cur, P_nxt, T31, R31, cap0, weight[i+1], opt, d, d, d);
            } else {
                T31 = T2;
                R31 = R2;
            }
            
            SL[i].T = T2;
            SL[i].R = R2;
            SL[i].bR = bR;
            SL[i].t = t;
            SL[i].r = r;
            SL[i].degenT = false;
            SL[i].degenR = false;
            
            SL[i+1].T1 = T31;
            SL[i+1].R1 = R31;
        }
        
        // Find angle between segments
        glm::vec2 ln1 = P_cur - P_las;
        glm::vec2 ln2 = P_nxt - P_cur;
        util::normalize(ln1);
        util::normalize(ln2);
        glm::vec2 V;
        util::dot(ln1, ln2, V);
        float cos_tho = -V.x - V.y;
        bool zero_degree = fabs(cos_tho - 1.0f) < 0.0000001f;
        bool d180_degree = cos_tho < -1.0f + 0.0001f;
        int result3 = 1;
        
        if ((cos_tho < 0 && opt.joint == JointStyle::Bevel) ||
            (opt.joint != JointStyle::Bevel && opt.cap == CapStyle::Round) ||
            (opt.joint == JointStyle::Round)) {
            SL[i-1].bR *= 0.01f;
            SL[i].bR *= 0.01f;
            SL[i+1].bR *= 0.01f;
        }
        
        // Orient vectors outward
        util::anchorOutward(T1, P_cur, P_nxt);
        util::followSigns(R1, T1);
        util::anchorOutward(T21, P_cur, P_nxt);
        util::followSigns(R21, T21);
        util::followSigns(SL[i].T1, T21);
        util::followSigns(SL[i].R1, T21);
        util::anchorOutward(T2, P_cur, P_las);
        util::followSigns(R2, T2);
        util::followSigns(SL[i].T, T2);
        util::followSigns(SL[i].R, T2);
        util::anchorOutward(T31, P_cur, P_las);
        util::followSigns(R31, T31);
        
        // Find intersection (miter point)
        {
            glm::vec2 interP, vP;
            result3 = util::intersect(P_las + T1, P_cur + T21,
                                      P_nxt + T31, P_cur + T2,
                                      interP);
            if (result3 != 0) {
                vP = interP - P_cur;
                SL[i].vP = vP;
                SL[i].vR = vP * (r / std::max(t, 0.001f));
            } else {
                SL[i].vP = SL[i].T;
                SL[i].vR = SL[i].R;
            }
        }
        
        // Invert vectors for inner edge tests
        glm::vec2 T1i = -T1, R1i = -R1;
        glm::vec2 T21i = -T21, R21i = -R21;
        glm::vec2 T2i = -T2, R2i = -R2;
        glm::vec2 T31i = -T31, R31i = -R31;
        
        // Fade degeneration
        glm::vec2 PR1, PR2, PT1, PT2;
        float pt1 = 0, pt2 = 0;
        int result1r = util::intersect(P_nxt - T31 - R31, P_nxt + T31 + R31,
                                       P_las + T1i + R1i, P_cur + T21i + R21i, PR1);
        int result2r = util::intersect(P_las - T1 - R1, P_las + T1 + R1,
                                       P_nxt + T31i + R31i, P_cur + T2i + R2i, PR2);
        bool is_result1r = result1r == 1;
        bool is_result2r = result2r == 1;
        bool inner_sec = util::intersecting(P_las + T1i + R1i, P_cur + T21i + R21i,
                                            P_nxt + T31i + R31i, P_cur + T2i + R2i);
        
        {
            float pts[2] = {0};
            int result1t = util::intersect(P_nxt - T31, P_nxt + T31,
                                           P_las + T1i, P_cur + T21i, PT1, pts);
            pt1 = pts[1];
            int result2t = util::intersect(P_las - T1, P_las + T1,
                                           P_nxt + T31i, P_cur + T2i, PT2, pts);
            pt2 = pts[1];
            bool is_result1t = result1t == 1;
            bool is_result2t = result2t == 1;
            
            if (zero_degree) {
                segment(SA, opt, capFirst, capLast, true);
                return;
            }
            
            if ((is_result1r || is_result2r) && !inner_sec) {
                SL[i].degenR = true;
                SL[i].PT = is_result1r ? PT1 : PT2;
                SL[i].PR = is_result1r ? PR1 : PR2;
                SL[i].pt = is_result1r ? pt1 : pt2;
                if (SL[i].pt < 0) SL[i].pt = 0.0001f;
                SL[i].preFull = is_result1r;
                SL[i].R_full_degen = false;
                
                glm::vec2 P_nxt2 = P[i+1];
                glm::vec2 P_las2 = P[i-1];
                if (opt.cap == CapStyle::Rect || opt.cap == CapStyle::Round) {
                    P_nxt2 += SA.capEnd;
                    P_las2 += SA.capStart;
                }
                glm::vec2 PR;
                int result2;
                if (is_result1r) {
                    result2 = util::intersect(P_nxt2 - T31i - R31i, P_nxt2 + T31i,
                                              P_las2 + T1i + R1i, P_cur + T21i + R21i, PR);
                } else {
                    result2 = util::intersect(P_las2 - T1i - R1i, P_las2 + T1i,
                                              P_nxt2 + T31i + R31i, P_cur + T2i + R2i, PR);
                }
                if (result2 == 1) {
                    SL[i].R_full_degen = true;
                    SL[i].PR = PR;
                }
            }
            
            if (is_result1t || is_result2t) {
                SL[i].degenT = true;
                SL[i].preFull = is_result1t;
                SL[i].PT = is_result1t ? PT1 : PT2;
                SL[i].pt = is_result1t ? pt1 : pt2;
            }
        }
        
        SL[i].djoint = static_cast<char>(opt.joint);
        if (opt.joint == JointStyle::Miter) {
            if (cos_tho >= cos_cri_angle) {
                SL[i].djoint = static_cast<char>(JointStyle::Bevel);
            }
        }
        
        if (d180_degree || result3 == 0) {
            util::sameSideOfLine(SL[i].R, SL[i-1].R, P_cur, P_las);
            util::followSigns(SL[i].T, SL[i].R);
            SL[i].vP = SL[i].T;
            util::followSigns(SL[i].T1, SL[i].T);
            util::followSigns(SL[i].R1, SL[i].T);
            SL[i].vR = SL[i].R;
            SL[i].djoint = static_cast<char>(JointStyle::Miter);
        }
    }
    
    // Point 2 (end)
    {
        int i = 2;
        glm::vec2 cap0;
        float r = 0, t = 0, d = 0;
        makeTrc(P[i-1], P[i], T2, R2, cap0, weight[i], opt, r, t, d);
        util::sameSideOfLine(R2, SL[i-1].R, P[i-1], P[i]);
        util::followSigns(T2, R2);
        
        SL[i].djoint = static_cast<char>(opt.cap);
        SL[i].T = T2;
        SL[i].R = R2;
        SL[i].t = t;
        SL[i].r = r;
        SL[i].degenT = false;
        SL[i].degenR = false;
    }
    
    if (capFirst || capLast) {
        anchorCap(opt, SA.P, SA.C, SA.SL, SA.vah, SA.capStart, SA.capEnd);
    }
    anchorLate(opt, P, C, SL, SA.vah, SA.capStart, SA.capEnd);
}

void Polyline::anchorCap(const Options& opt,
                         glm::vec2* P, ofFloatColor* C, StPolyline* SL,
                         VertexArrayHolder& tris,
                         const glm::vec2& cap1, const glm::vec2& cap2) {
    for (int i = 0, k = 0; k <= 1; i = 2, k++) {
        glm::vec2 cur_cap = (i == 0) ? cap1 : cap2;
        if (glm::length(cur_cap) < 0.001f) continue;
        
        VertexArrayHolder cap;
        cap.setGlDrawMode(VertexArrayHolder::DRAW_TRIANGLES);
        
        if (SL[i].djoint == static_cast<char>(CapStyle::Round)) {
            VertexArrayHolder strip;
            strip.setGlDrawMode(VertexArrayHolder::DRAW_TRIANGLE_STRIP);
            
            ofFloatColor C2 = C[i]; C2.a = 0.0f;
            glm::vec2 O = P[i];
            glm::vec2 app_P = O + SL[i].T;
            glm::vec2 bRcap = SL[i].bR;
            util::followSigns(bRcap, cur_cap);
            float dangle = getPljRoundDangle(SL[i].t, SL[i].r, opt.worldToScreenRatio);
            
            // Core arc
            vectorsToArc(strip, O, C[i], C[i],
                         SL[i].T + bRcap, -SL[i].T + bRcap,
                         dangle, SL[i].t, 0.0f, false, app_P,
                         glm::vec2(0), 0, false);
            strip.push(O - SL[i].T, C[i]);
            strip.push(app_P, C[i]);
            
            strip.jump();
            
            // Fade arc
            glm::vec2 a1 = O + SL[i].T;
            glm::vec2 a2 = O + SL[i].T * (1.0f / SL[i].t) * (SL[i].t + SL[i].r);
            glm::vec2 b1 = O - SL[i].T;
            glm::vec2 b2 = O - SL[i].T * (1.0f / SL[i].t) * (SL[i].t + SL[i].r);
            
            strip.push(a1, C[i]);
            strip.push(a2, C2);
            vectorsToArc(strip, O, C[i], C2,
                         SL[i].T + bRcap, -SL[i].T + bRcap,
                         dangle, SL[i].t, SL[i].t + SL[i].r, false, O,
                         glm::vec2(0), 0, false);
            strip.push(b1, C[i]);
            strip.push(b2, C2);
            
            cap.push(strip);
        } else {
            glm::vec2 P_cur = P[i];
            glm::vec2 P0 = P_cur + SL[i].T + SL[i].R;
            glm::vec2 P1l = P0 + cur_cap;
            glm::vec2 P2l = P_cur + SL[i].T;
            glm::vec2 P4l = P_cur - SL[i].T;
            glm::vec2 P3l = P4l - SL[i].R + cur_cap;
            glm::vec2 P5l = P4l - SL[i].R;
            
            cap.pushF(P0, C[i]);
            cap.pushF(P1l, C[i]);
            cap.push(P2l, C[i]);
            
            cap.pushF(P1l, C[i]);
            cap.push(P2l, C[i]);
            cap.pushF(P3l, C[i]);
            
            cap.push(P2l, C[i]);
            cap.pushF(P3l, C[i]);
            cap.push(P4l, C[i]);
            
            cap.pushF(P3l, C[i]);
            cap.push(P4l, C[i]);
            cap.pushF(P5l, C[i]);
        }
        tris.push(cap);
    }
}

void Polyline::anchorLate(const Options& opt,
                          glm::vec2* P, ofFloatColor* C, StPolyline* SL,
                          VertexArrayHolder& tris,
                          const glm::vec2& cap1, const glm::vec2& cap2) {
    glm::vec2 P_0 = P[0], P_1 = P[1], P_2 = P[2];
    if (SL[0].djoint == static_cast<char>(CapStyle::Butt) ||
        SL[0].djoint == static_cast<char>(CapStyle::Square))
        P_0 -= cap1;
    if (SL[2].djoint == static_cast<char>(CapStyle::Butt) ||
        SL[2].djoint == static_cast<char>(CapStyle::Square))
        P_2 -= cap2;
    
    // Core points (at T distance)
    glm::vec2 P0 = P_1 + SL[1].vP;
    glm::vec2 P1 = P_1 - SL[1].vP;
    glm::vec2 P2 = P_1 + SL[1].T1;
    glm::vec2 P3 = P_0 + SL[0].T;
    glm::vec2 P4 = P_0 - SL[0].T;
    glm::vec2 P5 = P_1 + SL[1].T;
    glm::vec2 P6 = P_2 + SL[2].T;
    glm::vec2 P7 = P_2 - SL[2].T;
    
    // Fade points (at T+R distance)
    glm::vec2 P0r = P0 + SL[1].vR;
    glm::vec2 P1r = P1 - SL[1].vR;
    glm::vec2 P2r = P2 + SL[1].R1 + SL[0].bR;
    glm::vec2 P3r = P3 + SL[0].R;
    glm::vec2 P4r = P4 - SL[0].R;
    glm::vec2 P5r = P5 + SL[1].R - SL[1].bR;
    glm::vec2 P6r = P6 + SL[2].R;
    glm::vec2 P7r = P7 - SL[2].R;
    
    // Compute Cpt for degeneration cases
    ofFloatColor Cpt = C[1];
    if (SL[1].degenT || SL[1].degenR) {
        float pt = sqrtf(std::max(SL[1].pt, 0.0f));
        if (SL[1].preFull)
            Cpt = util::colorBetween(C[0], C[1], pt);
        else
            Cpt = util::colorBetween(C[1], C[2], 1.0f - pt);
    }
    
    // Core body
    if (SL[1].degenT) {
        P1 = SL[1].PT;
        if (SL[1].degenR)
            P1r = SL[1].PR;
        
        tris.push3(P3, P2, P1, C[0], C[1], C[1]);
        tris.push3(P1, P5, P6, C[1], C[1], C[2]);
        if (SL[1].preFull) {
            tris.push3(P1, P3, P4, C[1], C[0], C[0]);
        } else {
            tris.push3(P1, P6, P7, C[1], C[2], C[2]);
        }
    } else if (SL[1].degenR && SL[1].pt > 0.0001f) {
        glm::vec2 P9 = SL[1].PT;
        if (SL[1].preFull) {
            tris.push3(P1, P5, P6, C[1], C[1], C[2]);
            tris.push3(P1, P6, P7, C[1], C[2], C[2]);
            tris.push3(P3, P2, P1, C[0], C[1], C[1]);
            tris.push3(P3, P9, P1, C[0], Cpt, C[1]);
            tris.push3(P3, P9, P4, C[0], Cpt, C[0]);
        } else {
            tris.push3(P3, P2, P1, C[0], C[1], C[1]);
            tris.push3(P1, P3, P4, C[1], C[0], C[0]);
            tris.push3(P5, P1, P6, C[1], C[1], C[2]);
            tris.push3(P1, P6, P9, C[1], C[2], Cpt);
            tris.push3(P7, P9, P6, C[2], Cpt, C[2]);
        }
    } else {
        tris.push3(P3, P2, P1, C[0], C[1], C[1]);
        tris.push3(P1, P3, P4, C[1], C[0], C[0]);
        tris.push3(P1, P5, P6, C[1], C[1], C[2]);
        tris.push3(P1, P6, P7, C[1], C[2], C[2]);
    }
    
    // Joint fill (core)
    switch (SL[1].djoint) {
        case static_cast<char>(JointStyle::Miter):
            tris.push3(P2, P5, P0, C[1], C[1], C[1]);
            // fall through to bevel
        case static_cast<char>(JointStyle::Bevel):
            tris.push3(P2, P5, P1, C[1], C[1], C[1]);
            break;
        case static_cast<char>(JointStyle::Round): {
            VertexArrayHolder strip;
            strip.setGlDrawMode(VertexArrayHolder::DRAW_TRIANGLE_STRIP);
            vectorsToArc(strip, P_1, C[1], C[1],
                         SL[1].T1, SL[1].T,
                         getPljRoundDangle(SL[1].t, SL[1].r, opt.worldToScreenRatio),
                         SL[1].t, 0.0f, false, P1,
                         glm::vec2(0), 0, true);
            tris.push(strip);
        } break;
    }
    
    // Inner fade
    if (SL[1].degenR) {
        glm::vec2 P9 = SL[1].PT;
        glm::vec2 P9r = SL[1].PR;
        ofFloatColor ccpt = SL[1].degenT ? C[1] : Cpt;
        
        if (SL[1].preFull) {
            // Quad: P9, P4, P9r, P4r (triangle strip order)
            tris.push(P9, ccpt);
            tris.push(P4, C[0]);
            tris.pushF(P9r, C[1]);
            tris.push(P4, C[0]);
            tris.pushF(P9r, C[1]);
            tris.pushF(P4r, C[0]);
            
            if (!SL[1].degenT) {
                glm::vec2 mid = (P9 + P7) * 0.5f;
                tris.push(P1, C[1]);
                tris.push(P9, Cpt);
                tris.pushF(mid, C[1]);
                tris.push(P1, C[1]);
                tris.push(P7, C[2]);
                tris.pushF(mid, C[1]);
            }
        } else {
            // Quad: P9, P7, P9r, P7r (triangle strip order)
            tris.push(P9, ccpt);
            tris.push(P7, C[2]);
            tris.pushF(P9r, C[1]);
            tris.push(P7, C[2]);
            tris.pushF(P9r, C[1]);
            tris.pushF(P7r, C[2]);
            
            if (!SL[1].degenT) {
                glm::vec2 mid = (P9 + P4) * 0.5f;
                tris.push(P1, C[1]);
                tris.push(P9, Cpt);
                tris.pushF(mid, C[1]);
                tris.push(P1, C[1]);
                tris.push(P4, C[0]);
                tris.pushF(mid, C[1]);
            }
        }
    } else {
        // Normal inner fade (both sides)
        tris.push(P1, C[1]);
        tris.push(P4, C[0]);
        tris.pushF(P1r, C[1]);
        tris.push(P4, C[0]);
        tris.pushF(P1r, C[1]);
        tris.pushF(P4r, C[0]);
        tris.push(P1, C[1]);
        tris.push(P7, C[2]);
        tris.pushF(P1r, C[1]);
        tris.push(P7, C[2]);
        tris.pushF(P1r, C[1]);
        tris.pushF(P7r, C[2]);
    }
    
    { // Outer fade (always drawn)
        tris.push(P2, C[1]);
        tris.push(P3, C[0]);
        tris.pushF(P2r, C[1]);
        tris.push(P3, C[0]);
        tris.pushF(P2r, C[1]);
        tris.pushF(P3r, C[0]);
        tris.push(P5, C[1]);
        tris.push(P6, C[2]);
        tris.pushF(P5r, C[1]);
        tris.push(P6, C[2]);
        tris.pushF(P5r, C[1]);
        tris.pushF(P6r, C[2]);
        
        // Joint fade
        switch (SL[1].djoint) {
            case static_cast<char>(JointStyle::Miter):
                tris.push(P0, C[1]);
                tris.push(P5, C[1]);
                tris.pushF(P0r, C[1]);
                tris.push(P5, C[1]);
                tris.pushF(P0r, C[1]);
                tris.pushF(P5r, C[1]);
                tris.push(P0, C[1]);
                tris.push(P2, C[1]);
                tris.pushF(P0r, C[1]);
                tris.push(P2, C[1]);
                tris.pushF(P0r, C[1]);
                tris.pushF(P2r, C[1]);
                break;
            case static_cast<char>(JointStyle::Bevel):
                tris.push(P2, C[1]);
                tris.push(P5, C[1]);
                tris.pushF(P2r, C[1]);
                tris.push(P5, C[1]);
                tris.pushF(P2r, C[1]);
                tris.pushF(P5r, C[1]);
                break;
            case static_cast<char>(JointStyle::Round): {
                VertexArrayHolder strip;
                strip.setGlDrawMode(VertexArrayHolder::DRAW_TRIANGLE_STRIP);
                ofFloatColor C2 = C[1]; C2.a = 0.0f;
                vectorsToArc(strip, P_1, C[1], C2,
                             SL[1].T1, SL[1].T,
                             getPljRoundDangle(SL[1].t, SL[1].r, opt.worldToScreenRatio),
                             SL[1].t, SL[1].t + SL[1].r, false, P_1,
                             glm::vec2(0), 0, false);
                tris.push(strip);
            } break;
        }
    }
}

// ============================================================================
// VectorsToArc (ported from C# reference)
// ============================================================================

void Polyline::vectorsToArc(VertexArrayHolder& hold,
                            const glm::vec2& P, const ofFloatColor& C, const ofFloatColor& C2,
                            const glm::vec2& PA, const glm::vec2& PB,
                            float dangle, float r, float r2,
                            bool ignorEnds, const glm::vec2& apparentP,
                            const glm::vec2& hint, float /*rr*/, bool /*innerFade*/) {
    // Generates a triangle strip arc from PA to PB around center P.
    // C = color for arc edge points, C2 = color for inner/center point.
    // For anti-aliased edges: pass C with alpha=0 (transparent outer edge)
    //   and C2 with full alpha (opaque center). The strip interpolates between them.
    // For opaque fills: pass both C and C2 with full alpha.
    
    const float m_pi = glm::pi<float>();
    if (r < 0.0001f) return;
    
    glm::vec2 A = PA * (1.0f / r);
    glm::vec2 B = PB * (1.0f / r);
    
    A.x = glm::clamp(A.x, -0.999999f, 0.999999f);
    B.x = glm::clamp(B.x, -0.999999f, 0.999999f);
    
    float angle1 = acosf(A.x);
    float angle2 = acosf(B.x);
    if (A.y > 0) angle1 = 2 * m_pi - angle1;
    if (B.y > 0) angle2 = 2 * m_pi - angle2;
    
    if (glm::length(hint) > 0.001f) {
        float nudge = 0.00001f;
        if ((hint.x > 0) || (hint.x == 0 && hint.y > 0)) {
            angle1 -= (angle1 < angle2 ? 1 : -1) * nudge;
        } else {
            angle1 += (angle1 < angle2 ? 1 : -1) * nudge;
        }
    }
    
    if (angle2 > angle1) {
        if (angle2 - angle1 > m_pi) angle2 -= 2 * m_pi;
    } else {
        if (angle1 - angle2 > m_pi) angle1 -= 2 * m_pi;
    }
    
    bool incremental = angle1 <= angle2;
    
    // For the second radius (r2): if > 0, inner points are on a smaller circle
    // instead of at apparentP. This is used for fade strips between two arcs.
    bool useInnerCircle = (r2 > 0.001f);
    
    auto pushArcPair = [&](float x, float y, bool reverse) {
        glm::vec2 outerPt(P.x + x * r, P.y - y * r);
        glm::vec2 innerPt = useInnerCircle
            ? glm::vec2(P.x + x * r2, P.y - y * r2)
            : apparentP;
        if (!reverse) {
            hold.push(outerPt, C);
            hold.push(innerPt, C2);
        } else {
            hold.push(innerPt, C2);
            hold.push(outerPt, C);
        }
    };
    
    if (incremental) {
        if (!ignorEnds) {
            glm::vec2 startOuter(P.x + PB.x, P.y + PB.y);
            glm::vec2 startInner = useInnerCircle
                ? glm::vec2(P.x + B.x * r2, P.y + B.y * r2) : apparentP;
            hold.push(startOuter, C);
            hold.push(startInner, C2);
        }
        int safety = 0;
        for (float a = angle2 - dangle; a > angle1 && safety < 200; a -= dangle, safety++) {
            pushArcPair(cosf(a), sinf(a), false);
        }
        if (!ignorEnds) {
            glm::vec2 endOuter(P.x + PA.x, P.y + PA.y);
            glm::vec2 endInner = useInnerCircle
                ? glm::vec2(P.x + A.x * r2, P.y + A.y * r2) : apparentP;
            hold.push(endOuter, C);
            hold.push(endInner, C2);
        }
    } else {
        if (!ignorEnds) {
            glm::vec2 startOuter(P.x + PB.x, P.y + PB.y);
            glm::vec2 startInner = useInnerCircle
                ? glm::vec2(P.x + B.x * r2, P.y + B.y * r2) : apparentP;
            hold.push(startInner, C2);
            hold.push(startOuter, C);
        }
        int safety = 0;
        for (float a = angle2 + dangle; a < angle1 && safety < 200; a += dangle, safety++) {
            pushArcPair(cosf(a), sinf(a), true);
        }
        if (!ignorEnds) {
            glm::vec2 endOuter(P.x + PA.x, P.y + PA.y);
            glm::vec2 endInner = useInnerCircle
                ? glm::vec2(P.x + A.x * r2, P.y + A.y * r2) : apparentP;
            hold.push(endInner, C2);
            hold.push(endOuter, C);
        }
    }
}

// ============================================================================
// Segment class
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
// Renderer with shader support
// ============================================================================

Renderer::Renderer() {}
Renderer::~Renderer() {}

void Renderer::setup() {
    if (initialized) return;
    initialized = true;
    ofLogNotice("ofxVase") << "Renderer initialized (vertex-alpha anti-aliasing)";
}

void Renderer::begin() {
    if (!initialized) setup();
    ofEnableAlphaBlending();
}

void Renderer::end() {
}

void Renderer::draw(const VertexArrayHolder& holder) {
    if (holder.vertices.empty()) return;
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
