#version 330 core
uniform sampler2D Texture0;
uniform int flip;

in vec2 vTexCoord;

//in float dCo;

in vec3 fragNor;
in vec3 lightDir;
in vec3 EPos;

out vec4 Outcolor;

void main() {

	vec4 texColor0 = texture(Texture0, vTexCoord);
	
	vec3 normal = normalize(fragNor);
	normal = (flip < 1) ? -normal : normal;
	vec3 light = normalize(lightDir);
	
	float dC = max(dot(normal, light), 0.0);		
	Outcolor = dC * texColor0;
}

