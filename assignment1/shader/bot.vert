#version 330 core

// Input with skinning attributes
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexUV;
layout(location = 3) in vec4 jointIndices;    // Which joints influence this vertex (up to 4)
layout(location = 4) in vec4 jointWeights;    // Weight for each joint

// Output data, to be interpolated for each fragment
out vec3 worldPosition;
out vec3 worldNormal;
out vec2 uv;

// Uniforms
uniform mat4 MVP;
uniform mat4 jointMatrices[27];  // Array of joint matrices for skinning

void main() {
    int jointIndex = int(jointIndices.x);
    float weight0 = jointWeights.x;
    mat4 skinMatrix = jointMatrices[jointIndex] * weight0;
    vec4 skinnedPosition = skinMatrix * vec4(vertexPosition, 1.0);
    vec4 skinnedNormal = skinMatrix * vec4(vertexNormal, 0.0);
    
    gl_Position = MVP * skinnedPosition;
    
    worldPosition = skinnedPosition.xyz;
    worldNormal = normalize(skinnedNormal.xyz);
    uv = vertexUV;
}