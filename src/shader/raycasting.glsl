-- Vertex

#extension GL_ARB_explicit_attrib_location : enable

layout(location = 0) in vec3 vertex;

out vec2 texCoord;

void main()
{
	texCoord = vec2(((vertex.x + 1.0)/2.0), ((vertex.y + 1.0)/2.0));
	gl_Position = vec4(vertex, 1.0);
}

-- Fragment

uniform sampler2D frontFaces;
uniform sampler2D backFaces;
uniform sampler3D volume;
uniform sampler2D transferFunction;

uniform int renderingMode;
in vec2 texCoord;
out vec4 fragColor;

void main()
{
	vec3 rayStart = texture(frontFaces, texCoord).xyz;
	vec3 rayEnd = texture(backFaces, texCoord).xyz;
	
	vec3 rayDir = rayEnd - rayStart;
	float rayLength = length(rayDir);
	vec3 step = normalize(rayDir) * 0.01;
	vec3 ray = vec3(0, 0, 0);

	switch(renderingMode)
	{
		case 0: //render front faces
		{
			fragColor = vec4(texture(frontFaces, texCoord).rgb, 1);
			break;
		}
		
		case 1: //render back faces
		{
			fragColor = vec4(texture(backFaces, texCoord).rgb, 1);
			break;
		}
		
		case 2: //render volume (MIP)
		{
			vec4 intensity = vec4(0, 0, 0, 0);
			vec4 maxIntensity = vec4(0, 0, 0, 0);
			ray = vec3(0,0,0);
			for(int i=0; i <=2000; i++)
			{
				vec4 intensity = texture(volume, (rayStart + ray));
				maxIntensity = max(maxIntensity, intensity);
				if(length(ray)>=rayLength)
				{
					break;
				}
				ray += step;
			}	
			fragColor = maxIntensity.rrra;

			break;
		}
		case 3: //render volume (Alpha-Compositing)
		{
			vec4 intensity = vec4(0, 0, 0, 0);
			vec4 color = vec4(0, 0, 0, 0);
			vec4 colorC = vec4(0, 0, 0, 0.95);
			for(int i=0; i <=1300; i++)
			{
				intensity = texture(volume, (rayStart + ray));
				color = texture(transferFunction, intensity.rr);
				colorC.rgb = (1 / colorC.a) * (colorC.a * colorC.rgb + (1 - colorC.a) * intensity.r * color.rgb);
				//colorC.a = colorC.a + (1 - colorC.a) * color.a;
				if(length(ray) >= rayLength )
				{	
					break;
				} 
				if(colorC.a > 0.99)
				{
					colorC.a = 1;
					break;
				}
				ray += step;
			}	
			fragColor = colorC;

			break;
		}
	}
}