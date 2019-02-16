#ifndef MESH_HPP
#define MESH_HPP
#include <glm/mat4x4.hpp>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <map>
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

class AnimatedMesh
{
public:
    AnimatedMesh();
    AnimatedMesh(std::string file);
    Mesh getMesh();
    void setUniformBones(Shader& shader, std::string uniformName);

private:
    /* std::vector<Bone> m_bones */
    Mesh m_mesh;
    Bone m_bones[MAX_BONES];
    int rootID;
    std::map<std::string, int> m_bonemapping;
    int* m_vertexboneids;
    float* m_vertexboneweights;
    int m_nbBones;

    void initBoneTransforms(aiScene* scene);
    void calcBoneTransform(aiNode* node, glm::mat4 parentTransform);
    void addBoneToVertex(int vertexid, int boneid, float w);
    void addBone(Bone b);
};
#endif
