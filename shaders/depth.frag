#version 330 core

void main()
{
    float zNear = 0.1;
    float zFar = 5.0;
    gl_FragDepth = (2.0 * zNear) / (zFar + zNear - gl_FragCoord.z * (zFar - zNear));
}