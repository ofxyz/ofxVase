#version 150

in vec4 vColor;
in vec2 vTexCoord;   // UV position in [-1,1] space
in vec3 vNormal;     // xy = fade multipliers (rx, ry)

out vec4 fragColor;

void main() {
    fragColor = vColor;

    // Compute per-pixel fade based on UV position and fade factors.
    // At UV center (0,0): factor = min(1 * rx, 1) = 1 (fully opaque)
    // At UV edge (±1,·): factor = min(0 * rx, 1) = 0 (fully transparent)
    // The rr value (rx/ry) controls the sharpness of the transition:
    //   larger rr = thinner fade band relative to total width
    float factx = min((1.0 - abs(vTexCoord.x)) * vNormal.x, 1.0);
    float facty = min((1.0 - abs(vTexCoord.y)) * vNormal.y, 1.0);

    fragColor.a *= min(factx, facty);

    if (fragColor.a < 0.004) {
        discard;
    }
}
