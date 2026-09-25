#version 460

#extension GL_ARB_shader_draw_parameters : enable
#extension GL_KHR_vulkan_glsl : enable
#define INF 1.0 / 0.0

layout( location = 0 ) in vec2 f_uvs;

layout ( set = 0, binding = 1 ) uniform sampler2D i_color;

layout(location = 0) out vec4 out_color;

const float EDGE_TRHESHOLD_MIN = 0.0312;
const float EDGE_THRESHOLD_MAX = 0.063;
const int SUBPIX_SCALE = 2;
const float SUBPIX_TRIM = 0.25;
const float SUBPIX_CAP = 0.75;
const int SEARCH_STEPS = 12;
const float SEARCH_THRESHOLD = 0.25;

float getLuminance(vec3 rgb){
    rgb = pow(rgb, vec3(2.2));
    return dot(rgb, vec3(0.299, 0.587, 0.114));
}

vec3 sampleColor(vec2 offset){
    return texture(i_color, f_uvs + offset).rgb;
}

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(i_color, 0));
    vec3 frag_color = texture(i_color, f_uvs).rgb;
    float lum_m = getLuminance(frag_color);
    vec3 rgb_n = sampleColor(vec2(0, -texelSize.y));
    float lum_n = getLuminance(rgb_n);
    vec3 rgb_s = sampleColor(vec2(0, texelSize.y));
    float lum_s = getLuminance(rgb_s);
    vec3 rgb_e = sampleColor(vec2(texelSize.x, 0));
    float lum_e = getLuminance(rgb_e);
    vec3 rgb_w = sampleColor(vec2(-texelSize.x, 0));
    float lum_w = getLuminance(rgb_w);
    float rangeMin = min(lum_m, min(min(lum_n, lum_s), min(lum_e, lum_w)));
    float rangeMax = max(lum_m, max(max(lum_n, lum_s), max(lum_e, lum_w)));
    float range = rangeMax - rangeMin;
    if(range < max(EDGE_TRHESHOLD_MIN, rangeMax * EDGE_THRESHOLD_MAX)){
        out_color = vec4(frag_color, 1.0);
        return;
    }
    //SUB-PIXEL ALIASING TEST:
    float averageLum = (lum_n + lum_s + lum_e + lum_w) * 0.25f;
    float rangeLum = abs(averageLum - lum_m);
    float blendL = max(0.0, (rangeLum / range) - SUBPIX_TRIM) * SUBPIX_SCALE;
    blendL = min(SUBPIX_CAP, blendL);
    vec3 rgbL = rgb_n + rgb_s + rgb_e + rgb_w + frag_color;
    vec3 rgb_nw = sampleColor(vec2(-texelSize.x, -texelSize.y));
    float lum_nw = getLuminance(rgb_nw);
    vec3 rgb_ne = sampleColor(vec2(texelSize.x, -texelSize.y));
    float lum_ne = getLuminance(rgb_ne);
    vec3 rgb_sw = sampleColor(vec2(-texelSize.x, texelSize.y));
    float lum_sw = getLuminance(rgb_sw);
    vec3 rgb_se = sampleColor(vec2(texelSize.x, texelSize.y));
    float lum_se = getLuminance(rgb_se);
    rgbL += rgb_nw + rgb_ne + rgb_sw + rgb_se;
    rgbL *= (1.0/9.0);
    //VERTICAL/HORIZONTAL EDGE TEST:
    float edgeVert = 
        abs((0.25 * lum_nw) + (-0.5 * lum_n) + (0.25 * lum_ne)) +
        abs((0.50 * lum_w)  + (-1.0 * lum_m) + (0.50 * lum_e))  + 
        abs((0.25 * lum_sw) + (-0.5 * lum_s) + (0.25 * lum_se));
    float edgeHorz = 
        abs((0.25 * lum_nw) + (-0.5 * lum_w) + (0.25 * lum_sw)) +
        abs((0.50 * lum_n)  + (-1.0 * lum_m) + (0.50 * lum_s))  + 
        abs((0.25 * lum_ne) + (-0.5 * lum_e) + (0.25 * lum_se));
    bool horzSpan = edgeHorz >= edgeVert;
    //END OF EDGE SEARCH:
    vec2 searchDir = horzSpan ? vec2(texelSize.x, 0.0) : vec2(0.0, texelSize.y);
    vec2 blendDir  = horzSpan ? vec2(0.0, texelSize.y) : vec2(texelSize.x, 0.0);
    float lumaN1 = horzSpan ? lum_n : lum_e;
    float lumaN2 = horzSpan ? lum_s : lum_w;
    float gradientN1 = abs(lumaN1 - lum_m);
    float gradientN2 = abs(lumaN2 - lum_m);
    float stepSign = gradientN1 >= gradientN2 ? -1.0 : 1.0;
    float lumaN_ref = gradientN1 >= gradientN2 ? lumaN1 : lumaN2;
    float gradientN = max(gradientN1, gradientN2);
    vec2 posN = f_uvs - searchDir;
    vec2 posP = f_uvs + searchDir;
    bool doneN = false;
    bool doneP = false;
    float lumaEndN = lumaN_ref;
    float lumaEndP = lumaN_ref;
    for(uint i = 0u; i < SEARCH_STEPS; i++) {
        if(!doneN) lumaEndN = getLuminance(texture(i_color, posN).rgb);
        if(!doneP) lumaEndP = getLuminance(texture(i_color, posP).rgb);
        doneN = doneN || (abs(lumaEndN - lumaN_ref) >= gradientN * SEARCH_THRESHOLD);
        doneP = doneP || (abs(lumaEndP - lumaN_ref) >= gradientN * SEARCH_THRESHOLD);
        if(doneN && doneP) break;
        if(!doneN) posN -= searchDir;
        if(!doneP) posP += searchDir;
    }
    //DISTANCES & BLEND:
    float distN = length(f_uvs - posN);
    float distP = length(posP - f_uvs);
    float dist   = min(distN, distP);
    float span   = distN + distP;
    float edgeBlend = (0.5 - dist / span) * stepSign;
    float finalBlend = max(blendL, abs(edgeBlend));
    float finalSign = abs(edgeBlend) > blendL ? sign(edgeBlend) : stepSign;
    //FINAL COLOR:
    vec2 uv_final = f_uvs + blendDir * finalBlend * finalSign;
    vec3 rgb_edge = texture(i_color, uv_final).rgb;
    vec3 rgb_final = mix(rgb_edge, rgbL, blendL);
    out_color = vec4(rgb_final, 1.0);
}