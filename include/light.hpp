#ifndef LIGHT_HPP
#define LIGHT_HPP

struct PointLight
{
    Transform transform;
    float intensity;
};

struct DirectionalLight
{
    glm::vec3 direction;
    float intensity;
};
#endif
