#version 330 core

float near =  0.1;
float far  =  100.0;

struct Material{
	sampler2D   texture_diffuse1;
	sampler2D   texture_specular1;
	float		shininess;
};
uniform Material material;

//DIRECTIONAL LIGHT
struct DirLight{
	vec3  direction;
	
	vec3  ambient;
	vec3  diffuse;
	vec3  specular;
};
uniform DirLight dirLight;

//POINT LIGHT
struct PointLight{
	vec3  position;
		  
	vec3  ambient;
	vec3  diffuse;
	vec3  specular;
		  
	float constant;
	float linear;
	float quadratic;
};
#define NR_POINT_LIGHTS 4
uniform PointLight pointLights[NR_POINT_LIGHTS];

//SPOT LIGHT

struct SpotLight{
	vec3  position;
	vec3  direction;

	float cutOff;	  
	float outerCutOff;	  	  

	vec3  ambient;
	vec3  diffuse;
	vec3  specular;

	float constant;
	float linear;
	float quadratic;
};		  
uniform SpotLight spotLight;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight spotLight, vec3 normal, vec3 fragPos, vec3 viewDir);



in      vec3 FragPos;
in      vec3 Normal;
in      vec2 TexCoords;
out     vec4 FragColor;
uniform vec3 viewPos;

//********************************************************************************************************************

void main() 
{
	//properties
	vec3 norm    =  normalize(Normal);
	vec3 viewDir =  normalize(viewPos - FragPos);

	//phase 1 -> directional lighting
	vec3 result = CalcDirLight(dirLight, norm, viewDir);

	//phase 2 -> point lights
	for(int i = 0; i < NR_POINT_LIGHTS; i++)
	{
		result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
	}

	//phase 3 -> spotlight
	result += CalcSpotLight(spotLight, norm, FragPos, viewDir);


	//LABS


	FragColor = texture(material.texture_diffuse1, TexCoords);
	//FragColor = vec4(result,1.0);

}

//********************************************************************************************************************
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir){
	vec3 lightDir = normalize(-light.direction);
	//diffuse shading
	float diff = max(dot(normal,lightDir), 0.0);
	// specular shading
	vec3 reflectDir = reflect(-lightDir, normal); 
	float spec = pow(max(dot(viewDir,reflectDir),0.0),material.shininess);
	

	//combine results
	vec3 ambient  =  light.ambient  *		 vec3(texture(material.texture_diffuse1, TexCoords));
	vec3 diffuse  =  light.diffuse  * diff * vec3(texture(material.texture_diffuse1,TexCoords));
	vec3 specular =  light.specular * spec * vec3(texture(material.texture_specular1, TexCoords));

	return (ambient + diffuse + specular);
}


vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir){
	vec3 lightDir = normalize(light.position - fragPos);
	//diffuse shading
	float diff = max(dot(normal,lightDir), 0.0);

	//specular shading
	vec3 reflectDir = reflect(-lightDir, normal); 
	float spec = pow(max(dot(viewDir,reflectDir),0.0), material.shininess);

	//attenuation
	float distance = length(light.position - fragPos);
	float attenuation = 1.0/ (light.constant + light.linear * distance + light.quadratic * (distance * distance));

	//combine results
	vec3 ambient =  light.ambient  *		vec3(texture(material.texture_diffuse1, TexCoords));
	vec3 diffuse =  light.diffuse  * diff * vec3(texture(material.texture_diffuse1,TexCoords));
	vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords));

	return (ambient + diffuse + specular) * attenuation;
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir){
	vec3 lightDir = normalize(light.position - fragPos);
	
	//diffuse shading
	float diff = max(dot(normal,lightDir),0.0);

	//specular shading
	vec3 reflectDir = reflect(-lightDir,normal);
	float spec = pow(max(dot(reflectDir,viewDir),0.0),material.shininess);

	//attenuation
	float distance = length(light.position - fragPos);
	float attenuation = 1.0/(light.constant + light.linear * distance + light.quadratic * (distance * distance));

	float theta = dot(lightDir, normalize(-light.direction));
	float epsilon = light.cutOff - light.outerCutOff;
	float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

	//combine results
	vec3 ambient =  light.ambient  *		vec3(texture(material.texture_diffuse1, TexCoords));
	vec3 diffuse =  light.diffuse  * diff * vec3(texture(material.texture_diffuse1,TexCoords));
	vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords));


	return ambient + (diffuse + specular) * attenuation * intensity;

}


