// ofxVase Fragment Shader - Anti-aliased line rendering
// Based on VASEr C# implementation

#version 150

uniform sampler2D tex0;

in vec4 vColor;
in vec2 vTexCoord;    // Position in stroke
in vec2 vTexCoord2;   // Fade control (rr values)

out vec4 fragColor;

void main() {
    fragColor = vColor;
    
    // Compute fade factors based on UV and rr values
    // rr.x controls X-axis fade, rr.y controls Y-axis fade
    // When rr > 0, we fade from center (uv=0) to edge (uv=1)
    
    float factx = 1.0;
    float facty = 1.0;
    
    // Only apply fade if rr value is positive
    if (vTexCoord2.x > 0.0) {
        factx = min((1.0 - abs(vTexCoord.x)) * vTexCoord2.x, 1.0);
    }
    if (vTexCoord2.y > 0.0) {
        facty = min((1.0 - abs(vTexCoord.y)) * vTexCoord2.y, 1.0);
    }
    
    // Apply minimum of both fade factors
    fragColor.a *= min(factx, facty);
    
    // Discard fully transparent pixels
    if (fragColor.a < 0.001) {
        discard;
    }
}
