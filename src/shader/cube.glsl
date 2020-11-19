-- Vertex

#extension GL_ARB_explicit_attrib_location : enable

uniform mat4 mvMatrix;
uniform mat4 projMatrix;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec3 texCoord;

out vec3 texCoordVar;


void main()
{
    mat4 rotation = mat4(-1, 0, 0, 0,
						  0, 0, -1, 0,
						  0, -1, 0, 0,
						  0, 0, 0, 1);
	texCoordVar = texCoord;
	gl_Position = projMatrix * mvMatrix * rotation * vec4(vertex, 1.0);
}

-- Fragment

in vec3 texCoordVar;

out vec4 fragColor;

void main()
{
   fragColor = vec4(texCoordVar, 1.0);
}