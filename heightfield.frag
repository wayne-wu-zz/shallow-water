#define EPS 0.00001
#define RADIUS 0.5

uniform float time;
uniform vec2 resolution;

float distSphere(vec3 p){
    return length(p) - RADIUS;
}

vec3 getGradient(vec3 p){
    return vec3((distSphere(p+vec3(EPS,0,0))-distSphere(p)),
                (distSphere(p+vec3(0,EPS,0))-distSphere(p)),
                (distSphere(p+vec3(0,0,EPS))-distSphere(p)));
}

vec4 shade(vec3 p){
    vec3 normal = normalize(getGradient(p));
    vec3 lightPos = vec3(0.0, 2.0, 0.0);
    vec3 lightDir = lightPos - p;
    float lightIntensity = dot(normalize(lightDir), normal);
    if (lightIntensity < 0.0)
        return vec4(0.0);
    return lightIntensity * vec4(1.0);
}

void main( void ) {

    vec3 eye = vec3(0,0,-5);
    vec3 up = vec3(0,1,0);
    vec3 right = vec3(1,0,0);
    
    float f = 2.0;
    float u = gl_FragCoord.x * 2.0 / resolution.x - 1.0;
    float v = gl_FragCoord.y * 2.0 / resolution.y - 1.0;
        
    float ar = resolution.x/resolution.y;
    right *= ar;
    
    vec3 forward = normalize(cross(right, up)); 

    vec3 image_p = right * u + up * v + forward * f;
        
    vec3 dir = normalize(image_p - eye);
    
    vec4 color = vec4(0.0);

    //Ray marching
    float t = 0.0;
    const int maxSteps = 50;
    for (int i = 0; i < maxSteps; ++i) {
        vec3 p = eye + dir * t;
        float d = distSphere(p);
        if(d < 0.0001) 
        {
            color = shade(p);
            break;
        }

        t += d;
    }
        
    gl_FragColor = color;

}