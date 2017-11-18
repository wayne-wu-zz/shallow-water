#define EPS 0.01
#define STEPSIZE 0.01
#define MAXSTEPS 1000
#define FREQ 20.0
#define AMP 0.005
#define OVERSTEP 4.0
#define AIR_IOR 1.0
#define WATER_IOR 1.33
#define FLOOR_WIDTH 2.0

attribute vec3 position;

uniform sampler2D heightSampler;
uniform float time;
uniform vec3 light1;
uniform vec3 light2;
uniform vec3 light3;

varying vec3 start1;
varying vec3 end1;
varying vec3 start2;
varying vec3 end2;
varying vec3 start3;
varying vec3 end3;

float distBox(vec3 p)
{
    vec3 b = vec3(1.0, 1.0, 1.0);
    vec3 d = abs(p) - b;
    return min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));
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

vec3 getSurfaceNormal(vec3 p)
{
    vec3 n = vec3(height(p.x - EPS, p.z) - height(p.x + EPS, p.z),
                  2.0 * EPS,
                  height(p.x, p.z - EPS) - height(p.x, p.z + EPS));
    return normalize(n);
}

vec3 getBoxNormal(vec3 p)
{
    vec3 n = vec3((distBox(p+vec3(EPS,0,0))-distBox(p)),
                  (distBox(p+vec3(0,EPS,0))-distBox(p)),
                  (distBox(p+vec3(0,0,EPS))-distBox(p)));
    return normalize(n);
}

vec3 getRefractedDir(vec3 dir, vec3 n)
{
    float costheta = dot(dir, -n);
    float ratio = AIR_IOR / WATER_IOR;
    float c = ratio * ratio * (1.0 - costheta * costheta);
    return ratio * dir + (ratio * costheta - sqrt(1.0 - c)) * n;
}

vec3 getEndPos(vec3 startPos, vec3 lightPos)
{
    // Get the ray direction
    vec3 dir = normalize(startPos - lightPos);
    vec3 p = startPos;

    bool hitSurface = false;
    bool hitBox = false;

    for (int i = 0; i < 50; i++) {
        float d = distBox(p);
        if(d < EPS)
        {
            hitBox = true;
            break;
        }
        p += dir * d;
    }

    if (!hitBox)
    {
        return startPos;
    }
    
    // Approximate the surface point using the plane y = 0
    float t = -p.y/dir.y;
    p = p + t*dir;
    hitSurface = true;

    // Checks if point is outside of surface region
    /*
    if (max(p.x, p.z) > 1.0 || min(p.x, p.z) < -1.0)
    {
        return p;
    }
    */

    // Calculate the intersection with the floor directly
    dir = normalize(getRefractedDir(dir, getSurfaceNormal(p)));
    t = (-1.0-p.y)/dir.y;
    p = p + t*dir;

    return p; 
}

vec3 remapCoordinate(vec3 pos, vec3 lightPos)
{
    vec3 dir = normalize(pos - lightPos);
    // Assuming a plane with equation y = 1.5
    float t = (1.5-lightPos.y)/dir.y;
    return lightPos + t*dir;
}

void main() {
    gl_Position = vec4( position, 1.0 );
    vec3 worldSpacePos = position.xzy;

    start3 = remapCoordinate(worldSpacePos, light3);
    end3 = getEndPos(start3, light3).xzy;
    start3 = start3.xzy;
}