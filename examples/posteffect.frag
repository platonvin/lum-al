#version 450

layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput drawnFameToPostProcess;
layout (location = 0) in vec2 outUV;
layout (location = 0) out vec4 outFragColor;

//simple hash from https://www.shadertoy.com/
uint hash21(uvec2 p){
    p *= uvec2(73333,7777);
    p ^= (uvec2(3333777777)>>(p>>28));
    uint n = p.x*p.y;
    return n^(n>>15);
}
float hash(uvec2 p){
    // we only need the top 24 bits to be good really
    uint h = hash21(p);
    // straight to float, see https://iquilezles.org/articles/sfrand/
    // return uintBitsToFloat((h>>9)|0x3f800000u)-1.0;
    return float(h)*(1.0/float(0xffffffffU));
}

float luminance(vec3 v){
    return dot(v, vec3(0.2126f, 0.7152f, 0.0722f));
}
vec3 change_luminance(vec3 c_in, float l_out){
    float l_in = luminance(c_in);
    return c_in * (l_out / l_in);
}

vec3 adjust_brightness(vec3 color, float value) {
    return color + value;
}
vec3 adjust_contrast(vec3 color, float value) {
    // return 0.5 + value * (color - 0.5);
    return 0.5 + (1.0 + value) * (color - 0.5);
}
vec3 adjust_exposure(vec3 color, float value) {
    return (1.0 + value) * color;
}
vec3 adjust_saturation(vec3 color, float value) {
    float grayscale = luminance(color);
    return mix(vec3(grayscale), color, 1.0 + value);
}

void main(){
    vec3 old_color = subpassLoad(drawnFameToPostProcess).xyz;

    float random = hash(uvec2(gl_FragCoord.xy));

    vec3 new_color = old_color;
        new_color = adjust_brightness(new_color, 0.3*random);
        new_color = adjust_saturation(new_color, 0.5*random);

	outFragColor = vec4(new_color,1);
}