#version 330 core
out vec4 FragColor;

in vec3 FragPos;  
in vec3 Normal;  
in vec2 TexCoords;

uniform sampler2D floorTexture;
uniform vec3 lightPos; 
uniform vec3 viewPos; 
uniform vec3 lightColor;
uniform vec3 objectColor;

vec3 BlinnPhong(vec3 normal, vec3 fragPos, vec3 lightPos, vec3 lightColor)
{
    // diffuse
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    // specular
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;    
    // simple attenuation
    float max_distance = 1.5;
    float distance = length(lightPos - fragPos);
    float attenuation = 1.0 / (distance);
    
    diffuse *= attenuation;
    specular *= attenuation;
    
    return diffuse + specular;
}

void main()
{
    //texture color
    vec3 color = texture(floorTexture, TexCoords).rgb;
    // Check if objectColor has been initialized
    if (objectColor != vec3(0.0)) {
        color = objectColor;
    }
	vec3 lighting = vec3(0.0);
    lighting += BlinnPhong(normalize(Normal), FragPos, lightPos, lightColor);
    color *= lighting;
    color = pow(color, vec3(1.0/2.2));
    FragColor = vec4(color, 1.0);
    
} 