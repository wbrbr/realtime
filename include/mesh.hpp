#ifndef MESH_HPP
#define MESH_HPP
#include <glm/mat4x4.hpp>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <map>
#include <array>
#include "shader.hpp"
#define MAX_BONES 32

struct Mesh
{
    unsigned int vao, vbo, numVertices;
};

struct Bone
{
    glm::mat4 localMatrix, offsetMatrix, worldMatrix;
    std::vector<int> children;
};

template<class T> struct Key
{
    T value;
    double tick;
};

struct BoneAnim
{
    std::vector<Key<glm::vec3>> positions;
    std::vector<Key<glm::quat>> rotations;
    std::vector<Key<glm::vec3>> scalings;
};

struct Animation
{
    std::array<BoneAnim, MAX_BONES> bone_anims;
    double durationInTicks;
    float ticksPerSec;
};

class AnimatedMesh
{
public:
    AnimatedMesh();
    AnimatedMesh(std::string file);
    Mesh getMesh();
    void setUniformBones(Shader& shader, std::string uniformName);
    void update(double time);

private:
    /* std::vector<Bone> m_bones */
    Mesh m_mesh;
    Bone m_bones[MAX_BONES];
    std::map<std::string, int> m_bonemapping;
    int* m_vertexboneids;
    float* m_vertexboneweights;
    int m_nbBones;
    Animation m_animation;

    void initBoneTransforms(const aiScene* scene);
    void calcBoneTransform(aiNode* node, glm::mat4 parentTransform);
    void addBoneToVertex(int vertexid, int boneid, float w);
    void addBone(Bone b);
    void interpolateFrames(int boneid, double tick, glm::vec3* position, glm::quat* rotation, glm::vec3* scale);
    glm::vec3 interpolatePosition(int boneid, double tick);
    glm::quat interpolateRotation(int boneid, double tick);
    glm::vec3 interpolateScale(int boneid, double tick);
    void updateTransforms(int boneid, double tick, const glm::mat4& parentMatrix);
    void printBone(int boneid);
    void printMappings();
};
#endif
