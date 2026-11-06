#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
uniform mat4 model;
uniform mat4 projection;
out vec4 vertexColor;
void main()
{
    vec4 worldPosition = model * vec4(aPosition, 1.0);
    // Coordinate locali -> mondo -> clipping ortografico.
    gl_Position = projection * worldPosition;
    vertexColor = aColor;
}
