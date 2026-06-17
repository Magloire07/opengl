#define GL_GLEXT_PROTOTYPES

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <cstdint>
#include <vector>
#include <array>
#include <iostream>
#include "common/GLShader.h"
#include "DragonData.h"

struct vec3 {
    float x, y, z;
};

inline vec3 add(const vec3& a, const vec3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline vec3 sub(const vec3& a, const vec3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline vec3 mul(const vec3& v, float s) { return { v.x * s, v.y * s, v.z * s }; }
inline float dot(const vec3& a, const vec3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline vec3 cross(const vec3& a, const vec3& b) {
    return {
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
    };
}
inline float length(const vec3& v) { return sqrtf(dot(v, v)); }
inline vec3 normalize(const vec3& v) { float l = length(v); return l > 0.f ? mul(v, 1.f / l) : v; }

struct mat4 {
    float m[16];
    mat4() : m{
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f
    } {}
    mat4(float m00, float m01, float m02, float m03,
         float m10, float m11, float m12, float m13,
         float m20, float m21, float m22, float m23,
         float m30, float m31, float m32, float m33)
    : m{ m00, m01, m02, m03,
         m10, m11, m12, m13,
         m20, m21, m22, m23,
         m30, m31, m32, m33 } {}
};

inline mat4 multiply(const mat4& A, const mat4& B) {
    mat4 C;
    for (int i = 0; i < 16; i++) C.m[i] = 0.f;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            for (int k = 0; k < 4; k++)
                C.m[i*4+j] += A.m[i*4+k] * B.m[k*4+j];
    return C;
}

inline mat4 makeTranslation(float tx, float ty, float tz) {
    return mat4(
        1.f, 0.f, 0.f, tx,
        0.f, 1.f, 0.f, ty,
        0.f, 0.f, 1.f, tz,
        0.f, 0.f, 0.f, 1.f
    );
}

inline mat4 makeScale(float sx, float sy, float sz) {
    return mat4(
        sx, 0.f, 0.f, 0.f,
        0.f, sy, 0.f, 0.f,
        0.f, 0.f, sz, 0.f,
        0.f, 0.f, 0.f, 1.f
    );
}

inline mat4 makeRotationY(float a) {
    float c = cosf(a), s = sinf(a);
    return mat4(
         c, 0.f,  s, 0.f,
        0.f, 1.f, 0.f, 0.f,
        -s, 0.f,  c, 0.f,
        0.f, 0.f, 0.f, 1.f
    );
}

inline mat4 makeRotationX(float a) {
    float c = cosf(a), s = sinf(a);
    return mat4(
        1.f, 0.f, 0.f, 0.f,
        0.f, c, -s, 0.f,
        0.f, s,  c, 0.f,
        0.f, 0.f, 0.f, 1.f
    );
}

inline mat4 makeRotationZ(float a) {
    float c = cosf(a), s = sinf(a);
    return mat4(
         c, -s, 0.f, 0.f,
         s,  c, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f
    );
}

inline mat4 makePerspective(float fovY, float aspect, float nearZ, float farZ) {
    float f = 1.f / tanf(fovY * 0.5f);
    float A = (farZ + nearZ) / (nearZ - farZ);
    float B = 2.f * farZ * nearZ / (nearZ - farZ);
    return mat4(
        f / aspect, 0.f, 0.f, 0.f,
        0.f, f, 0.f, 0.f,
        0.f, 0.f, A, B,
        0.f, 0.f, -1.f, 0.f
    );
}

inline mat4 makeLookAt(const vec3& eye, const vec3& center, const vec3& up) {
    vec3 f = normalize(sub(center, eye));
    vec3 s = normalize(cross(f, up));
    vec3 u = cross(s, f);
    return mat4(
        s.x, s.y, s.z, -dot(s, eye),
        u.x, u.y, u.z, -dot(u, eye),
        -f.x, -f.y, -f.z, dot(f, eye),
        0.f, 0.f, 0.f, 1.f
    );
}

struct SkyVertex { float x, y, z; };
struct ObjectVertex { float x, y, z; float nx, ny, nz; float u, v; };

GLShader g_SkyboxShader;
GLShader g_ObjectShader;

GLuint g_SkyboxVBO = 0;
GLuint g_SkyboxEBO = 0;
GLuint g_SkyboxVAO = 0;
GLuint g_CubemapTexture = 0;

GLuint g_CubeVBO = 0;
GLuint g_CubeVAO = 0;

GLuint g_SphereVBO = 0;
GLuint g_SphereEBO = 0;
GLuint g_SphereVAO = 0;
uint32_t g_SphereIndexCount = 0;

GLuint g_DragonVBO = 0;
GLuint g_DragonEBO = 0;
GLuint g_DragonVAO = 0;
uint32_t g_DragonIndexCount = 0;

GLuint g_DragonTexture = 0;

GLuint g_GroundVBO = 0;
GLuint g_GroundVAO = 0;
uint32_t g_GroundVertexCount = 0;

// Post-processing and FBO Globals
GLuint g_FBO = 0;
GLuint g_FBOTex = 0;
GLuint g_DepthRBO = 0;

GLuint g_ScreenQuadVAO = 0;
GLuint g_ScreenQuadVBO = 0;

GLShader g_ScreenShader;
int g_EffectMode = 0; // 0: Normal, 1: Grayscale, 2: Inversion, 3: Sobel, 4: Chromatic Aberration


int g_Width = 960;
int g_Height = 540;

vec3 g_CameraPosition = { 0.f, 4.5f, 20.f };
float g_CameraYaw = -90.f;
float g_CameraPitch = -15.f;

// mouse control state
double g_LastX = 480.0;
double g_LastY = 270.0;
bool g_FirstMouse = true;
bool g_MousePressed = false;
float g_MouseSensitivity = 0.12f;

static const SkyVertex skyboxVertices[] = {
    { -0.5f, -0.5f, -0.5f },
    {  0.5f, -0.5f, -0.5f },
    {  0.5f,  0.5f, -0.5f },
    { -0.5f,  0.5f, -0.5f },
    { -0.5f, -0.5f,  0.5f },
    {  0.5f, -0.5f,  0.5f },
    {  0.5f,  0.5f,  0.5f },
    { -0.5f,  0.5f,  0.5f }
};

static const uint32_t skyboxIndices[] = {
    4,5,6, 6,7,4,
    1,0,3, 3,2,1,
    0,4,7, 7,3,0,
    5,1,2, 2,6,5,
    0,1,5, 5,4,0,
    7,6,2, 2,3,7
};

static const vec3 cubePositions[8] = {
    { -0.5f, -0.5f, -0.5f }, { 0.5f, -0.5f, -0.5f },
    { 0.5f,  0.5f, -0.5f }, { -0.5f,  0.5f, -0.5f },
    { -0.5f, -0.5f,  0.5f }, { 0.5f, -0.5f,  0.5f },
    { 0.5f,  0.5f,  0.5f }, { -0.5f,  0.5f,  0.5f }
};

static const int cubeFaces[6][4] = {
    {0,1,2,3}, {5,4,7,6}, {4,0,3,7}, {1,5,6,2}, {4,5,1,0}, {3,2,6,7}
};

static const vec3 cubeNormals[6] = {
    {0.f, 0.f, -1.f}, {0.f, 0.f, 1.f}, {-1.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, {0.f, -1.f, 0.f}, {0.f, 1.f, 0.f}
};

GLuint LoadTexture2D(const char* filename)
{
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 4);
    if (!data) {
        std::cerr << "Impossible de charger la texture : " << filename << std::endl;
        return 0;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return texture;
}

GLuint LoadCubemap(const char* faces[6])
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (int i = 0; i < 6; i++) {
        int width, height, nrChannels;
        unsigned char* data = stbi_load(faces[i], &width, &height, &nrChannels, 3);
        if (!data) {
            std::cerr << "Impossible de charger la face cubemap : " << faces[i] << std::endl;
            glDeleteTextures(1, &textureID);
            return 0;
        }
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return textureID;
}

void CreateSkyboxMesh()
{
    glGenVertexArrays(1, &g_SkyboxVAO);
    glGenBuffers(1, &g_SkyboxVBO);
    glGenBuffers(1, &g_SkyboxEBO);

    glBindVertexArray(g_SkyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_SkyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_SkyboxEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), skyboxIndices, GL_STATIC_DRAW);

    GLint posLoc = glGetAttribLocation(g_SkyboxShader.GetProgram(), "a_position");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(SkyVertex), (void*)0);
    glBindVertexArray(0);
}

void CreateCubeMesh()
{
    std::vector<ObjectVertex> vertices;
    vertices.reserve(36);
    for (int face = 0; face < 6; face++) {
        const vec3& normal = cubeNormals[face];
        int i0 = cubeFaces[face][0];
        int i1 = cubeFaces[face][1];
        int i2 = cubeFaces[face][2];
        int i3 = cubeFaces[face][3];

        vertices.push_back({ cubePositions[i0].x, cubePositions[i0].y, cubePositions[i0].z, normal.x, normal.y, normal.z, 0.f, 0.f });
        vertices.push_back({ cubePositions[i1].x, cubePositions[i1].y, cubePositions[i1].z, normal.x, normal.y, normal.z, 1.f, 0.f });
        vertices.push_back({ cubePositions[i2].x, cubePositions[i2].y, cubePositions[i2].z, normal.x, normal.y, normal.z, 1.f, 1.f });

        vertices.push_back({ cubePositions[i0].x, cubePositions[i0].y, cubePositions[i0].z, normal.x, normal.y, normal.z, 0.f, 0.f });
        vertices.push_back({ cubePositions[i2].x, cubePositions[i2].y, cubePositions[i2].z, normal.x, normal.y, normal.z, 1.f, 1.f });
        vertices.push_back({ cubePositions[i3].x, cubePositions[i3].y, cubePositions[i3].z, normal.x, normal.y, normal.z, 0.f, 1.f });
    }

    glGenVertexArrays(1, &g_CubeVAO);
    glGenBuffers(1, &g_CubeVBO);

    glBindVertexArray(g_CubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_CubeVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ObjectVertex), vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ObjectVertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ObjectVertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ObjectVertex), (void*)(6 * sizeof(float)));
    glBindVertexArray(0);
}

void CreateSphereMesh(int rings, int sectors)
{
    std::vector<ObjectVertex> vertices;
    std::vector<uint32_t> indices;
    vertices.reserve((rings + 1) * (sectors + 1));

    for (int r = 0; r <= rings; r++) {
        float theta = (float)r * 3.14159265f / rings;
        float sinTheta = sinf(theta);
        float cosTheta = cosf(theta);

        for (int s = 0; s <= sectors; s++) {
            float phi = (float)s * 2.f * 3.14159265f / sectors;
            float sinPhi = sinf(phi);
            float cosPhi = cosf(phi);
            vec3 normal = { cosPhi * sinTheta, cosTheta, sinPhi * sinTheta };
            vertices.push_back({ normal.x, normal.y, normal.z, normal.x, normal.y, normal.z, (float)s / sectors, (float)r / rings });
        }
    }

    for (int r = 0; r < rings; r++) {
        for (int s = 0; s < sectors; s++) {
            uint32_t first = r * (sectors + 1) + s;
            uint32_t second = first + sectors + 1;
            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);
            indices.push_back(first + 1);
            indices.push_back(second);
            indices.push_back(second + 1);
        }
    }

    glGenVertexArrays(1, &g_SphereVAO);
    glGenBuffers(1, &g_SphereVBO);
    glGenBuffers(1, &g_SphereEBO);

    glBindVertexArray(g_SphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_SphereVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ObjectVertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_SphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ObjectVertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ObjectVertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ObjectVertex), (void*)(6 * sizeof(float)));
    glBindVertexArray(0);

    g_SphereIndexCount = (uint32_t)indices.size();
}

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

bool LoadOBJ(const char* filepath, GLuint& vao, GLuint& vbo, GLuint& ebo, uint32_t& indexCount)
{
    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = "./";

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(filepath, reader_config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader error: " << reader.Error() << std::endl;
        }
        return false;
    }

    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader warning: " << reader.Warning() << std::endl;
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();

    std::vector<ObjectVertex> vertices;
    std::vector<uint32_t> indices;

    for (size_t s = 0; s < shapes.size(); s++) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                ObjectVertex vertex;
                vertex.x = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                vertex.y = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                vertex.z = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

                if (idx.normal_index >= 0) {
                    vertex.nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
                    vertex.ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
                    vertex.nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
                } else {
                    vertex.nx = 0.0f;
                    vertex.ny = 1.0f;
                    vertex.nz = 0.0f;
                }

                if (idx.texcoord_index >= 0) {
                    vertex.u = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                    vertex.v = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
                } else {
                    vertex.u = 0.0f;
                    vertex.v = 0.0f;
                }

                vertices.push_back(vertex);
                indices.push_back((uint32_t)indices.size());
            }
            index_offset += fv;
        }
    }

    if (vertices.empty()) return false;

    indexCount = (uint32_t)indices.size();

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ObjectVertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ObjectVertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ObjectVertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ObjectVertex), (void*)(6 * sizeof(float)));
    glBindVertexArray(0);

    return true;
}

void CreateDragonMesh()
{
    if (!LoadOBJ("dragon.obj", g_DragonVAO, g_DragonVBO, g_DragonEBO, g_DragonIndexCount)) {
        std::cerr << "Failed to load dragon.obj mesh!" << std::endl;
    }
}


void CreateGroundMesh()
{
    // Large ground plane at y = -0.5, subdivided into a grid for checkerboard effect
    const float halfSize = 50.f;
    const int divisions = 50;
    const float step = (2.f * halfSize) / divisions;

    std::vector<ObjectVertex> vertices;
    vertices.reserve(divisions * divisions * 6);

    for (int i = 0; i < divisions; i++) {
        for (int j = 0; j < divisions; j++) {
            float x0 = -halfSize + i * step;
            float z0 = -halfSize + j * step;
            float x1 = x0 + step;
            float z1 = z0 + step;
            float y = -1.5f;

            // UV tiling for checkerboard
            float u0 = (float)i;
            float v0 = (float)j;
            float u1 = (float)(i + 1);
            float v1 = (float)(j + 1);

            vec3 normal = { 0.f, 1.f, 0.f };

            // Triangle 1
            vertices.push_back({ x0, y, z0, normal.x, normal.y, normal.z, u0, v0 });
            vertices.push_back({ x1, y, z0, normal.x, normal.y, normal.z, u1, v0 });
            vertices.push_back({ x1, y, z1, normal.x, normal.y, normal.z, u1, v1 });
            // Triangle 2
            vertices.push_back({ x0, y, z0, normal.x, normal.y, normal.z, u0, v0 });
            vertices.push_back({ x1, y, z1, normal.x, normal.y, normal.z, u1, v1 });
            vertices.push_back({ x0, y, z1, normal.x, normal.y, normal.z, u0, v1 });
        }
    }

    g_GroundVertexCount = (uint32_t)vertices.size();

    glGenVertexArrays(1, &g_GroundVAO);
    glGenBuffers(1, &g_GroundVBO);

    glBindVertexArray(g_GroundVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_GroundVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ObjectVertex), vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ObjectVertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ObjectVertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ObjectVertex), (void*)(6 * sizeof(float)));
    glBindVertexArray(0);
}

void UpdateCamera(GLFWwindow* window, float deltaTime)
{
    const float speed = 4.0f;
    vec3 forward;
    float yawRad = g_CameraYaw * 3.14159265f / 180.f;
    float pitchRad = g_CameraPitch * 3.14159265f / 180.f;
    forward.x = cosf(yawRad) * cosf(pitchRad);
    forward.y = sinf(pitchRad);
    forward.z = sinf(yawRad) * cosf(pitchRad);
    forward = normalize(forward);
    vec3 right = normalize(cross(forward, { 0.f, 1.f, 0.f }));
    vec3 up = normalize(cross(right, forward));
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    g_CameraPosition = add(g_CameraPosition, mul(moveForward, speed * deltaTime));

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    g_CameraPosition = add(g_CameraPosition, mul(moveForward, -speed * deltaTime));

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) g_CameraPosition = add(g_CameraPosition, mul(right, -speed * deltaTime));
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) g_CameraPosition = add(g_CameraPosition, mul(right, speed * deltaTime));
    /* test */
    /*
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) g_CameraPosition.y += speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) g_CameraPosition.y -= speed * deltaTime;
    */
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) g_CameraPosition = add(g_CameraPosition, mul(up, speed * deltaTime));
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) g_CameraPosition = add(g_CameraPosition, mul(up, -speed * deltaTime));

    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) g_CameraYaw -= 90.f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) g_CameraYaw += 90.f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) g_CameraPitch += 60.f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) g_CameraPitch -= 60.f * deltaTime;

    if (g_CameraPitch > 89.f) g_CameraPitch = 89.f;
    if (g_CameraPitch < -89.f) g_CameraPitch = -89.f;
}

// Mouse movement callback: adjusts yaw/pitch while mouse button is pressed
void MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (!g_MousePressed) {
        // keep last position in sync so we don't get a jump when user presses
        g_LastX = xpos;
        g_LastY = ypos;
        g_FirstMouse = false;
        return;
    }

    if (g_FirstMouse) {
        g_LastX = xpos;
        g_LastY = ypos;
        g_FirstMouse = false;
    }

    double xoffset = xpos - g_LastX;
    double yoffset = g_LastY - ypos; // reversed since y-coordinates range from bottom to top
    g_LastX = xpos;
    g_LastY = ypos;

    xoffset *= g_MouseSensitivity;
    yoffset *= g_MouseSensitivity;

    g_CameraYaw += (float)xoffset;
    g_CameraPitch += (float)yoffset;

    if (g_CameraPitch > 89.0f) g_CameraPitch = 89.0f;
    if (g_CameraPitch < -89.0f) g_CameraPitch = -89.0f;
    // debug output to verify mouse updates
    //std::cout << "Mouse look -> Yaw: " << g_CameraYaw << " Pitch: " << g_CameraPitch << std::endl;
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            g_MousePressed = true;
            g_FirstMouse = true; // reset to avoid sudden jump
            // optionally hide cursor while rotating
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else if (action == GLFW_RELEASE) {
            g_MousePressed = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

// Pour partie 2 

//ajout de Create FBO
void CreateFBO()
{
    glGenFramebuffers(1, &g_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, g_FBO);

    glGenTextures(1, &g_FBOTex);
    glBindTexture(GL_TEXTURE_2D, g_FBOTex);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        g_Width,
        g_Height,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        nullptr
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        g_FBOTex,
        0
    );

    glGenRenderbuffers(1, &g_DepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, g_DepthRBO);

    glRenderbufferStorage(
        GL_RENDERBUFFER,
        GL_DEPTH24_STENCIL8,
        g_Width,
        g_Height
    );

    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER,
        g_DepthRBO
    );

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "FBO incomplet !" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//Toujours pour partie 2 
//jout de quad plein écran 
void CreateScreenQuad()
{
    float quad[] =
    {
        -1.f,-1.f,0.f,0.f,
         1.f,-1.f,1.f,0.f,
         1.f, 1.f,1.f,1.f,

        -1.f,-1.f,0.f,0.f,
         1.f, 1.f,1.f,1.f,
        -1.f, 1.f,0.f,1.f
    };

    glGenVertexArrays(1, &g_ScreenQuadVAO);
    glGenBuffers(1, &g_ScreenQuadVBO);

    glBindVertexArray(g_ScreenQuadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, g_ScreenQuadVBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(quad),quad,GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float)));

    glBindVertexArray(0);
}

bool Initialise()
{
    g_SkyboxShader.LoadVertexShader("basic.vs");
    g_SkyboxShader.LoadFragmentShader("basic.fs");
    if (!g_SkyboxShader.Create()) {
        return false;
    }

    g_ObjectShader.LoadVertexShader("object.vs");
    g_ObjectShader.LoadFragmentShader("object.fs");
    if (!g_ObjectShader.Create()) {
        return false;
    }

    CreateSkyboxMesh();
    CreateCubeMesh();
    CreateSphereMesh(32, 32);
    CreateDragonMesh();
    CreateGroundMesh();

    const char* textures_faces[6] = {
        "texture3/posx.jpg",
        "texture3/negx.jpg",
        "texture3/posy.jpg",
        "texture3/negy.jpg",
        "texture3/posz.jpg",
        "texture3/negz.jpg",
    };
    g_CubemapTexture = LoadCubemap(textures_faces);
    g_DragonTexture = LoadTexture2D("dragon.png");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);


    //Pour partie 2
    g_ScreenShader.LoadVertexShader("screen.vs");
    g_ScreenShader.LoadFragmentShader("screen.fs");
    g_ScreenShader.Create();

    CreateFBO();
    CreateScreenQuad();

    return true;
}

void Terminate()
{
    g_SkyboxShader.Destroy();
    g_ObjectShader.Destroy();
    glDeleteVertexArrays(1, &g_SkyboxVAO);
    glDeleteBuffers(1, &g_SkyboxVBO);
    glDeleteBuffers(1, &g_SkyboxEBO);
    glDeleteTextures(1, &g_CubemapTexture);
    glDeleteVertexArrays(1, &g_CubeVAO);
    glDeleteBuffers(1, &g_CubeVBO);
    glDeleteVertexArrays(1, &g_SphereVAO);
    glDeleteBuffers(1, &g_SphereVBO);
    glDeleteBuffers(1, &g_SphereEBO);
    glDeleteVertexArrays(1, &g_DragonVAO);
    glDeleteBuffers(1, &g_DragonVBO);
    glDeleteBuffers(1, &g_DragonEBO);
    glDeleteTextures(1, &g_DragonTexture);
    glDeleteVertexArrays(1, &g_GroundVAO);
    glDeleteBuffers(1, &g_GroundVBO);

    //Terminations de ce qui a été initialisé pour la partie 2
    g_ScreenShader.Destroy();
    glDeleteFramebuffers(1, &g_FBO);
    glDeleteTextures(1, &g_FBOTex);
    glDeleteRenderbuffers(1, &g_DepthRBO);
    glDeleteVertexArrays(1, &g_ScreenQuadVAO);
    glDeleteBuffers(1, &g_ScreenQuadVBO);
}

void OnResize(GLFWwindow*, int width, int height)
{
    g_Width = width;
    g_Height = height;
}

void RenderScene(GLFWwindow* window, float width, float height)
{

    //Pour partie 2 : 
    //Remplacement de ça : 
    /*
    glViewport(0, 0, width, height);
    glClearColor(0.12f, 0.12f, 0.16f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    */

    //Par ça : 
    glBindFramebuffer(GL_FRAMEBUFFER, g_FBO);
    glViewport(0,0,width,height);
    glClearColor(0.12f,0.12f,0.16f,1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    static double lastTime = glfwGetTime();
    double currentTime = glfwGetTime();
    float deltaTime = (float)(currentTime - lastTime);
    lastTime = currentTime;
    UpdateCamera(window, deltaTime);

    float aspect = width / height;
    mat4 projection = makePerspective(60.f * 3.14159265f / 180.f, aspect, 0.1f, 100.f);

    vec3 cameraFront;
    float yawRad = g_CameraYaw * 3.14159265f / 180.f;
    float pitchRad = g_CameraPitch * 3.14159265f / 180.f;
    cameraFront.x = cosf(yawRad) * cosf(pitchRad);
    cameraFront.y = sinf(pitchRad);
    cameraFront.z = sinf(yawRad) * cosf(pitchRad);
    cameraFront = normalize(cameraFront);

    vec3 cameraTarget = add(g_CameraPosition, cameraFront);
    mat4 view = makeLookAt(g_CameraPosition, cameraTarget, { 0.f, 1.f, 0.f });

    glDepthFunc(GL_LEQUAL);
    glUseProgram(g_SkyboxShader.GetProgram());
    mat4 viewSky = view;
    viewSky.m[3] = 0.f;
    viewSky.m[7] = 0.f;
    viewSky.m[11] = 0.f;
    mat4 skyMVP = multiply(projection, viewSky);
    glUniformMatrix4fv(glGetUniformLocation(g_SkyboxShader.GetProgram(), "u_MVP"), 1, GL_TRUE, skyMVP.m);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, g_CubemapTexture);
    glUniform1i(glGetUniformLocation(g_SkyboxShader.GetProgram(), "u_cubemap"), 0);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glBindVertexArray(g_SkyboxVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    glUseProgram(g_ObjectShader.GetProgram());
    GLint mvpLoc = glGetUniformLocation(g_ObjectShader.GetProgram(), "u_MVP");
    GLint modelLoc = glGetUniformLocation(g_ObjectShader.GetProgram(), "u_Model");
    GLint useTextureLoc = glGetUniformLocation(g_ObjectShader.GetProgram(), "u_UseTexture");
    GLint colorLoc = glGetUniformLocation(g_ObjectShader.GetProgram(), "u_Color");

    // Upload camera position for Blinn-Phong and reflection
    glUniform3f(glGetUniformLocation(g_ObjectShader.GetProgram(), "u_ViewPos"), g_CameraPosition.x, g_CameraPosition.y, g_CameraPosition.z);

    // Bind cubemap to texture unit 1 for indirect specular reflection
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, g_CubemapTexture);
    glUniform1i(glGetUniformLocation(g_ObjectShader.GetProgram(), "u_Cubemap"), 1);

    // Ground plane (checkerboard)
    {
        mat4 groundModel; // identity — ground is already at y=-0.5
        mat4 groundMVP = multiply(projection, multiply(view, groundModel));
        glUniformMatrix4fv(mvpLoc, 1, GL_TRUE, groundMVP.m);
        glUniformMatrix4fv(modelLoc, 1, GL_TRUE, groundModel.m);
        glUniform1i(useTextureLoc, 2); // checkerboard mode
        glUniform3f(colorLoc, 1.f, 1.f, 1.f);
        glDisable(GL_CULL_FACE);
        glBindVertexArray(g_GroundVAO);
        glDrawArrays(GL_TRIANGLES, 0, g_GroundVertexCount);
        glBindVertexArray(0);
        glEnable(GL_CULL_FACE);
    }

    mat4 sphereModel = makeScale(1.2f, 1.2f, 1.2f);
    mat4 sphereMVP = multiply(projection, multiply(view, sphereModel));
    glUniformMatrix4fv(mvpLoc, 1, GL_TRUE, sphereMVP.m);
    glUniformMatrix4fv(modelLoc, 1, GL_TRUE, sphereModel.m);
    glUniform1i(useTextureLoc, 0);
    glUniform3f(colorLoc, 0.96f, 0.96f, 0.96f);
    glBindVertexArray(g_SphereVAO);
    glDrawElements(GL_TRIANGLES, g_SphereIndexCount, GL_UNSIGNED_INT, 0);

    float time = (float)currentTime;
    mat4 dragonModel = multiply(makeTranslation(0.f, 1.15f, 0.f), multiply(makeRotationY(-time * 0.9f), makeScale(0.35f, 0.35f, 0.35f)));
    mat4 dragonMVP = multiply(projection, multiply(view, dragonModel));
    glUniformMatrix4fv(mvpLoc, 1, GL_TRUE, dragonMVP.m);
    glUniformMatrix4fv(modelLoc, 1, GL_TRUE, dragonModel.m);
    glUniform1i(useTextureLoc, 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_DragonTexture);
    glUniform1i(glGetUniformLocation(g_ObjectShader.GetProgram(), "u_Texture"), 0);
    glUniform3f(colorLoc, 1.f, 1.f, 1.f);
    glBindVertexArray(g_DragonVAO);
    glDrawElements(GL_TRIANGLES, g_DragonIndexCount, GL_UNSIGNED_INT, 0);

    glBindVertexArray(g_CubeVAO);
    const vec3 cubePositionsFixed[8] = {
        { -5.0f, 0.55f, -5.0f },
        {  0.0f, 0.55f, -5.5f },
        {  5.0f, 0.55f, -5.0f },
        { -5.5f, 0.55f,  0.0f },
        {  5.5f, 0.55f,  0.0f },
        { -5.0f, 0.55f,  5.0f },
        {  0.0f, 0.55f,  5.5f },
        {  5.0f, 0.55f,  5.0f }
    };
    for (int i = 0; i < 8; i++) {
        vec3 position = cubePositionsFixed[i];
        float rotation = (i % 2 == 0) ? 0.f : 0.45f;
        mat4 cubeModel = multiply(makeTranslation(position.x, position.y, position.z), makeRotationY(rotation));
        cubeModel = multiply(cubeModel, makeScale(0.65f, 0.65f, 0.65f));
        mat4 cubeMVP = multiply(projection, multiply(view, cubeModel));
        glUniformMatrix4fv(mvpLoc, 1, GL_TRUE, cubeMVP.m);
        glUniformMatrix4fv(modelLoc, 1, GL_TRUE, cubeModel.m);
        glUniform1i(useTextureLoc, 0);
        if ((i % 2) == 0) glUniform3f(colorLoc, 0.96f, 0.96f, 0.96f);
        else glUniform3f(colorLoc, 0.05f, 0.05f, 0.05f);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glBindVertexArray(0);
    
    
    //Pour partie 2
    //affichage du rendu FBO sur l'écran avec post-traitement
    glBindFramebuffer(GL_FRAMEBUFFER,0);

    glDisable(GL_DEPTH_TEST);

    glUseProgram(g_ScreenShader.GetProgram());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,g_FBOTex);

    glUniform1i(glGetUniformLocation(g_ScreenShader.GetProgram(),"u_ScreenTexture"),0);

    glUniform1i(glGetUniformLocation(g_ScreenShader.GetProgram(),"u_EffectMode"),g_EffectMode);

    glBindVertexArray(g_ScreenQuadVAO);
    glDrawArrays(GL_TRIANGLES,0,6);

    glEnable(GL_DEPTH_TEST);
}

int main()
{
    if (!glfwInit()) {
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(g_Width, g_Height, "Scene Cubemap Dragon", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, OnResize);
    // mouse callbacks for view control
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // initialize mouse center
    int fbw, fbh;
    glfwGetFramebufferSize(window, &fbw, &fbh);
    g_LastX = fbw * 0.5;
    g_LastY = fbh * 0.5;
    glfwGetFramebufferSize(window, &g_Width, &g_Height);

    if (!Initialise()) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        //Pour partie 2 
        if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) g_EffectMode = 0;
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) g_EffectMode = 1;
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) g_EffectMode = 2;
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) g_EffectMode = 3;
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) g_EffectMode = 4;

        RenderScene(window, (float)g_Width, (float)g_Height);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    Terminate();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
