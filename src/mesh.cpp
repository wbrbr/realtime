#include "mesh.hpp"
#include "GL/gl3w.h"
#include "glm/gtc/type_ptr.hpp"
#include <iostream>

glm::mat4 aiMatrixToGlm(const aiMatrix4x4* from)
{
    glm::mat4 to;


    to[0][0] = (GLfloat)from->a1; to[0][1] = (GLfloat)from->b1;  to[0][2] = (GLfloat)from->c1; to[0][3] = (GLfloat)from->d1;
    to[1][0] = (GLfloat)from->a2; to[1][1] = (GLfloat)from->b2;  to[1][2] = (GLfloat)from->c2; to[1][3] = (GLfloat)from->d2;
    to[2][0] = (GLfloat)from->a3; to[2][1] = (GLfloat)from->b3;  to[2][2] = (GLfloat)from->c3; to[2][3] = (GLfloat)from->d3;
    to[3][0] = (GLfloat)from->a4; to[3][1] = (GLfloat)from->b4;  to[3][2] = (GLfloat)from->c4; to[3][3] = (GLfloat)from->d4;

    return to;
}

AnimatedMesh::AnimatedMesh()
{
    m_nbBones = 0;
}

AnimatedMesh::AnimatedMesh(std::string path)
{
    m_nbBones = 0;
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_FlipUVs);
    if (!scene) {
        std::cerr << importer.GetErrorString() << std::endl;
        return;
    }
    aiMesh* m = scene->mMeshes[0];
    unsigned int vao, vbo;
    Mesh mesh;
    mesh.numVertices = m->mNumVertices;
    mesh.vao = vao;
    mesh.vbo = vbo;

    m_vertexboneids = (int*)malloc(4 * sizeof(int) * mesh.numVertices);
    m_vertexboneweights = (float*)malloc(4 * sizeof(float) * mesh.numVertices);

    for (unsigned int i = 0; i < 4 * m->mNumVertices; i++)
    {
        m_vertexboneids[i] = -1;
    }

    for (unsigned int i = 0; i < m->mNumBones; i++)
    {
        Bone bone;
        bone.offsetMatrix = aiMatrixToGlm(&m->mBones[i]->mOffsetMatrix);
        addBone(bone);
        m_bonemapping[std::string(m->mBones[i]->mName.C_Str())] = m_nbBones-1;
        for (unsigned int j = 0; j < m->mBones[i]->mNumWeights; j++)
        {
            aiVertexWeight w = m->mBones[i]->mWeights[j];
            addBoneToVertex((int)w.mVertexId, m_nbBones-1, w.mWeight);
        }
    }

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, m->mNumVertices * 14 * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m->mNumVertices * 3 * sizeof(float), m->mVertices);
    glBufferSubData(GL_ARRAY_BUFFER, m->mNumVertices * 3 * sizeof(float), m->mNumVertices * 3 * sizeof(float), m->mNormals);
    if (m->HasTextureCoords(0)) {
        float* texcoords = static_cast<float*>(malloc(m->mNumVertices * 2 * sizeof(float)));
        for (unsigned int i = 0; i < m->mNumVertices; i++)
        {
            auto vec = m->mTextureCoords[0][i];
            texcoords[2*i] = vec.x;
            texcoords[2*i+1] = vec.y;
        }
        glBufferSubData(GL_ARRAY_BUFFER, m->mNumVertices * 6 * sizeof(float), m->mNumVertices * 2 * sizeof(float), texcoords);
        free(texcoords);
    } else {
        float* zero = static_cast<float*>(malloc(m->mNumVertices * 2 * sizeof(float)));
        for (unsigned int i = 0; i < m->mNumVertices * 2; i++)
        {
            zero[i] = 0.f;
        }
        glBufferSubData(GL_ARRAY_BUFFER, m->mNumVertices * 6 * sizeof(float), m->mNumVertices * 2 * sizeof(float), zero);
        free(zero);
    }
    glBufferSubData(GL_ARRAY_BUFFER, m->mNumVertices * 8 * sizeof(float), m->mNumVertices * 3 * sizeof(float), m->mTangents);
    glBufferSubData(GL_ARRAY_BUFFER, m->mNumVertices * 11 * sizeof(float), m->mNumVertices * 3 * sizeof(float), m->mBitangents);
    glBufferSubData(GL_ARRAY_BUFFER, m->mNumVertices * 14 * sizeof(float), m->mNumVertices * 4 * sizeof(float), m_vertexboneweights);
    glBufferSubData(GL_ARRAY_BUFFER, m->mNumVertices * 18 * sizeof(float), m->mNumVertices * 4 * sizeof(int), m_vertexboneids);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(m->mNumVertices * 3 * sizeof(float)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(m->mNumVertices * 6 * sizeof(float)));
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(m->mNumVertices * 8 * sizeof(float)));
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(m->mNumVertices * 11 * sizeof(float)));
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(m->mNumVertices * 14 * sizeof(float)));
    glVertexAttribIPointer(6, 4, GL_INT, 0, (GLvoid*)(m->mNumVertices * 18 * sizeof(float)));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glEnableVertexAttribArray(5);
    glEnableVertexAttribArray(6);

    m_mesh.vao = vao;
    m_mesh.vbo = vbo;
    m_mesh.numVertices = m->mNumVertices;
}

void AnimatedMesh::calcBoneTransform(aiNode* node, glm::mat4 parentTransform)
{
    std::string name(node->mName.C_Str()); 

    if (m_bonemapping.find(name) != m_bonemapping.end()) {
        int id = m_bonemapping[name];
        m_bones[id].localMatrix = aiMatrixToGlm(&node->mTransformation);
        glm::mat4 trans = parentTransform * m_bones[id].localMatrix;
        m_bones[id].worldMatrix = trans * m_bones[id].offsetMatrix;

        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            calcBoneTransform(node->mChildren[i], trans);
        }
    } else {
        glm::mat4 trans = parentTransform * aiMatrixToGlm(&node->mTransformation);
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            calcBoneTransform(node->mChildren[i], trans);
        }
    }
}

void AnimatedMesh::initBoneTransforms(aiScene* scene)
{
    calcBoneTransform(scene->mRootNode, glm::mat4());
}

void AnimatedMesh::addBoneToVertex(int vertexid, int boneid, float w)
{
    assert(vertexid < m_mesh.numVertices);
    assert(boneid < MAX_BONES);
    for (int i = 0; i < 4; i++)
    {
        if (m_vertexboneids[4*vertexid+i] == -1) {
            m_vertexboneids[4*vertexid+i] = boneid;
            m_vertexboneweights[4*vertexid+i] = w;
        }
    }

    assert(0);
}

void AnimatedMesh::addBone(Bone b)
{
    assert(m_nbBones < MAX_BONES);
    m_bones[m_nbBones] = b;
    m_nbBones++;
}

Mesh AnimatedMesh::getMesh()
{
    return m_mesh;
}

void AnimatedMesh::setUniformBones(Shader& shader, std::string uniformName)
{
    for (int i = 0; i < m_nbBones; i++)
    {
        int loc = shader.getLoc(uniformName + "[" + std::to_string(i) + "]");
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(m_bones[i].worldMatrix));
    }
}
