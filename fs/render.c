#define EPS 0.001
#define STEPSIZE 0.001
#define MAXSTEPS 1000
#define FREQ 20.0
#define AMP 0.005
#define OVERSTEP 4.0
#define AIR_IOR 1.0
#define WATER_IOR 1.33
#define TANK_HEIGHT 0.5
#define BBOX_HEIGHT 0.1
#define FLOOR_WIDTH 2.0

uniform float time;
uniform vec2 resolution;
uniform sampler2D heightSampler;
uniform sampler2D wallSampler;
uniform sampler2D skySampler;
uniform sampler2D causticsSampler;
uniform vec3 eyeCoordinate;

uniform vec3 light1;
uniform vec3 light2;
uniform vec3 light3;

float distBox(vec3 p, vec3 b)
{
    vec3 d = abs(p) - b;
    return min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));
}

float distBox(vec3 p)
{
    return distBox(p, vec3(1.0, TANK_HEIGHT, 1.0));
}

float distWaveBbox(vec3 p)
{
    return distBox(p, vec3(1.0, BBOX_HEIGHT, 1.0));
}

float sinusoid(float x, float z)
{
    float t = x * x * sin(time) + z * z * sin(time);
    return AMP*(sin(FREQ*t) - cos((FREQ/2.0)*t));
}

float height(float x, float z)
{
    vec2 uv = (vec2(x, z) + vec2(1.0)) * 0.5;
    return texture2D(heightSampler, uv).z;
}

float height(vec3 p)
{
    return height(p.x, p.z);
}

float frac(float x)
{
    return x - floor(x);
}

vec3 getBoxNormal(vec3 p)
{
    vec3 n = vec3((distBox(p+vec3(EPS,0,0))-distBox(p)),
                (distBox(p+vec3(0,EPS,0))-distBox(p)),
                (distBox(p+vec3(0,0,EPS))-distBox(p)));
    return normalize(n);
}

vec3 getSurfaceNormal(vec3 p)
{
    vec3 n = vec3(height(p.x - EPS, p.z) - height(p.x + EPS, p.z),
                2.0 * EPS,
                height(p.x, p.z - EPS) - height(p.x, p.z + EPS));
    return normalize(n);
}

float getCaustics(vec3 p)
{
    vec2 uv = (p.xz + vec2(1.0)) * 0.5;
    vec4 caustics = texture2D(causticsSampler, uv);
    float intensity = caustics.x;
    return pow(intensity, 1.0) * 5.0;
}

vec4 getCheckerBoard(vec3 p)
{
    float numTiles = 40.0;
    float x = floor((p.x + 2.0) * 0.25 * numTiles);
    float y = floor((p.y + 2.0) * 0.25 * numTiles);
    float z = floor((p.z + 2.0) * 0.25 * numTiles);
    bool isSame = (frac(x * 0.5) < EPS) == (frac(z * 0.5) < EPS);
    isSame = (frac(y * 0.5) < EPS) ? isSame : !isSame;
    return isSame ? vec4(0.1) : vec4(0.9);
}

vec4 getCheckerBoardFlat(vec3 p)
{
    float numTiles = 40.0;
    float x = floor((p.x + 2.0) * 0.25 * numTiles);
    float z = floor((p.z + 2.0) * 0.25 * numTiles);
    bool isSame = (frac(x * 0.5) < EPS) == (frac(z * 0.5) < EPS);
    return isSame ? vec4(0.1) : vec4(0.9);
}

vec4 getPic(vec3 p, sampler2D sampler)
{
    vec2 uv = (p.xz + vec2(1.0)) * 0.5;
    return texture2D(sampler, uv);
}

vec3 reflection(vec3 v, vec3 n)
{
    return -v + 2.0 * dot(v, n) * n;
}

float intensity(vec3 eye, vec3 p, vec3 n,
                float kSpec, float specWeight, float diffWeight)
{
    vec3 toLight = normalize(light1 - p);
    vec3 toEye = normalize(eye - p);
    vec3 ref = normalize(reflection(toLight, n));

    float diffuse = 0.0;
    float specular = 0.0;

    // Do diffuse lighting
    diffuse +=  max(0.0, dot(toLight, n));
    // Do specular lighting
    specular += pow(max(dot(ref, toEye), 0.0), kSpec);

    // Recompute for second light
    toLight = normalize(light2 - p);
    ref = normalize(reflection(toLight, n));
    diffuse += max(0.0, dot(toLight, ref));
    specular += pow(max(dot(ref, toEye), 0.0), kSpec);

    // Recompute for third light
    toLight = normalize(light3 - p);
    ref = normalize(reflection(toLight, n));
    diffuse += max(0.0, dot(toLight, ref));
    specular += pow(max(dot(ref, toEye), 0.0), kSpec);

    return specWeight * specular + diffWeight * diffuse;
}

vec3 getRefractedDir(vec3 dir, vec3 n)
{
    float costheta = dot(dir, -n);
    float ratio = AIR_IOR / WATER_IOR;
    float c = ratio * ratio * (1.0 - costheta * costheta);
    return ratio * dir + (ratio * costheta - sqrt(1.0 - c)) * n;
}

vec4 shadeWall(vec3 eye, vec3 p, vec3 normal)
{
    return 0.5 * intensity(eye, p, normal, 0.0, 0.0, 1.0) * getCheckerBoard(p);
}

vec4 shadeFloor(vec3 eye, vec3 p)
{
    return 0.5 * intensity(eye, p, vec3(0.0, 1.0, 0.0), 0.0, 0.0, 1.0) * getCheckerBoardFlat(p);
}

vec4 shadeSky(vec3 p)
{
    return getPic(p, skySampler);
}

vec4 shadeWater(vec3 eye, vec3 p, vec3 normal)
{
    return intensity(eye, p, normal, 50.0, 2.0, 0.0) * vec4(1.0);
}

float getFresnel(vec3 n, vec3 eye, vec3 p)
{
    vec3 toEye = normalize(eye - p);
    return dot(n, toEye);
}

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

bool outOfBox(vec3 p)
{
    return max(abs(p.x), abs(p.z)) > 1.0;
}

bool hitFloor(vec3 p, vec3 dir, inout vec3 floorPos)
{
    float t = (-TANK_HEIGHT - p.y)/dir.y;
    floorPos = p + t*dir;
    return max(abs(floorPos.x), abs(floorPos.z)) < FLOOR_WIDTH;
}

bool hitBox(inout vec3 p, const vec3 dir)
{
    for (int i = 0; i < 50; i++) 
    {
        float d = p.y < height(p) ? distBox(p) : distWaveBbox(p);
        if (d < EPS)
        {
            return true;
        }
        p += dir * d;
    }
    return false;
}

bool hitSurface(const vec3 dir, const vec3 eye, inout vec3 p, inout vec4 surfaceColor, bool invertNormal)
{
    for (int i = 0; i < MAXSTEPS; i++)
    {
        // When we hit very close to the surface
        if (p.y < height(p))
        {
            vec3 n = invertNormal ? -getSurfaceNormal(p) : getSurfaceNormal(p);
            surfaceColor += shadeWater(eye, p, getSurfaceNormal(p));
            return true;
        }

        // Checks if the ray hits the box from the inside
        if (distWaveBbox(p) >= EPS)
        {
            return false;
        }

        p += dir * STEPSIZE;
    }
    return true;
}

void main()
{
    vec3 eye = getCartesian(eyeCoordinate);
    vec3 focus = vec3(0, 0, 0);
    vec3 forward = normalize(focus - eye);
    vec3 right = normalize(getRightVector(eyeCoordinate));
    vec3 up = normalize(cross(right, forward));

    float f = 2.0;
    float u = gl_FragCoord.x * 2.0 / resolution.x - 1.0;
    float v = gl_FragCoord.y * 2.0 / resolution.y - 1.0;

    float ar = resolution.x / resolution.y;
    right *= ar;

    vec3 imagePos = eye + right * u + up * v + forward * f;
    vec3 dir = normalize(imagePos - eye);

    vec4 surfaceColor = vec4(0.0);
    vec4 boxColor = vec4(0.0);
    vec4 background = vec4(0.9);
    vec4 wallColor = background;

    vec3 p = eye;
    vec3 surfaceIntersection;

    vec3 floorPos = vec3(0.0);
    if (!hitFloor(p, dir, floorPos))
    {
        discard;
    }

    if (!hitBox(p, dir))
    {
        gl_FragColor = shadeFloor(eye, floorPos);
        return;
    }
    
    vec3 n = vec3(0.0);
    bool shootingUp = p.y > eye.y; 

    // If the part we hit is below the surface
    if (p.y < height(p))
    {
        n = getBoxNormal(p);
        surfaceColor = shadeWater(eye, p, n);
    }
    // If the part we hit is above the surface
    else
    {
        if (shootingUp)
        {
            gl_FragColor = background;
            return;
        }
            
        if (hitSurface(dir, eye, p, surfaceColor, false))
        {
            n = getSurfaceNormal(p);
        }
        else
        {
            vec3 floorPos = vec3(0.0);
            gl_FragColor = hitFloor(p, dir, floorPos) ? shadeFloor(eye, floorPos) :
                                                        background;
            return;
        }
    }

    // Calculate refraction when hit box or surface
    vec3 air_dir = dir;
    dir = getRefractedDir(dir, n);
    surfaceIntersection = p;

    // Step out of the box and step backwards until we hit it
    p += OVERSTEP * dir;
    hitBox(p, -dir);

    // If point is still above the surface, we constant raymarch to surface
    if (p.y > height(p))
    {
        if (!hitSurface(-dir, eye, p, surfaceColor, false))
        {
            wallColor = shadeWall(eye, p, -getBoxNormal(p));
        }
        else
        {
            wallColor = 0.1 * shadeSky(p);
        }
    }
    else
    {
        //if hit the side of the box
        if (p.y > -TANK_HEIGHT)
        {
            wallColor = shadeWater(eye, p, -getBoxNormal(p));
            vec3 floorPos = vec3(0.0);
            if (hitFloor(p, air_dir, floorPos))
            {
                wallColor += shadeFloor(eye, floorPos);
            }
        }
        //else hit the bottom of the tank
        else
        {
            wallColor = 0.5 * shadeFloor(eye, p) + vec4(getCaustics(p));
        }
    }

    float fresnel = getFresnel(n, eye, surfaceIntersection);
    surfaceColor *= 0.3;
    gl_FragColor = (1.0 - fresnel) * surfaceColor + (fresnel + 0.3) * wallColor;
}