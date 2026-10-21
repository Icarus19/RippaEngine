#version 450 core

layout(location = 0) out vec4 color;

in vec3 Color;
in vec2 TexCoord;
in vec3 TexCoords3D;
in vec3 Normal;
in vec3 FragPosition;

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform vec3 viewPosition;
uniform float zoom;

struct Material {
    sampler2D diffuse;
    vec3 specular;
    sampler2D specularMap;
    float shininess;
};
uniform Material material;

struct Light {
    vec3 position;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float intensity;
    float cutoff;
    float outerCutoff;
    float constant;
    float linear;
    float quadratic;
};
uniform Light light;

void main()
{
    //vec2 flippedTexCoord = vec2(-TexCoord.x, TexCoord.y);

	//float colorV = texture(texture0, TexCoord).r;
    //color = vec4(colorV, colorV, colorV, 1.0);
	//color = mix(texture(texture0, TexCoord), texture(texture1, flippedTexCoord), zoom);
	//color = vec4(Color, 1.0);

    color = texture(material.diffuse, TexCoord);
	vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPosition);

    // ambient
    vec3 ambient = light.ambient * color.rgb;

    // Spotlight
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutoff - light.outerCutoff;
    float spotIntensity = clamp((theta - light.outerCutoff) / epsilon, 0.0f, 1.0f);

    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    // diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    diffuse = light.diffuse * diff * color.rgb;

    // specular
    vec3 viewDir = normalize(viewPosition - FragPosition);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    specular = light.specular * spec * material.specular;//vec3(texture(material.specularMap, TexCoord));

    // attenuation
    float distance = length(light.position - FragPosition);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    attenuation *= light.intensity;
    diffuse *= attenuation;
    specular *= attenuation;

    // spotintensity
    diffuse *= spotIntensity;
    specular *= spotIntensity;

    color = vec4(ambient + diffuse + specular, 1.0);
    
    
	//color = vec4(TexCoord.x, TexCoord.y, 0.0, 1.0);
	//color = vec4(TexCoords3D, 1.0);
}