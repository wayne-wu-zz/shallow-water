// Compute heightfields out of fluxes
uniform float time;
uniform vec2 resolution;
uniform vec2 screenRes;
uniform sampler2D sampler;
uniform int mouseHit;
uniform vec2 mousePos;
uniform vec3 eyeCoordinate;

#define TIMESTEP 0.5
#define H 1.0/resolution.x
#define HITRADIUS 0.1
#define HITDEPTH -0.09

vec3 getCartesian(vec3 sphericalCoord)
{
    float radius = sphericalCoord.x;
    float phi = sphericalCoord.y;
    float theta = sphericalCoord.z;
    return vec3(
        radius * sin(phi) * cos(theta),
        radius * cos(phi),
        radius * sin(phi) * sin(theta)
    );
}

vec3 getRightVector(vec3 coord)
{
    return vec3(sin(coord.z), 0.0, -cos(coord.z));
}

vec3 getMouseHitLocation()
{
    vec3 eye = getCartesian(eyeCoordinate);
    vec3 focus = vec3(0, 0, 0);
    vec3 forward = normalize(focus - eye);
    vec3 right = normalize(getRightVector(eyeCoordinate));
    vec3 up = normalize(cross(right, forward));

    float f = 2.0;
    float u = (mousePos.x * 2.0) / screenRes.x - 1.0;
    float v = ((screenRes.y-mousePos.y) * 2.0) / screenRes.y - 1.0;

    float ar = screenRes.x / screenRes.y;
    right *= ar;

    vec3 mouseP = eye + right * u + up * v + forward * f;
    vec3 dir = normalize(mouseP - eye);
    float t = -mouseP.y/dir.y;
    return mouseP + t*dir;
}

bool withinRadius(vec2 uv)
{
    vec3 mouse = getMouseHitLocation();
    if (max(abs(mouse.x), abs(mouse.z)) > 1.0)
        return false; //out of bound
    vec2 mouseUV = (mouse.xz + vec2(1.0)) * 0.5;
    return length(uv - mouseUV) < HITRADIUS;
}

void main()
{
    vec2 uv = gl_FragCoord.xy / resolution.xy;
    
    vec2 pos = uv * 2.0 - vec2(1.0);
    if (mouseHit == 1 && withinRadius(uv)) 
    {
        //float t = pos.x*pos.x + pos.y*pos.y;
        //float h = -HITDEPTH*cos(t);
        gl_FragColor = vec4(0.0, 0.0, HITDEPTH, 0.0);
        return;
    }

    vec2 eps = vec2(H, 0.0);
    float du = texture2D(sampler, uv + eps.xy).x - texture2D(sampler, uv - eps.xy).x;
    float dw = texture2D(sampler, uv + eps.yx).y - texture2D(sampler, uv - eps.yx).y;

    float N = texture2D(sampler, uv + eps.yx).z;
    float S = texture2D(sampler, uv - eps.yx).z;
    float E = texture2D(sampler, uv + eps.xy).z;
    float W = texture2D(sampler, uv - eps.xy).z;

    // Do some averaging to eliminate noise
    float avg = (N + S + E + W) * 0.25;
    float blend = 0.5;
    float h = blend * avg +  (1.0 - blend) * texture2D(sampler, uv).z;
    h -= TIMESTEP * (du + dw);

    vec2 new_speed = texture2D(sampler, uv).xy;
    gl_FragColor = vec4(new_speed, 0.99*h, 0.0);
}