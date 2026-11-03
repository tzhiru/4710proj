#version 330 core 

out vec4 color;

uniform vec3 MatAmb;
uniform vec3 MatDif;
uniform vec3 MatSpec;
uniform float MatShine;

uniform vec3 lightColor;

//interpolated normal and light vector in camera space
in vec3 fragNor;
in vec3 lightDir;

//position of the vertex in camera space
in vec3 EPos;

void main()
{
	vec3 normal = normalize(fragNor);
	vec3 light = normalize(lightDir);
	
	float dC = max(dot(normal, light), 0.0);
	vec3 halfway = normalize(light + normalize(-EPos));
	float spec = pow(max(dot(halfway, normal), 0.0), MatShine);

	color = vec4(lightColor * (MatAmb + (dC * MatDif) + (spec * MatSpec)), 1.0);
}
