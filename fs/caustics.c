varying vec3 start1;
varying vec3 end1;
varying vec3 start3; 
varying vec3 end3;

float getArea(vec3 pos)
{
    return length(dFdx(pos)) * length(dFdy(pos));
}

void main()
{
    gl_FragColor = vec4(getArea(start3)/getArea(end3), 1.0, 1.0, 1.0);
}