#version 330 core 

out vec4 color;

//uniform vec3 MatAmb;
//uniform vec3 MatDif;
//uniform vec3 MatSpec;
//uniform float MatShine;

uniform vec3 lightColor;

//interpolated normal and light vector in camera space
//in vec3 fragNor;
//in vec3 lightDir;

//position of the vertex in camera space
//in vec3 EPos;

void main()
{
	color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
}
