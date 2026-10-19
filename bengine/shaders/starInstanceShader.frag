#version 330 core
out vec4 FragColor;

struct Material{
	sampler2D   texture_diffuse1;
	sampler2D   texture_specular1;
};

uniform Material material;

struct DirLight{
	vec3  direction;
	
	vec3  ambient;
	vec3  diffuse;
	vec3  specular;
};
uniform DirLight dirLight;


in vec3 Normal;
in vec2 TexCoords;
in vec3 FragPos;
uniform vec3 viewPos;


vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);

void main()
{
	//properties
	vec3 norm    =  normalize(Normal);
	vec3 viewDir =  normalize(viewPos - FragPos);

	//phase 1 -> directional lighting
	vec3 result = CalcDirLight(dirLight, norm, viewDir);

	FragColor = vec4(result, 1.0);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
	vec3 lightDir = normalize(-light.direction);
	//diffuse shading
	float diff = max(dot(normal,lightDir), 0.0);
	// specular shading
	vec3 reflectDir = reflect(-lightDir, normal); 
	float spec = pow(max(dot(viewDir,reflectDir),0.0), 32);
	

	//combine results
	vec3 ambient  =  light.ambient;
	vec3 diffuse  =  light.diffuse  * diff;
	vec3 specular =  light.specular * spec * 0.5;

	return (ambient + diffuse + specular);
}