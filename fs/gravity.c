// Accelerate velocities based on gravity
uniform vec2 resolution;
uniform sampler2D sampler;

#define TIMESTEP 0.5
#define G 1.0
#define H 1.0/resolution.x

void main()
{
    vec2 uv = gl_FragCoord.xy / resolution.xy;
    vec2 eps = vec2(H, 0.0);
    float dhdx = texture2D(sampler, uv + eps.xy).z - texture2D(sampler, uv - eps.xy).z;
    float dhdz = texture2D(sampler, uv + eps.yx).z - texture2D(sampler, uv - eps.yx).z;
    vec2 delh = vec2(dhdx, dhdz);

    vec2 v = texture2D(sampler, uv).xy - TIMESTEP * G * delh;
    float h = texture2D(sampler, uv).z;

    if (gl_FragCoord.x <= 1.5)
    {
        v.x = 0.9 * abs(v.x);
    }
    if (gl_FragCoord.y <= 1.5)
    {
        v.y = 0.9 * abs(v.y);
    }
    if (gl_FragCoord.x >= resolution.x - 1.5)
    {
        v.x = 0.9 * -abs(v.x);
    }
    if (gl_FragCoord.y >= resolution.y - 1.5)
    {
        v.y = 0.9 * -abs(v.y);
    }

    gl_FragColor = vec4(v, h, 0.0);
}