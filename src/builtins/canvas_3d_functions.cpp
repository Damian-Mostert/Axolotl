#include "include/builtins.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_opengl.h>
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>

#ifndef GL_GENERATE_MIPMAP
#define GL_GENERATE_MIPMAP 0x8191
#endif

typedef void (*PFNGLGENERATEMIPMAPPROC)(GLenum target);
static PFNGLGENERATEMIPMAPPROC glGenerateMipmapPtr = nullptr;
struct Vec3
{
    float x, y, z;
};
struct Vec2
{
    float x, y;
};
struct TextureOptions {
    std::string path;
    Vec2 scale{1, 1};
    Vec2 offset{0, 0};
    bool clamp{false};
};
struct Bone
{
    std::string name;
    Vec3 position{0, 0, 0};
    Vec3 rotation{0, 0, 0};
    int parentId = -1;
};
struct Animation
{
    std::string name;
    std::unordered_map<std::string, std::vector<std::pair<float, Vec3>>> positionKeys;
    std::unordered_map<std::string, std::vector<std::pair<float, Vec3>>> rotationKeys;
    float duration = 0.0f;
};
struct Mesh
{
    std::vector<Vec3> vertices;
    std::vector<Vec2> uvs;
    std::vector<int> indices;
    std::vector<int> uvIndices;
    std::vector<std::string> faceMaterials;
    std::unordered_map<std::string, GLuint> materialTextures;
    std::unordered_map<std::string, SDL_Color> materialColors;
    std::unordered_map<std::string, TextureOptions> materialTexOpts;
    Vec3 position{0, 0, 0};
    Vec3 rotation{0, 0, 0};
    Vec3 scale{1, 1, 1};
    SDL_Color color{255, 255, 255, 255};
    bool visible = true;
    bool wireframe = false;
    bool doubleSided = false;
    float metallic = 0.0f;
    Vec3 velocity{0, 0, 0};
    Vec3 angularVelocity{0, 0, 0};
    float mass = 1.0f;
    Vec3 aabbMin{0, 0, 0};
    Vec3 aabbMax{0, 0, 0};
    float friction = 0.5f;
    float bounciness = 0.3f;
    bool castShadow = true;
    bool receiveShadow = true;
    int collisionShape = 0;
    float collisionRadius = 1.0f;
    GLuint textureId = 0;
    Vec3 groundNormal{0, 1, 0};
};
struct Camera
{
    Vec3 position{0, 0, 5};
    float fov = 60.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    float distance = 5.0f;
    Vec3 target{0, 0, 0};
};
struct Light
{
    Vec3 position{5, 10, 5};
    SDL_Color color{255, 255, 255, 255};
    float intensity = 1.0f;
};
struct Scene
{
    std::vector<int> meshIds;
    std::vector<int> lightIds;
    SDL_Color background{20, 20, 30, 255};
    Vec3 ambientLight{0.3f, 0.3f, 0.3f};
    bool enableMSAA = true;
    int msaaSamples = 4;
    bool enableDepthTest = true;
    bool enableSmoothing = true;
    bool enableShadows = false;
    float shadowIntensity = 0.5f;
    bool enableFog = false;
    float fogDensity = 0.02f;
    SDL_Color fogColor{128, 128, 128, 255};
};
extern std::unordered_map<int, std::shared_ptr<CanvasContext>> canvases;
static std::unordered_map<int, Mesh> meshes;
static std::unordered_map<int, Camera> cameras;
static std::unordered_map<int, Scene> scenes;
static std::unordered_map<int, Light> lights;
static std::unordered_map<int, std::vector<Bone>> skeletons;
static std::unordered_map<int, std::vector<Animation>> animations;
static std::unordered_map<int, std::pair<int, float>> animationStates;
static std::unordered_map<int, std::vector<std::pair<std::string, TextureOptions>>> pendingTextures;
static int nextMeshId = 1, nextCameraId = 1, nextSceneId = 1, nextLightId = 1;
static bool devMode = false;
static int devCameraId = -1;
static int lastMouseX = 0, lastMouseY = 0;
static bool mouseDown = false;
Vec3 rotateX(Vec3 v, float a)
{
    float c = cos(a), s = sin(a);
    return {v.x, v.y * c - v.z * s, v.y * s + v.z * c};
}
Vec3 rotateY(Vec3 v, float a)
{
    float c = cos(a), s = sin(a);
    return {v.x * c + v.z * s, v.y, -v.x * s + v.z * c};
}
Vec3 rotateZ(Vec3 v, float a)
{
    float c = cos(a), s = sin(a);
    return {v.x * c - v.y * s, v.x * s + v.y * c, v.z};
}
void calcAABB(Mesh &mesh)
{
    if (mesh.vertices.empty())
        return;
    mesh.aabbMin = mesh.aabbMax = mesh.vertices[0];
    for (const auto &v : mesh.vertices)
    {
        if (v.x < mesh.aabbMin.x)
            mesh.aabbMin.x = v.x;
        if (v.y < mesh.aabbMin.y)
            mesh.aabbMin.y = v.y;
        if (v.z < mesh.aabbMin.z)
            mesh.aabbMin.z = v.z;
        if (v.x > mesh.aabbMax.x)
            mesh.aabbMax.x = v.x;
        if (v.y > mesh.aabbMax.y)
            mesh.aabbMax.y = v.y;
        if (v.z > mesh.aabbMax.z)
            mesh.aabbMax.z = v.z;
    }
}
Vec3 transformVertex(Vec3 v, const Mesh &m)
{
    v.x *= m.scale.x;
    v.y *= m.scale.y;
    v.z *= m.scale.z;
    v = rotateX(v, m.rotation.x);
    v = rotateY(v, m.rotation.y);
    v = rotateZ(v, m.rotation.z);
    v.x += m.position.x;
    v.y += m.position.y;
    v.z += m.position.z;
    return v;
}
bool rayTriangleIntersect(Vec3 origin, Vec3 dir, Vec3 v0, Vec3 v1, Vec3 v2, float &t, Vec3 &normal)
{
    Vec3 e1 = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
    Vec3 e2 = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};
    Vec3 h = {dir.y * e2.z - dir.z * e2.y, dir.z * e2.x - dir.x * e2.z, dir.x * e2.y - dir.y * e2.x};
    float a = e1.x * h.x + e1.y * h.y + e1.z * h.z;
    if (fabs(a) < 0.0001f)
        return false;
    float f = 1.0f / a;
    Vec3 s = {origin.x - v0.x, origin.y - v0.y, origin.z - v0.z};
    float u = f * (s.x * h.x + s.y * h.y + s.z * h.z);
    if (u < 0.0f || u > 1.0f)
        return false;
    Vec3 q = {s.y * e1.z - s.z * e1.y, s.z * e1.x - s.x * e1.z, s.x * e1.y - s.y * e1.x};
    float v = f * (dir.x * q.x + dir.y * q.y + dir.z * q.z);
    if (v < 0.0f || u + v > 1.0f)
        return false;
    t = f * (e2.x * q.x + e2.y * q.y + e2.z * q.z);
    normal = {e1.y * e2.z - e1.z * e2.y, e1.z * e2.x - e1.x * e2.z, e1.x * e2.y - e1.y * e2.x};
    float len = sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    if (len > 0.0001f)
    {
        normal.x /= len;
        normal.y /= len;
        normal.z /= len;
    }
    return t >= 0.0f;
}
bool aabbIntersect(Vec3 min1, Vec3 max1, Vec3 min2, Vec3 max2)
{
    return (min1.x <= max2.x && max1.x >= min2.x) &&
           (min1.y <= max2.y && max1.y >= min2.y) &&
           (min1.z <= max2.z && max1.z >= min2.z);
}
Vec3 transformToCamera(Vec3 v, Camera &cam)
{
    Vec3 rel = {v.x - cam.position.x, v.y - cam.position.y, v.z - cam.position.z};
    Vec3 forward = {cam.target.x - cam.position.x, cam.target.y - cam.position.y, cam.target.z - cam.position.z};
    float len = sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
    if (len < 0.001f)
        return {rel.x, rel.y, -rel.z};
    forward.x /= len;
    forward.y /= len;
    forward.z /= len;
    Vec3 worldUp = {0, 1, 0};
    Vec3 right = {worldUp.y * forward.z - worldUp.z * forward.y, worldUp.z * forward.x - worldUp.x * forward.z, worldUp.x * forward.y - worldUp.y * forward.x};
    len = sqrt(right.x * right.x + right.y * right.y + right.z * right.z);
    if (len < 0.001f)
        return {rel.x, rel.y, -rel.z};
    right.x /= len;
    right.y /= len;
    right.z /= len;
    Vec3 up = {forward.y * right.z - forward.z * right.y, forward.z * right.x - forward.x * right.z, forward.x * right.y - forward.y * right.x};
    return {right.x * rel.x + right.y * rel.y + right.z * rel.z,
            up.x * rel.x + up.y * rel.y + up.z * rel.z,
            -(forward.x * rel.x + forward.y * rel.y + forward.z * rel.z)};
}
Vec3 project(Vec3 v, float fov, int w, int h)
{
    float aspect = (float)w / (float)h;
    float fovRad = fov * M_PI / 180.0f;
    float f = 1.0f / tan(fovRad / 2.0f);
    if (v.z > -0.1f)
        v.z = -0.1f;
    float x = (v.x * f / aspect / -v.z) * (w / 2.0f) + (w / 2.0f);
    float y = (-v.y * f / -v.z) * (h / 2.0f) + (h / 2.0f);
    return {x, y, v.z};
}
class CreateSceneBuiltin : public BuiltinFunction
{
public:
    //@desc Create a new 3D scene container
    std::string getName() const override { return "createScene"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        int id = nextSceneId++;
        scenes[id] = Scene();
        auto obj = std::make_shared<ObjectValue>();
        obj->fields["_sceneId"] = id;
        interp->lastValue = obj;
        return "{object}";
    }
};
class AddToSceneBuiltin : public BuiltinFunction
{
public:
    //@desc Add mesh or light to scene
    //@parent scene
    std::string getName() const override { return "add"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (!node->callee || node->args.size() != 1)
            throw std::runtime_error("scene.add(mesh)");
        auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
        Value sceneVal = interp->evaluate(fa->object.get());
        auto sceneObj = std::get<std::shared_ptr<ObjectValue>>(sceneVal);
        int sceneId = std::get<int>(sceneObj->fields["_sceneId"]);
        Value meshVal = interp->evaluate(node->args[0].get());
        auto meshObj = std::get<std::shared_ptr<ObjectValue>>(meshVal);
        if (meshObj->fields.count("_meshId"))
            scenes[sceneId].meshIds.push_back(std::get<int>(meshObj->fields["_meshId"]));
        else if (meshObj->fields.count("_lightId"))
            scenes[sceneId].lightIds.push_back(std::get<int>(meshObj->fields["_lightId"]));
        return "";
    }
};
class RemoveMeshBuiltin : public BuiltinFunction
{
public:
    //@desc Remove mesh or light from scene
    //@parent scene
    std::string getName() const override { return "removeMesh"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (!node->callee || node->args.size() != 1)
            throw std::runtime_error("scene.removeMesh(mesh)");
        auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
        Value sceneVal = interp->evaluate(fa->object.get());
        auto sceneObj = std::get<std::shared_ptr<ObjectValue>>(sceneVal);
        int sceneId = std::get<int>(sceneObj->fields["_sceneId"]);
        Value meshVal = interp->evaluate(node->args[0].get());
        auto meshObj = std::get<std::shared_ptr<ObjectValue>>(meshVal);
        if (meshObj->fields.count("_meshId"))
        {
            int meshId = std::get<int>(meshObj->fields["_meshId"]);
            auto &meshIds = scenes[sceneId].meshIds;
            meshIds.erase(std::remove(meshIds.begin(), meshIds.end(), meshId), meshIds.end());
        }
        else if (meshObj->fields.count("_lightId"))
        {
            int lightId = std::get<int>(meshObj->fields["_lightId"]);
            auto &lightIds = scenes[sceneId].lightIds;
            lightIds.erase(std::remove(lightIds.begin(), lightIds.end(), lightId), lightIds.end());
        }
        return "";
    }
};
class BoxGeometryBuiltin : public BuiltinFunction
{
public:
    //@desc Create box geometry with width, height, depth
    std::string getName() const override { return "BoxGeometry"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        float w = 1, h = 1, d = 1;
        if (node->args.size() >= 1)
        {
            auto v = interp->evaluate(node->args[0].get());
            w = h = d = std::holds_alternative<int>(v) ? std::get<int>(v) : std::get<float>(v);
        }
        if (node->args.size() >= 3)
        {
            auto v1 = interp->evaluate(node->args[1].get());
            auto v2 = interp->evaluate(node->args[2].get());
            h = std::holds_alternative<int>(v1) ? std::get<int>(v1) : std::get<float>(v1);
            d = std::holds_alternative<int>(v2) ? std::get<int>(v2) : std::get<float>(v2);
        }
        Mesh mesh;
        mesh.vertices = {{-w / 2, -h / 2, -d / 2}, {w / 2, -h / 2, -d / 2}, {w / 2, h / 2, -d / 2}, {-w / 2, h / 2, -d / 2}, {-w / 2, -h / 2, d / 2}, {w / 2, -h / 2, d / 2}, {w / 2, h / 2, d / 2}, {-w / 2, h / 2, d / 2}};
        mesh.indices = {0, 2, 1, 0, 3, 2, 1, 6, 5, 1, 2, 6, 5, 7, 4, 5, 6, 7, 4, 3, 0, 4, 7, 3, 3, 6, 2, 3, 7, 6, 4, 1, 5, 4, 0, 1};
        calcAABB(mesh);
        int id = nextMeshId++;
        meshes[id] = mesh;
        auto obj = std::make_shared<ObjectValue>();
        obj->fields["_meshId"] = id;
        interp->lastValue = obj;
        return "{object}";
    }
};
class SphereGeometryBuiltin : public BuiltinFunction
{
public:
    //@desc Create sphere geometry with radius and segments
    std::string getName() const override { return "SphereGeometry"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        float r = 1;
        int wSeg = 16, hSeg = 12;
        if (node->args.size() >= 1)
        {
            auto v = interp->evaluate(node->args[0].get());
            r = std::holds_alternative<int>(v) ? std::get<int>(v) : std::get<float>(v);
        }
        if (node->args.size() >= 2)
            wSeg = std::get<int>(interp->evaluate(node->args[1].get()));
        if (node->args.size() >= 3)
            hSeg = std::get<int>(interp->evaluate(node->args[2].get()));
        Mesh mesh;
        for (int i = 0; i <= hSeg; i++)
        {
            float theta = i * M_PI / hSeg;
            for (int j = 0; j <= wSeg; j++)
            {
                float phi = j * 2 * M_PI / wSeg;
                mesh.vertices.push_back({r * sin(theta) * cos(phi), r * cos(theta), r * sin(theta) * sin(phi)});
            }
        }
        for (int i = 0; i < hSeg; i++)
        {
            for (int j = 0; j < wSeg; j++)
            {
                int a = i * (wSeg + 1) + j, b = a + wSeg + 1;
                mesh.indices.push_back(a);
                mesh.indices.push_back(a + 1);
                mesh.indices.push_back(b);
                mesh.indices.push_back(b);
                mesh.indices.push_back(a + 1);
                mesh.indices.push_back(b + 1);
            }
        }
        calcAABB(mesh);
        int id = nextMeshId++;
        meshes[id] = mesh;
        auto obj = std::make_shared<ObjectValue>();
        obj->fields["_meshId"] = id;
        interp->lastValue = obj;
        return "{object}";
    }
};
class PlaneGeometryBuiltin : public BuiltinFunction
{
public:
    //@desc Create plane geometry with width and height
    std::string getName() const override { return "PlaneGeometry"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        float w = 1, h = 1;
        if (node->args.size() >= 2)
        {
            auto v0 = interp->evaluate(node->args[0].get());
            auto v1 = interp->evaluate(node->args[1].get());
            w = std::holds_alternative<int>(v0) ? std::get<int>(v0) : std::get<float>(v0);
            h = std::holds_alternative<int>(v1) ? std::get<int>(v1) : std::get<float>(v1);
        }
        Mesh mesh;
        mesh.vertices = {{-w / 2, -h / 2, 0}, {w / 2, -h / 2, 0}, {w / 2, h / 2, 0}, {-w / 2, h / 2, 0}};
        mesh.indices = {0, 2, 1, 0, 3, 2};
        mesh.doubleSided = true;
        calcAABB(mesh);
        int id = nextMeshId++;
        meshes[id] = mesh;
        auto obj = std::make_shared<ObjectValue>();
        obj->fields["_meshId"] = id;
        interp->lastValue = obj;
        return "{object}";
    }
};
class TorusGeometryBuiltin : public BuiltinFunction
{
public:
    //@desc Create torus geometry with radius, tube, and segments
    std::string getName() const override { return "TorusGeometry"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        float radius = 1, tube = 0.4f;
        int rSeg = 16, tSeg = 32;
        if (node->args.size() >= 1)
        {
            auto v = interp->evaluate(node->args[0].get());
            radius = std::holds_alternative<int>(v) ? std::get<int>(v) : std::get<float>(v);
        }
        if (node->args.size() >= 2)
        {
            auto v = interp->evaluate(node->args[1].get());
            tube = std::holds_alternative<int>(v) ? std::get<int>(v) : std::get<float>(v);
        }
        if (node->args.size() >= 3)
            rSeg = std::get<int>(interp->evaluate(node->args[2].get()));
        if (node->args.size() >= 4)
            tSeg = std::get<int>(interp->evaluate(node->args[3].get()));
        Mesh mesh;
        for (int i = 0; i <= rSeg; i++)
        {
            float u = i * 2 * M_PI / rSeg;
            for (int j = 0; j <= tSeg; j++)
            {
                float v = j * 2 * M_PI / tSeg;
                mesh.vertices.push_back({(radius + tube * cos(v)) * cos(u), tube * sin(v), (radius + tube * cos(v)) * sin(u)});
            }
        }
        for (int i = 0; i < rSeg; i++)
        {
            for (int j = 0; j < tSeg; j++)
            {
                int a = i * (tSeg + 1) + j, b = a + tSeg + 1;
                mesh.indices.push_back(a);
                mesh.indices.push_back(a + 1);
                mesh.indices.push_back(b);
                mesh.indices.push_back(b);
                mesh.indices.push_back(a + 1);
                mesh.indices.push_back(b + 1);
            }
        }
        calcAABB(mesh);
        int id = nextMeshId++;
        meshes[id] = mesh;
        auto obj = std::make_shared<ObjectValue>();
        obj->fields["_meshId"] = id;
        interp->lastValue = obj;
        return "{object}";
    }
};
class CylinderGeometryBuiltin : public BuiltinFunction
{
public:
    //@desc Create cylinder geometry with radii, height, and segments
    std::string getName() const override { return "CylinderGeometry"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        float radiusTop = 1, radiusBottom = 1, height = 2;
        int radialSegments = 32;
        if (node->args.size() >= 1)
        {
            auto v = interp->evaluate(node->args[0].get());
            radiusTop = std::holds_alternative<int>(v) ? std::get<int>(v) : std::get<float>(v);
        }
        if (node->args.size() >= 2)
        {
            auto v = interp->evaluate(node->args[1].get());
            radiusBottom = std::holds_alternative<int>(v) ? std::get<int>(v) : std::get<float>(v);
        }
        if (node->args.size() >= 3)
        {
            auto v = interp->evaluate(node->args[2].get());
            height = std::holds_alternative<int>(v) ? std::get<int>(v) : std::get<float>(v);
        }
        if (node->args.size() >= 4)
            radialSegments = std::get<int>(interp->evaluate(node->args[3].get()));
        Mesh mesh;
        float halfHeight = height / 2;
        for (int i = 0; i <= radialSegments; i++)
        {
            float theta = i * 2 * M_PI / radialSegments;
            float cosT = cos(theta), sinT = sin(theta);
            mesh.vertices.push_back({radiusTop * cosT, halfHeight, radiusTop * sinT});
            mesh.vertices.push_back({radiusBottom * cosT, -halfHeight, radiusBottom * sinT});
        }
        for (int i = 0; i < radialSegments; i++)
        {
            int a = i * 2, b = a + 1, c = a + 2, d = a + 3;
            mesh.indices.push_back(a);
            mesh.indices.push_back(c);
            mesh.indices.push_back(b);
            mesh.indices.push_back(b);
            mesh.indices.push_back(c);
            mesh.indices.push_back(d);
        }
        int centerTop = mesh.vertices.size();
        mesh.vertices.push_back({0, halfHeight, 0});
        int centerBottom = mesh.vertices.size();
        mesh.vertices.push_back({0, -halfHeight, 0});
        for (int i = 0; i < radialSegments; i++)
        {
            int next = (i + 1) % (radialSegments + 1);
            mesh.indices.push_back(centerTop);
            mesh.indices.push_back(next * 2);
            mesh.indices.push_back(i * 2);
            mesh.indices.push_back(centerBottom);
            mesh.indices.push_back(i * 2 + 1);
            mesh.indices.push_back(next * 2 + 1);
        }
        calcAABB(mesh);
        mesh.collisionShape = 1;
        mesh.collisionRadius = fmax(radiusTop, radiusBottom);
        int id = nextMeshId++;
        meshes[id] = mesh;
        auto obj = std::make_shared<ObjectValue>();
        obj->fields["_meshId"] = id;
        interp->lastValue = obj;
        return "{object}";
    }
};
class LoadOBJBuiltin : public BuiltinFunction
{
public:
    //@desc Load 3D mesh from OBJ file
    std::string getName() const override { return "loadOBJ"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 1)
            throw std::runtime_error("loadOBJ(filepath)");
        std::string filepath = std::get<std::string>(interp->evaluate(node->args[0].get()));
        
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(filepath, 
            aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);
        
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
            throw std::runtime_error("Failed to load OBJ: " + std::string(importer.GetErrorString()));
        
        Mesh mesh;
        std::string dir = filepath.substr(0, filepath.find_last_of("/\\") + 1);
        
        // Process all meshes
        for (unsigned int m = 0; m < scene->mNumMeshes; m++)
        {
            aiMesh* aimesh = scene->mMeshes[m];
            unsigned int baseVertex = mesh.vertices.size();
            
            // Vertices and UVs
            for (unsigned int i = 0; i < aimesh->mNumVertices; i++)
            {
                mesh.vertices.push_back({aimesh->mVertices[i].x, aimesh->mVertices[i].y, aimesh->mVertices[i].z});
                if (aimesh->mTextureCoords[0])
                    mesh.uvs.push_back({aimesh->mTextureCoords[0][i].x, aimesh->mTextureCoords[0][i].y});
            }
            
            // Faces
            std::string matName = "";
            if (aimesh->mMaterialIndex < scene->mNumMaterials)
            {
                aiMaterial* mat = scene->mMaterials[aimesh->mMaterialIndex];
                aiString name;
                mat->Get(AI_MATKEY_NAME, name);
                matName = name.C_Str();
            }
            
            for (unsigned int i = 0; i < aimesh->mNumFaces; i++)
            {
                aiFace face = aimesh->mFaces[i];
                for (unsigned int j = 0; j < face.mNumIndices; j++)
                {
                    mesh.indices.push_back(baseVertex + face.mIndices[j]);
                    if (aimesh->mTextureCoords[0])
                        mesh.uvIndices.push_back(baseVertex + face.mIndices[j]);
                }
                mesh.faceMaterials.push_back(matName);
            }
        }
        
        // Process materials
        for (unsigned int i = 0; i < scene->mNumMaterials; i++)
        {
            aiMaterial* mat = scene->mMaterials[i];
            aiString name;
            mat->Get(AI_MATKEY_NAME, name);
            std::string matName = name.C_Str();
            
            // Diffuse color
            aiColor3D color(1.f, 1.f, 1.f);
            mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
            mesh.materialColors[matName] = {(Uint8)(color.r * 255), (Uint8)(color.g * 255), (Uint8)(color.b * 255), 255};
            
            // Diffuse texture
            if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0)
            {
                aiString texPath;
                mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);
                std::string fullPath = dir + texPath.C_Str();
                TextureOptions texOpts;
                texOpts.path = fullPath;
                pendingTextures[nextMeshId].push_back({matName, texOpts});
            }
        }
        
        calcAABB(mesh);
        int id = nextMeshId++;
        meshes[id] = mesh;
        
        auto obj = std::make_shared<ObjectValue>();
        obj->fields["_meshId"] = id;
        interp->lastValue = obj;
        return "{object}";
    }
};

class PerspectiveCameraBuiltin : public BuiltinFunction
{
public:
    //@desc Create perspective camera with field of view
    std::string getName() const override { return "PerspectiveCamera"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        Camera cam;
        if (node->args.size() >= 1)
        {
            auto v = interp->evaluate(node->args[0].get());
            cam.fov = std::holds_alternative<int>(v) ? std::get<int>(v) : std::get<float>(v);
        }
        int id = nextCameraId++;
        cameras[id] = cam;
        auto obj = std::make_shared<ObjectValue>();
        obj->fields["_cameraId"] = id;
        interp->lastValue = obj;
        return "{object}";
    }
};
class SetPositionBuiltin : public BuiltinFunction
{
public:
    //@desc Set object position in 3D space
    //@parent mesh,camera,light
    std::string getName() const override { return "setPosition"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 3)
            throw std::runtime_error("setPosition(x, y, z)");
        auto v0 = interp->evaluate(node->args[0].get());
        auto v1 = interp->evaluate(node->args[1].get());
        auto v2 = interp->evaluate(node->args[2].get());
        float x = std::holds_alternative<int>(v0) ? std::get<int>(v0) : std::get<float>(v0);
        float y = std::holds_alternative<int>(v1) ? std::get<int>(v1) : std::get<float>(v1);
        float z = std::holds_alternative<int>(v2) ? std::get<int>(v2) : std::get<float>(v2);
        if (node->callee && dynamic_cast<FieldAccess *>(node->callee.get()))
        {
            auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
            Value objVal = interp->evaluate(fa->object.get());
            auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
            if (obj->fields.count("_meshId"))
                meshes[std::get<int>(obj->fields["_meshId"])].position = {x, y, z};
            else if (obj->fields.count("_cameraId"))
                cameras[std::get<int>(obj->fields["_cameraId"])].position = {x, y, z};
            else if (obj->fields.count("_lightId"))
                lights[std::get<int>(obj->fields["_lightId"])].position = {x, y, z};
        }
        return "";
    }
};
class SetRotationBuiltin : public BuiltinFunction
{
public:
    //@desc Set object rotation in radians
    //@parent mesh
    std::string getName() const override { return "setRotation"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 3)
            throw std::runtime_error("setRotation(x, y, z)");
        auto v0 = interp->evaluate(node->args[0].get());
        auto v1 = interp->evaluate(node->args[1].get());
        auto v2 = interp->evaluate(node->args[2].get());
        float x = std::holds_alternative<int>(v0) ? std::get<int>(v0) : std::get<float>(v0);
        float y = std::holds_alternative<int>(v1) ? std::get<int>(v1) : std::get<float>(v1);
        float z = std::holds_alternative<int>(v2) ? std::get<int>(v2) : std::get<float>(v2);
        if (node->callee && dynamic_cast<FieldAccess *>(node->callee.get()))
        {
            auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
            Value objVal = interp->evaluate(fa->object.get());
            auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
            if (obj->fields.count("_meshId"))
                meshes[std::get<int>(obj->fields["_meshId"])].rotation = {x, y, z};
        }
        return "";
    }
};
class SetScaleBuiltin : public BuiltinFunction
{
public:
    //@desc Set object scale factors
    //@parent mesh
    std::string getName() const override { return "setScale"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 3)
            throw std::runtime_error("setScale(x, y, z)");
        auto v0 = interp->evaluate(node->args[0].get());
        auto v1 = interp->evaluate(node->args[1].get());
        auto v2 = interp->evaluate(node->args[2].get());
        float x = std::holds_alternative<int>(v0) ? std::get<int>(v0) : std::get<float>(v0);
        float y = std::holds_alternative<int>(v1) ? std::get<int>(v1) : std::get<float>(v1);
        float z = std::holds_alternative<int>(v2) ? std::get<int>(v2) : std::get<float>(v2);
        if (node->callee && dynamic_cast<FieldAccess *>(node->callee.get()))
        {
            auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
            Value objVal = interp->evaluate(fa->object.get());
            auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
            if (obj->fields.count("_meshId"))
            {
                Mesh &mesh = meshes[std::get<int>(obj->fields["_meshId"])];
                mesh.scale = {x, y, z};
                calcAABB(mesh);
            }
        }
        return "";
    }
};
class LookAtBuiltin : public BuiltinFunction
{
public:
    //@desc Point camera at target coordinates
    //@parent camera
    std::string getName() const override { return "lookAt"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 3)
            throw std::runtime_error("lookAt(x, y, z)");
        if (node->callee && dynamic_cast<FieldAccess *>(node->callee.get()))
        {
            auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
            Value objVal = interp->evaluate(fa->object.get());
            auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
            if (obj->fields.count("_cameraId"))
            {
                auto v0 = interp->evaluate(node->args[0].get());
                auto v1 = interp->evaluate(node->args[1].get());
                auto v2 = interp->evaluate(node->args[2].get());
                float x = std::holds_alternative<int>(v0) ? std::get<int>(v0) : std::get<float>(v0);
                float y = std::holds_alternative<int>(v1) ? std::get<int>(v1) : std::get<float>(v1);
                float z = std::holds_alternative<int>(v2) ? std::get<int>(v2) : std::get<float>(v2);
                int camId = std::holds_alternative<int>(obj->fields.at("_cameraId")) ? std::get<int>(obj->fields.at("_cameraId")) : (int)std::get<float>(obj->fields.at("_cameraId"));
                cameras[camId].target = {x, y, z};
            }
        }
        return "";
    }
};
class EnableDevModeBuiltin : public BuiltinFunction
{
public:
    //@desc Enable development camera controls
    std::string getName() const override { return "enableDevMode"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 1)
            throw std::runtime_error("enableDevMode(camera)");
        Value camVal = interp->evaluate(node->args[0].get());
        auto camObj = std::get<std::shared_ptr<ObjectValue>>(camVal);
        devCameraId = std::get<int>(camObj->fields["_cameraId"]);
        devMode = true;
        Camera &cam = cameras[devCameraId];
        cam.distance = sqrt(cam.position.x * cam.position.x + cam.position.y * cam.position.y + cam.position.z * cam.position.z);
        cam.yaw = atan2(cam.position.x, cam.position.z);
        cam.pitch = atan2(cam.position.y, sqrt(cam.position.x * cam.position.x + cam.position.z * cam.position.z));
        return "";
    }
};
class UpdateDevCameraBuiltin : public BuiltinFunction
{
public:
    //@desc Update development camera position
    std::string getName() const override { return "updateDevCamera"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (!devMode || devCameraId < 0)
            return "";
        Camera &cam = cameras[devCameraId];
        cam.position.x = cam.target.x + cam.distance * sin(cam.yaw) * cos(cam.pitch);
        cam.position.y = cam.target.y + cam.distance * sin(cam.pitch);
        cam.position.z = cam.target.z + cam.distance * cos(cam.yaw) * cos(cam.pitch);
        return "";
    }
};
class HandleDevInputBuiltin : public BuiltinFunction
{
public:
    //@desc Process development mode input events
    std::string getName() const override { return "handleDevInput"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                interp->lastValue = 0;
                return "0";
            }
            if (devMode && devCameraId >= 0)
            {
                Camera &cam = cameras[devCameraId];
                if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
                {
                    mouseDown = true;
                    lastMouseX = event.button.x;
                    lastMouseY = event.button.y;
                }
                if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
                    mouseDown = false;
                if (event.type == SDL_MOUSEMOTION && mouseDown)
                {
                    int dx = event.motion.x - lastMouseX;
                    int dy = event.motion.y - lastMouseY;
                    cam.yaw += dx * 0.005f;
                    cam.pitch -= dy * 0.005f;
                    cam.pitch = fmax(-1.5f, fmin(1.5f, cam.pitch));
                    lastMouseX = event.motion.x;
                    lastMouseY = event.motion.y;
                }
                if (event.type == SDL_MOUSEWHEEL)
                {
                    cam.distance -= event.wheel.y * 0.5f;
                    cam.distance = fmax(1.0f, fmin(50.0f, cam.distance));
                }
            }
        }
        if (devMode && devCameraId >= 0)
        {
            Camera &cam = cameras[devCameraId];
            const Uint8 *keys = SDL_GetKeyboardState(nullptr);
            if (keys[SDL_SCANCODE_W])
                cam.target.z -= 0.1f;
            if (keys[SDL_SCANCODE_S])
                cam.target.z += 0.1f;
            if (keys[SDL_SCANCODE_A])
                cam.target.x -= 0.1f;
            if (keys[SDL_SCANCODE_D])
                cam.target.x += 0.1f;
            if (keys[SDL_SCANCODE_Q])
                cam.target.y -= 0.1f;
            if (keys[SDL_SCANCODE_E])
                cam.target.y += 0.1f;
        }
        SDL_Delay(16);
        interp->lastValue = 1;
        return "1";
    }
};
class PointLightBuiltin : public BuiltinFunction
{
public:
    //@desc Create point light with color and intensity
    std::string getName() const override { return "PointLight"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        Light light;
        if (node->args.size() >= 1)
        {
            std::string color = std::get<std::string>(interp->evaluate(node->args[0].get()));
            if (color[0] == '#' && color.length() == 7)
            {
                light.color.r = std::stoi(color.substr(1, 2), nullptr, 16);
                light.color.g = std::stoi(color.substr(3, 2), nullptr, 16);
                light.color.b = std::stoi(color.substr(5, 2), nullptr, 16);
            }
        }
        if (node->args.size() >= 2)
        {
            auto v = interp->evaluate(node->args[1].get());
            light.intensity = std::holds_alternative<int>(v) ? std::get<int>(v) : std::get<float>(v);
        }
        int id = nextLightId++;
        lights[id] = light;
        auto obj = std::make_shared<ObjectValue>();
        obj->fields["_lightId"] = id;
        interp->lastValue = obj;
        return "{object}";
    }
};
class AmbientLightBuiltin : public BuiltinFunction
{
public:
    //@desc Create ambient light with color and intensity
    std::string getName() const override { return "AmbientLight"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        Light light;
        light.intensity = 0.5f;
        if (node->args.size() >= 1)
        {
            std::string color = std::get<std::string>(interp->evaluate(node->args[0].get()));
            if (color[0] == '#' && color.length() == 7)
            {
                light.color.r = std::stoi(color.substr(1, 2), nullptr, 16);
                light.color.g = std::stoi(color.substr(3, 2), nullptr, 16);
                light.color.b = std::stoi(color.substr(5, 2), nullptr, 16);
            }
        }
        if (node->args.size() >= 2)
        {
            auto v = interp->evaluate(node->args[1].get());
            light.intensity = std::holds_alternative<int>(v) ? std::get<int>(v) : std::get<float>(v);
        }
        int id = nextLightId++;
        lights[id] = light;
        auto obj = std::make_shared<ObjectValue>();
        obj->fields["_lightId"] = id;
        interp->lastValue = obj;
        return "{object}";
    }
};
class DirectionalLightBuiltin : public BuiltinFunction
{
public:
    //@desc Create directional light with color and intensity
    std::string getName() const override { return "DirectionalLight"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        Light light;
        light.position = {0, 1, 0};
        if (node->args.size() >= 1)
        {
            std::string color = std::get<std::string>(interp->evaluate(node->args[0].get()));
            if (color[0] == '#' && color.length() == 7)
            {
                light.color.r = std::stoi(color.substr(1, 2), nullptr, 16);
                light.color.g = std::stoi(color.substr(3, 2), nullptr, 16);
                light.color.b = std::stoi(color.substr(5, 2), nullptr, 16);
            }
        }
        if (node->args.size() >= 2)
        {
            auto v = interp->evaluate(node->args[1].get());
            light.intensity = std::holds_alternative<int>(v) ? std::get<int>(v) : std::get<float>(v);
        }
        int id = nextLightId++;
        lights[id] = light;
        auto obj = std::make_shared<ObjectValue>();
        obj->fields["_lightId"] = id;
        interp->lastValue = obj;
        return "{object}";
    }
};
class SetColorBuiltin : public BuiltinFunction
{
public:
    //@desc Set mesh color using hex string
    //@parent mesh
    std::string getName() const override { return "setColor"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 1)
            throw std::runtime_error("setColor(color)");
        std::string color = std::get<std::string>(interp->evaluate(node->args[0].get()));
        if (node->callee && dynamic_cast<FieldAccess *>(node->callee.get()))
        {
            auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
            Value objVal = interp->evaluate(fa->object.get());
            auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
            if (obj->fields.count("_meshId") && color[0] == '#' && color.length() == 7)
            {
                auto &mesh = meshes[std::get<int>(obj->fields["_meshId"])];
                mesh.color.r = std::stoi(color.substr(1, 2), nullptr, 16);
                mesh.color.g = std::stoi(color.substr(3, 2), nullptr, 16);
                mesh.color.b = std::stoi(color.substr(5, 2), nullptr, 16);
            }
        }
        return "";
    }
};
class SetMetallicBuiltin : public BuiltinFunction
{
public:
    //@desc Set mesh metallic property for reflections
    //@parent mesh
    std::string getName() const override { return "setMetallic"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 1)
            throw std::runtime_error("setMetallic(value)");
        auto v = interp->evaluate(node->args[0].get());
        float metallic = std::holds_alternative<int>(v) ? std::get<int>(v) : std::get<float>(v);
        if (node->callee && dynamic_cast<FieldAccess *>(node->callee.get()))
        {
            auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
            Value objVal = interp->evaluate(fa->object.get());
            auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
            if (obj->fields.count("_meshId"))
                meshes[std::get<int>(obj->fields["_meshId"])].metallic = metallic;
        }
        return "";
    }
};
class RenderSceneBuiltin : public BuiltinFunction
{
public:
    //@desc Render scene with camera to canvas
    //@parent canvas
    std::string getName() const override { return "render"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() == 0)
        {
            if (!node->callee)
                throw std::runtime_error("render() must be called on canvas");
            if (auto fa = dynamic_cast<FieldAccess *>(node->callee.get()))
            {
                Value canvasVal = interp->evaluate(fa->object.get());
                auto canvas = std::get<std::shared_ptr<ObjectValue>>(canvasVal);
                int id = std::get<int>(canvas->fields["_id"]);
                auto ctx = canvases[id];
                if (ctx->renderer)
                {
                    SDL_RenderPresent(ctx->renderer);
                }
            }
            return "";
        }
        if (node->args.size() != 2)
            return "";
        if (!node->callee)
            throw std::runtime_error("render must be called on canvas");
        auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
        Value canvasVal = interp->evaluate(fa->object.get());
        auto canvas = std::get<std::shared_ptr<ObjectValue>>(canvasVal);
        int canvasId = std::get<int>(canvas->fields["_id"]);
        auto ctx = canvases[canvasId];
        Value sceneVal = interp->evaluate(node->args[0].get());
        auto sceneObj = std::get<std::shared_ptr<ObjectValue>>(sceneVal);
        int sceneId = std::get<int>(sceneObj->fields["_sceneId"]);
        Scene &scene = scenes[sceneId];
        Value camVal = interp->evaluate(node->args[1].get());
        auto camObj = std::get<std::shared_ptr<ObjectValue>>(camVal);
        int camId = std::get<int>(camObj->fields["_cameraId"]);
        Camera &cam = cameras[camId];
        std::vector<std::tuple<float, int, size_t, int, Vec3, Vec3, Vec3, Vec3, Vec3, Vec3>> sortedTris;
        int sceneOrder = 0;
        for (int meshId : scene.meshIds)
        {
            Mesh &mesh = meshes[meshId];
            if (!mesh.visible)
            {
                sceneOrder++;
                continue;
            }
            for (size_t i = 0; i < mesh.indices.size(); i += 3)
            {
                Vec3 worldV[3];
                for (int j = 0; j < 3; j++)
                {
                    Vec3 vert = mesh.vertices[mesh.indices[i + j]];
                    vert.x *= mesh.scale.x;
                    vert.y *= mesh.scale.y;
                    vert.z *= mesh.scale.z;
                    vert = rotateX(vert, mesh.rotation.x);
                    vert = rotateY(vert, mesh.rotation.y);
                    vert = rotateZ(vert, mesh.rotation.z);
                    vert.x += mesh.position.x;
                    vert.y += mesh.position.y;
                    vert.z += mesh.position.z;
                    worldV[j] = vert;
                }
                Vec3 worldCenter = {(worldV[0].x + worldV[1].x + worldV[2].x) / 3, (worldV[0].y + worldV[1].y + worldV[2].y) / 3, (worldV[0].z + worldV[1].z + worldV[2].z) / 3};
                Vec3 e1 = {worldV[1].x - worldV[0].x, worldV[1].y - worldV[0].y, worldV[1].z - worldV[0].z};
                Vec3 e2 = {worldV[2].x - worldV[0].x, worldV[2].y - worldV[0].y, worldV[2].z - worldV[0].z};
                Vec3 n = {e1.y * e2.z - e1.z * e2.y, e1.z * e2.x - e1.x * e2.z, e1.x * e2.y - e1.y * e2.x};
                float len = sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
                if (len > 0)
                {
                    n.x /= len;
                    n.y /= len;
                    n.z /= len;
                }
                Vec3 viewDir = {cam.position.x - worldCenter.x, cam.position.y - worldCenter.y, cam.position.z - worldCenter.z};
                float viewLen = sqrt(viewDir.x * viewDir.x + viewDir.y * viewDir.y + viewDir.z * viewDir.z);
                if (viewLen > 0)
                {
                    viewDir.x /= viewLen;
                    viewDir.y /= viewLen;
                    viewDir.z /= viewLen;
                }
                float dot = n.x * viewDir.x + n.y * viewDir.y + n.z * viewDir.z;
                if (!mesh.doubleSided && dot < -0.01f)
                    continue;
                Vec3 v[3];
                for (int j = 0; j < 3; j++)
                {
                    v[j] = transformToCamera(worldV[j], cam);
                }
                float avgZ = (v[0].z + v[1].z + v[2].z) / 3.0f;
                sortedTris.push_back({avgZ, meshId, i, sceneOrder, v[0], v[1], v[2], worldV[0], worldV[1], worldV[2]});
            }
            sceneOrder++;
        }
        if (!ctx->glContext)
        {
            if (ctx->renderer)
            {
                SDL_DestroyRenderer(ctx->renderer);
                ctx->renderer = nullptr;
            }
            SDL_DestroyWindow(ctx->window);

            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
            SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
            SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
            if (scene.enableMSAA)
            {
                SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
                SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, scene.msaaSamples);
            }

            ctx->window = SDL_CreateWindow("Axolotl 3D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ctx->width, ctx->height, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
            if (!ctx->window)
                return "";

            ctx->glContext = SDL_GL_CreateContext(ctx->window);
            if (!ctx->glContext)
                return "";
            SDL_GL_SetSwapInterval(1);
            ctx->useOpenGL = true;
            ctx->is2D = false;
        }
        SDL_GL_MakeCurrent(ctx->window, ctx->glContext);
        
        // Initialize glGenerateMipmap if not already done
        if (!glGenerateMipmapPtr)
        {
            glGenerateMipmapPtr = (PFNGLGENERATEMIPMAPPROC)SDL_GL_GetProcAddress("glGenerateMipmap");
        }
        
        // Load textures for materials
        if (!pendingTextures.empty())
        {
            for (auto it = pendingTextures.begin(); it != pendingTextures.end(); )
            {
                int meshId = it->first;
                auto &textures = it->second;
                
                if (meshes.find(meshId) != meshes.end() && !textures.empty())
                {
                    Mesh &mesh = meshes[meshId];
                    
                    // Load each material texture into materialTextures map
                    for (const auto &[matName, texOpts] : textures)
                    {
                        SDL_Surface *surface = IMG_Load(texOpts.path.c_str());
                        if (surface)
                        {
                            SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
                            SDL_FreeSurface(surface);
                            if (converted)
                            {
                                GLuint texId;
                                glGenTextures(1, &texId);
                                glBindTexture(GL_TEXTURE_2D, texId);
                                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, converted->w, converted->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, converted->pixels);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texOpts.clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texOpts.clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
                                SDL_FreeSurface(converted);
                                mesh.materialTextures[matName] = texId;
                                mesh.materialTexOpts[matName] = texOpts;
                                std::cout << "[Texture] Loaded [" << matName << "] ID: " << texId << std::endl;
                            }
                        }
                    }
                }
                it = pendingTextures.erase(it);
            }
        }
        
        glViewport(0, 0, ctx->width, ctx->height);
        if (scene.enableDepthTest)
        {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
        }
        if (scene.enableMSAA)
            glEnable(GL_MULTISAMPLE);
        else
            glDisable(GL_MULTISAMPLE);
        if (scene.enableSmoothing)
        {
            glEnable(GL_LINE_SMOOTH);
            glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
            glEnable(GL_POLYGON_SMOOTH);
            glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
        }
        else
        {
            glDisable(GL_LINE_SMOOTH);
            glDisable(GL_POLYGON_SMOOTH);
        }
        
        // Setup OpenGL lighting
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.01f);
        
        GLfloat ambient[] = {scene.ambientLight.x, scene.ambientLight.y, scene.ambientLight.z, 1.0f};
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
        
        if (!scene.lightIds.empty())
        {
            Light &l = lights[scene.lightIds[0]];
            GLfloat lightPos[] = {l.position.x, l.position.y, l.position.z, 1.0f};
            GLfloat lightColor[] = {l.color.r / 255.0f * l.intensity, l.color.g / 255.0f * l.intensity, l.color.b / 255.0f * l.intensity, 1.0f};
            glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
            glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);
            glLightfv(GL_LIGHT0, GL_SPECULAR, lightColor);
        }
        
        glClearColor(scene.background.r / 255.0f, scene.background.g / 255.0f, scene.background.b / 255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        float aspect = (float)ctx->width / (float)ctx->height;
        float fovRad = cam.fov * M_PI / 180.0f;
        float top = tan(fovRad / 2.0f) * 0.1f;
        float right = top * aspect;
        glFrustum(-right, right, -top, top, 0.1f, 1000.0f);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        Vec3 forward = {cam.target.x - cam.position.x, cam.target.y - cam.position.y, cam.target.z - cam.position.z};
        float flen = sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
        if (flen > 0.001f)
        {
            forward.x /= flen;
            forward.y /= flen;
            forward.z /= flen;
        }
        Vec3 worldUp = {0, 1, 0};
        Vec3 r = {worldUp.y * forward.z - worldUp.z * forward.y, worldUp.z * forward.x - worldUp.x * forward.z, worldUp.x * forward.y - worldUp.y * forward.x};
        float rlen = sqrt(r.x * r.x + r.y * r.y + r.z * r.z);
        if (rlen > 0.001f)
        {
            r.x /= rlen;
            r.y /= rlen;
            r.z /= rlen;
        }
        Vec3 up = {forward.y * r.z - forward.z * r.y, forward.z * r.x - forward.x * r.z, forward.x * r.y - forward.y * r.x};
        float view[16] = {
            r.x, up.x, -forward.x, 0,
            r.y, up.y, -forward.y, 0,
            r.z, up.z, -forward.z, 0,
            0, 0, 0, 1};
        glMultMatrixf(view);
        glTranslatef(-cam.position.x, -cam.position.y, -cam.position.z);

        int lastMeshId = -1;
        std::string lastMaterial = "";
        glBegin(GL_TRIANGLES);
        for (auto &[depth, meshId, i, order, v0, v1, v2, w0, w1, w2] : sortedTris)
        {
            Mesh &mesh = meshes[meshId];
            if (!mesh.visible)
                continue;
            
            // Get material for this face
            size_t faceIdx = i / 3;
            std::string currentMat = faceIdx < mesh.faceMaterials.size() ? mesh.faceMaterials[faceIdx] : "";
            
            // Switch texture if material changed
            if (meshId != lastMeshId || currentMat != lastMaterial)
            {
                glEnd();
                
                if (!currentMat.empty() && mesh.materialTextures.count(currentMat))
                {
                    glEnable(GL_TEXTURE_2D);
                    glBindTexture(GL_TEXTURE_2D, mesh.materialTextures[currentMat]);
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                }
                else if (mesh.textureId)
                {
                    glEnable(GL_TEXTURE_2D);
                    glBindTexture(GL_TEXTURE_2D, mesh.textureId);
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                }
                else
                {
                    glDisable(GL_TEXTURE_2D);
                }
                
                lastMeshId = meshId;
                lastMaterial = currentMat;
                glBegin(GL_TRIANGLES);
            }
            
            Vec3 worldV[3] = {w0, w1, w2};
            Vec3 worldCenter = {(worldV[0].x + worldV[1].x + worldV[2].x) / 3, (worldV[0].y + worldV[1].y + worldV[2].y) / 3, (worldV[0].z + worldV[1].z + worldV[2].z) / 3};
            Vec3 e1 = {worldV[1].x - worldV[0].x, worldV[1].y - worldV[0].y, worldV[1].z - worldV[0].z};
            Vec3 e2 = {worldV[2].x - worldV[0].x, worldV[2].y - worldV[0].y, worldV[2].z - worldV[0].z};
            Vec3 n = {e1.y * e2.z - e1.z * e2.y, e1.z * e2.x - e1.x * e2.z, e1.x * e2.y - e1.y * e2.x};
            float nlen = sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
            if (nlen > 0)
            {
                n.x /= nlen;
                n.y /= nlen;
                n.z /= nlen;
            }
            Vec3 viewDir = {cam.position.x - worldCenter.x, cam.position.y - worldCenter.y, cam.position.z - worldCenter.z};
            float viewLen = sqrt(viewDir.x * viewDir.x + viewDir.y * viewDir.y + viewDir.z * viewDir.z);
            if (viewLen > 0)
            {
                viewDir.x /= viewLen;
                viewDir.y /= viewLen;
                viewDir.z /= viewLen;
            }
            if (mesh.doubleSided)
            {
                float dot = n.x * viewDir.x + n.y * viewDir.y + n.z * viewDir.z;
                if (dot < 0)
                {
                    n.x = -n.x;
                    n.y = -n.y;
                    n.z = -n.z;
                }
            }
            float diffuse = scene.ambientLight.x;
            float specular = 0.0f;
            float shadow = 1.0f;
            if (!scene.lightIds.empty())
            {
                Light &l = lights[scene.lightIds[0]];
                Vec3 lightDir = {l.position.x - worldCenter.x, l.position.y - worldCenter.y, l.position.z - worldCenter.z};
                float dist = sqrt(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
                if (dist > 0)
                {
                    lightDir.x /= dist;
                    lightDir.y /= dist;
                    lightDir.z /= dist;
                }
                float diff = fmax(0.0f, n.x * lightDir.x + n.y * lightDir.y + n.z * lightDir.z);
                if (scene.enableShadows && diff > 0.01f)
                {
                    Vec3 shadowRay = {worldCenter.x + n.x * 0.01f, worldCenter.y + n.y * 0.01f, worldCenter.z + n.z * 0.01f};
                    for (int shadowMeshId : scene.meshIds)
                    {
                        if (shadowMeshId == meshId)
                            continue;
                        Mesh &sm = meshes[shadowMeshId];
                        if (!sm.visible)
                            continue;
                        Vec3 sc = {(sm.aabbMin.x + sm.aabbMax.x) * 0.5f, (sm.aabbMin.y + sm.aabbMax.y) * 0.5f, (sm.aabbMin.z + sm.aabbMax.z) * 0.5f};
                        Vec3 se = {(sm.aabbMax.x - sm.aabbMin.x) * 0.5f * sm.scale.x, (sm.aabbMax.y - sm.aabbMin.y) * 0.5f * sm.scale.y, (sm.aabbMax.z - sm.aabbMin.z) * 0.5f * sm.scale.z};
                        Vec3 smin = {sm.position.x + sc.x - se.x, sm.position.y + sc.y - se.y, sm.position.z + sc.z - se.z};
                        Vec3 smax = {sm.position.x + sc.x + se.x, sm.position.y + sc.y + se.y, sm.position.z + sc.z + se.z};
                        float tmin = 0, tmax = dist;
                        for (int axis = 0; axis < 3; axis++)
                        {
                            float o = axis == 0 ? shadowRay.x : (axis == 1 ? shadowRay.y : shadowRay.z);
                            float d = axis == 0 ? lightDir.x : (axis == 1 ? lightDir.y : lightDir.z);
                            float bmin = axis == 0 ? smin.x : (axis == 1 ? smin.y : smin.z);
                            float bmax = axis == 0 ? smax.x : (axis == 1 ? smax.y : smax.z);
                            if (fabs(d) > 0.0001f)
                            {
                                float t1 = (bmin - o) / d, t2 = (bmax - o) / d;
                                if (t1 > t2)
                                {
                                    float tmp = t1;
                                    t1 = t2;
                                    t2 = tmp;
                                }
                                tmin = fmax(tmin, t1);
                                tmax = fmin(tmax, t2);
                                if (tmin > tmax)
                                    break;
                            }
                        }
                        if (tmin <= tmax && tmin < dist)
                        {
                            shadow = 1.0f - scene.shadowIntensity;
                            break;
                        }
                    }
                }
                diffuse += diff * l.intensity * (l.color.r / 255.0f) * shadow;
                if (mesh.metallic > 0.01f)
                {
                    Vec3 reflectDir = {lightDir.x - 2.0f * diff * n.x, lightDir.y - 2.0f * diff * n.y, lightDir.z - 2.0f * diff * n.z};
                    float spec = fmax(0.0f, reflectDir.x * viewDir.x + reflectDir.y * viewDir.y + reflectDir.z * viewDir.z);
                    specular = pow(spec, 32.0f) * mesh.metallic * l.intensity * shadow;
                }
            }
            // Get material color
            SDL_Color matColor = mesh.color;
            if (!currentMat.empty() && mesh.materialColors.count(currentMat))
            {
                matColor = mesh.materialColors[currentMat];
            }
            
            // Apply material color with lighting (texture will be modulated with this)
            float r = (matColor.r / 255.0f) * (diffuse + specular);
            float g = (matColor.g / 255.0f) * (diffuse + specular);
            float b = (matColor.b / 255.0f) * (diffuse + specular);
            glColor4f(r, g, b, matColor.a / 255.0f);
            glNormal3f(n.x, n.y, n.z);
            
            // Apply UV coordinates if available
            int uvBaseIdx = (i / 3) * 3;
            bool hasTexture = (!currentMat.empty() && mesh.materialTextures.count(currentMat)) || mesh.textureId;
            
            // Get texture options for this material
            TextureOptions texOpts;
            if (!currentMat.empty() && mesh.materialTexOpts.count(currentMat))
            {
                texOpts = mesh.materialTexOpts[currentMat];
            }
            else
            {
                texOpts.scale = {1, 1};
                texOpts.offset = {0, 0};
            }
            
            if (hasTexture && !mesh.uvs.empty() && !mesh.uvIndices.empty())
            {
                if (uvBaseIdx < (int)mesh.uvIndices.size() && mesh.uvIndices[uvBaseIdx] < (int)mesh.uvs.size())
                {
                    Vec2 uv0 = mesh.uvs[mesh.uvIndices[uvBaseIdx]];
                    glTexCoord2f(uv0.x * texOpts.scale.x + texOpts.offset.x, 1.0f - (uv0.y * texOpts.scale.y + texOpts.offset.y));
                }
            }
            glVertex3f(worldV[0].x, worldV[0].y, worldV[0].z);
            
            if (hasTexture && !mesh.uvs.empty() && !mesh.uvIndices.empty())
            {
                if (uvBaseIdx + 1 < (int)mesh.uvIndices.size() && mesh.uvIndices[uvBaseIdx + 1] < (int)mesh.uvs.size())
                {
                    Vec2 uv1 = mesh.uvs[mesh.uvIndices[uvBaseIdx + 1]];
                    glTexCoord2f(uv1.x * texOpts.scale.x + texOpts.offset.x, 1.0f - (uv1.y * texOpts.scale.y + texOpts.offset.y));
                }
            }
            glVertex3f(worldV[1].x, worldV[1].y, worldV[1].z);
            
            if (hasTexture && !mesh.uvs.empty() && !mesh.uvIndices.empty())
            {
                if (uvBaseIdx + 2 < (int)mesh.uvIndices.size() && mesh.uvIndices[uvBaseIdx + 2] < (int)mesh.uvs.size())
                {
                    Vec2 uv2 = mesh.uvs[mesh.uvIndices[uvBaseIdx + 2]];
                    glTexCoord2f(uv2.x * texOpts.scale.x + texOpts.offset.x, 1.0f - (uv2.y * texOpts.scale.y + texOpts.offset.y));
                }
            }
            glVertex3f(worldV[2].x, worldV[2].y, worldV[2].z);
        }
        glEnd();
        glDisable(GL_TEXTURE_2D);
        SDL_GL_SwapWindow(ctx->window);
        return "";
    }
};
class IsCollidingBuiltin : public BuiltinFunction
{
public:
    //@desc Check if meshes are colliding
    std::string getName() const override { return "isColliding"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        try
        {
            if (node->args.size() < 2)
                throw std::runtime_error("isColliding(mesh1, mesh2, ...)");
            std::vector<int> meshIds;
            for (auto &arg : node->args)
            {
                Value v = interp->evaluate(arg.get());
                if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(v))
                {
                    interp->lastValue = 0;
                    return "0";
                }
                auto obj = std::get<std::shared_ptr<ObjectValue>>(v);
                if (obj && obj->fields.count("_meshId"))
                {
                    try
                    {
                        auto &field = obj->fields.at("_meshId");
                        if (std::holds_alternative<int>(field))
                        {
                            meshIds.push_back(std::get<int>(field));
                        }
                        else if (std::holds_alternative<float>(field))
                        {
                            meshIds.push_back((int)std::get<float>(field));
                        }
                    }
                    catch (...)
                    {
                    }
                }
            }
            if (meshIds.size() < 2)
            {
                interp->lastValue = 0;
                return "0";
            }
            for (size_t i = 0; i < meshIds.size(); i++)
            {
                for (size_t j = i + 1; j < meshIds.size(); j++)
                {
                    if (meshes.find(meshIds[i]) == meshes.end() || meshes.find(meshIds[j]) == meshes.end())
                        continue;
                    Mesh &m1 = meshes[meshIds[i]];
                    Mesh &m2 = meshes[meshIds[j]];
                    Vec3 c1 = {(m1.aabbMin.x + m1.aabbMax.x) * 0.5f, (m1.aabbMin.y + m1.aabbMax.y) * 0.5f, (m1.aabbMin.z + m1.aabbMax.z) * 0.5f};
                    Vec3 e1 = {(m1.aabbMax.x - m1.aabbMin.x) * 0.5f * m1.scale.x, (m1.aabbMax.y - m1.aabbMin.y) * 0.5f * m1.scale.y, (m1.aabbMax.z - m1.aabbMin.z) * 0.5f * m1.scale.z};
                    Vec3 c2 = {(m2.aabbMin.x + m2.aabbMax.x) * 0.5f, (m2.aabbMin.y + m2.aabbMax.y) * 0.5f, (m2.aabbMin.z + m2.aabbMax.z) * 0.5f};
                    Vec3 e2 = {(m2.aabbMax.x - m2.aabbMin.x) * 0.5f * m2.scale.x, (m2.aabbMax.y - m2.aabbMin.y) * 0.5f * m2.scale.y, (m2.aabbMax.z - m2.aabbMin.z) * 0.5f * m2.scale.z};
                    Vec3 min1 = {m1.position.x + c1.x - e1.x, m1.position.y + c1.y - e1.y, m1.position.z + c1.z - e1.z};
                    Vec3 max1 = {m1.position.x + c1.x + e1.x, m1.position.y + c1.y + e1.y, m1.position.z + c1.z + e1.z};
                    Vec3 min2 = {m2.position.x + c2.x - e2.x, m2.position.y + c2.y - e2.y, m2.position.z + c2.z - e2.z};
                    Vec3 max2 = {m2.position.x + c2.x + e2.x, m2.position.y + c2.y + e2.y, m2.position.z + c2.z + e2.z};
                    if (aabbIntersect(min1, max1, min2, max2))
                    {
                        interp->lastValue = 1;
                        return "1";
                    }
                }
            }
            interp->lastValue = 0;
            return "0";
        }
        catch (...)
        {
            interp->lastValue = 0;
            return "0";
        }
    }
};
REGISTER_BUILTIN(CreateSceneBuiltin)
REGISTER_BUILTIN(AddToSceneBuiltin)
REGISTER_BUILTIN(RemoveMeshBuiltin)
REGISTER_BUILTIN(BoxGeometryBuiltin)
REGISTER_BUILTIN(SphereGeometryBuiltin)
REGISTER_BUILTIN(PlaneGeometryBuiltin)
REGISTER_BUILTIN(TorusGeometryBuiltin)
REGISTER_BUILTIN(CylinderGeometryBuiltin)
REGISTER_BUILTIN(LoadOBJBuiltin)
REGISTER_BUILTIN(PerspectiveCameraBuiltin)
REGISTER_BUILTIN(SetPositionBuiltin)
REGISTER_BUILTIN(SetRotationBuiltin)
REGISTER_BUILTIN(SetScaleBuiltin)
REGISTER_BUILTIN(LookAtBuiltin)
REGISTER_BUILTIN(SetColorBuiltin)
REGISTER_BUILTIN(SetMetallicBuiltin)
REGISTER_BUILTIN(PointLightBuiltin)
REGISTER_BUILTIN(AmbientLightBuiltin)
REGISTER_BUILTIN(DirectionalLightBuiltin)
REGISTER_BUILTIN(EnableDevModeBuiltin)
REGISTER_BUILTIN(UpdateDevCameraBuiltin)
REGISTER_BUILTIN(HandleDevInputBuiltin)
REGISTER_BUILTIN(RenderSceneBuiltin)
REGISTER_BUILTIN(IsCollidingBuiltin)
class FollowTargetBuiltin : public BuiltinFunction
{
public:
    //@desc Make camera follow target mesh
    //@parent camera
    std::string getName() const override { return "followTarget"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 4)
            throw std::runtime_error("followTarget(camera, target, distance, height)");
        Value camVal = interp->evaluate(node->args[0].get());
        auto camObj = std::get<std::shared_ptr<ObjectValue>>(camVal);
        int camId = std::get<int>(camObj->fields["_cameraId"]);

        Value targetVal = interp->evaluate(node->args[1].get());
        auto targetObj = std::get<std::shared_ptr<ObjectValue>>(targetVal);
        int targetId = std::get<int>(targetObj->fields["_meshId"]);

        auto v2 = interp->evaluate(node->args[2].get());
        float distance = std::holds_alternative<int>(v2) ? std::get<int>(v2) : std::get<float>(v2);

        auto v3 = interp->evaluate(node->args[3].get());
        float height = std::holds_alternative<int>(v3) ? std::get<int>(v3) : std::get<float>(v3);

        Camera &cam = cameras[camId];
        Mesh &target = meshes[targetId];

        float angle = target.rotation.y;
        cam.position.x = target.position.x - sin(angle) * distance;
        cam.position.y = target.position.y + height;
        cam.position.z = target.position.z - cos(angle) * distance;
        cam.target = {target.position.x, target.position.y + height * 0.2f, target.position.z};

        return "";
    }
};
REGISTER_BUILTIN(FollowTargetBuiltin)
class GetPositionBuiltin : public BuiltinFunction
{
public:
    //@desc Get mesh position as object with x, y, z
    //@parent mesh
    std::string getName() const override { return "getPosition"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (!node->callee)
            throw std::runtime_error("getPosition must be called on mesh");
        auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
        Value objVal = interp->evaluate(fa->object.get());
        auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);

        if (obj->fields.count("_meshId"))
        {
            int meshId = std::get<int>(obj->fields["_meshId"]);
            Mesh &mesh = meshes[meshId];
            auto result = std::make_shared<ObjectValue>();
            result->fields["x"] = mesh.position.x;
            result->fields["y"] = mesh.position.y;
            result->fields["z"] = mesh.position.z;
            interp->lastValue = result;
            return "{object}";
        }
        throw std::runtime_error("getPosition requires mesh object");
    }
};
REGISTER_BUILTIN(GetPositionBuiltin)
class MoveByBuiltin : public BuiltinFunction
{
public:
    //@desc Move mesh by relative offset
    //@parent mesh
    std::string getName() const override { return "moveBy"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 3)
            throw std::runtime_error("moveBy(x, y, z)");
        if (!node->callee)
            throw std::runtime_error("moveBy must be called on mesh");
        auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
        Value objVal = interp->evaluate(fa->object.get());
        auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);

        if (obj->fields.count("_meshId"))
        {
            int meshId = std::get<int>(obj->fields["_meshId"]);
            auto v0 = interp->evaluate(node->args[0].get());
            auto v1 = interp->evaluate(node->args[1].get());
            auto v2 = interp->evaluate(node->args[2].get());
            float x = std::holds_alternative<int>(v0) ? std::get<int>(v0) : std::get<float>(v0);
            float y = std::holds_alternative<int>(v1) ? std::get<int>(v1) : std::get<float>(v1);
            float z = std::holds_alternative<int>(v2) ? std::get<int>(v2) : std::get<float>(v2);

            Mesh &mesh = meshes[meshId];
            mesh.position.x += x;
            mesh.position.y += y;
            mesh.position.z += z;
        }
        return "";
    }
};
REGISTER_BUILTIN(MoveByBuiltin)
class DeformVerticesBuiltin : public BuiltinFunction
{
public:
    //@desc Deform mesh vertices with noise function
    //@parent mesh
    std::string getName() const override { return "deformVertices"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 3)
            throw std::runtime_error("deformVertices(amplitude, frequency, seed)");
        if (!node->callee)
            throw std::runtime_error("deformVertices must be called on mesh");
        auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
        Value objVal = interp->evaluate(fa->object.get());
        auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
        if (obj->fields.count("_meshId"))
        {
            int meshId = std::get<int>(obj->fields["_meshId"]);
            Mesh &mesh = meshes[meshId];
            auto v0 = interp->evaluate(node->args[0].get());
            auto v1 = interp->evaluate(node->args[1].get());
            auto v2 = interp->evaluate(node->args[2].get());
            float amplitude = std::holds_alternative<int>(v0) ? std::get<int>(v0) : std::get<float>(v0);
            float frequency = std::holds_alternative<int>(v1) ? std::get<int>(v1) : std::get<float>(v1);
            float seed = std::holds_alternative<int>(v2) ? std::get<int>(v2) : std::get<float>(v2);
            for (auto &v : mesh.vertices)
            {
                float noise = sin(v.x * frequency + seed) * cos(v.z * frequency + seed * 1.3f);
                v.y += noise * amplitude;
            }
            calcAABB(mesh);
        }
        return "";
    }
};
REGISTER_BUILTIN(DeformVerticesBuiltin)
class SetGraphicsBuiltin : public BuiltinFunction
{
public:
    //@desc Configure scene graphics settings
    std::string getName() const override { return "setGraphics"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 2)
            throw std::runtime_error("setGraphics(scene, settings)");
        Value sceneVal = interp->evaluate(node->args[0].get());
        auto sceneObj = std::get<std::shared_ptr<ObjectValue>>(sceneVal);
        int sceneId = std::get<int>(sceneObj->fields["_sceneId"]);
        Scene &scene = scenes[sceneId];
        Value settingsVal = interp->evaluate(node->args[1].get());
        auto settings = std::get<std::shared_ptr<ObjectValue>>(settingsVal);
        if (settings->fields.count("msaa"))
        {
            auto v = settings->fields["msaa"];
            scene.enableMSAA = std::holds_alternative<int>(v) ? std::get<int>(v) != 0 : std::get<float>(v) != 0.0f;
        }
        if (settings->fields.count("msaaSamples"))
        {
            auto v = settings->fields["msaaSamples"];
            scene.msaaSamples = std::holds_alternative<int>(v) ? std::get<int>(v) : (int)std::get<float>(v);
        }
        if (settings->fields.count("depthTest"))
        {
            auto v = settings->fields["depthTest"];
            scene.enableDepthTest = std::holds_alternative<int>(v) ? std::get<int>(v) != 0 : std::get<float>(v) != 0.0f;
        }
        if (settings->fields.count("smoothing"))
        {
            auto v = settings->fields["smoothing"];
            scene.enableSmoothing = std::holds_alternative<int>(v) ? std::get<int>(v) != 0 : std::get<float>(v) != 0.0f;
        }
        if (settings->fields.count("shadows"))
        {
            auto v = settings->fields["shadows"];
            scene.enableShadows = std::holds_alternative<int>(v) ? std::get<int>(v) != 0 : std::get<float>(v) != 0.0f;
        }
        if (settings->fields.count("shadowIntensity"))
        {
            auto v = settings->fields["shadowIntensity"];
            scene.shadowIntensity = std::holds_alternative<int>(v) ? std::get<int>(v) : std::get<float>(v);
        }
        if (settings->fields.count("fog"))
        {
            auto v = settings->fields["fog"];
            scene.enableFog = std::holds_alternative<int>(v) ? std::get<int>(v) != 0 : std::get<float>(v) != 0.0f;
        }
        if (settings->fields.count("fogDensity"))
        {
            auto v = settings->fields["fogDensity"];
            scene.fogDensity = std::holds_alternative<int>(v) ? std::get<int>(v) : std::get<float>(v);
        }
        if (settings->fields.count("fogColor"))
        {
            std::string color = std::get<std::string>(settings->fields["fogColor"]);
            if (color[0] == '#' && color.length() == 7)
            {
                scene.fogColor.r = std::stoi(color.substr(1, 2), nullptr, 16);
                scene.fogColor.g = std::stoi(color.substr(3, 2), nullptr, 16);
                scene.fogColor.b = std::stoi(color.substr(5, 2), nullptr, 16);
            }
        }
        if (settings->fields.count("backgroundColor"))
        {
            std::string color = std::get<std::string>(settings->fields["backgroundColor"]);
            if (color[0] == '#' && color.length() == 7)
            {
                scene.background.r = std::stoi(color.substr(1, 2), nullptr, 16);
                scene.background.g = std::stoi(color.substr(3, 2), nullptr, 16);
                scene.background.b = std::stoi(color.substr(5, 2), nullptr, 16);
            }
        }
        return "";
    }
};
REGISTER_BUILTIN(SetGraphicsBuiltin)
class GetGroundNormalBuiltin : public BuiltinFunction
{
public:
    //@desc Get ground normal vector for mesh
    //@parent mesh
    std::string getName() const override { return "getGroundNormal"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (!node->callee)
            throw std::runtime_error("getGroundNormal must be called on mesh");
        auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
        Value objVal = interp->evaluate(fa->object.get());
        auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
        if (obj->fields.count("_meshId"))
        {
            int meshId = std::get<int>(obj->fields["_meshId"]);
            Mesh &mesh = meshes[meshId];
            auto result = std::make_shared<ObjectValue>();
            result->fields["x"] = mesh.groundNormal.x;
            result->fields["y"] = mesh.groundNormal.y;
            result->fields["z"] = mesh.groundNormal.z;
            interp->lastValue = result;
            return "{object}";
        }
        throw std::runtime_error("getGroundNormal requires mesh object");
    }
};
REGISTER_BUILTIN(GetGroundNormalBuiltin)
class SetTextureBuiltin : public BuiltinFunction
{
public:
    //@desc Apply texture image to mesh
    //@parent mesh,loadOBJ
    std::string getName() const override { return "setTexture"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 1)
            throw std::runtime_error("mesh.setTexture(filepath)");
        if (!node->callee)
            throw std::runtime_error("setTexture must be called on mesh");
        auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
        Value objVal = interp->evaluate(fa->object.get());
        auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
        if (!obj->fields.count("_meshId"))
            throw std::runtime_error("setTexture requires mesh object");
        int meshId = std::get<int>(obj->fields["_meshId"]);
        std::string filepath = std::get<std::string>(interp->evaluate(node->args[0].get()));
        
        TextureOptions texOpts;
        texOpts.path = filepath;
        pendingTextures[meshId].push_back({"", texOpts});
        
        return "";
    }
};
REGISTER_BUILTIN(SetTextureBuiltin)
class CreateBoneBuiltin : public BuiltinFunction
{
public:
    //@desc Create skeletal bone for animation
    //@parent mesh
    std::string getName() const override { return "createBone"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() < 1)
            throw std::runtime_error("mesh.createBone(name, [parentBoneIndex])");
        if (!node->callee)
            throw std::runtime_error("createBone must be called on mesh");
        auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
        if (!fa)
            throw std::runtime_error("createBone must be called on mesh");
        Value objVal = interp->evaluate(fa->object.get());
        if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(objVal))
            throw std::runtime_error("createBone requires mesh object");
        auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
        if (!obj || !obj->fields.count("_meshId"))
            throw std::runtime_error("createBone requires mesh object");
        int meshId = std::get<int>(obj->fields["_meshId"]);
        std::string name = std::get<std::string>(interp->evaluate(node->args[0].get()));
        Bone bone;
        bone.name = name;
        if (node->args.size() >= 2)
        {
            auto v = interp->evaluate(node->args[1].get());
            bone.parentId = std::holds_alternative<int>(v) ? std::get<int>(v) : (int)std::get<float>(v);
        }
        skeletons[meshId].push_back(bone);
        interp->lastValue = (int)skeletons[meshId].size() - 1;
        return std::to_string(skeletons[meshId].size() - 1);
    }
};
class SetBonePoseBuiltin : public BuiltinFunction
{
public:
    //@desc Set bone position and rotation
    //@parent mesh
    std::string getName() const override { return "setBonePose"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 7)
            throw std::runtime_error("mesh.setBonePose(boneIndex, px, py, pz, rx, ry, rz)");
        if (!node->callee)
            throw std::runtime_error("setBonePose must be called on mesh");
        auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
        if (!fa)
            return "";
        Value objVal = interp->evaluate(fa->object.get());
        if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(objVal))
            return "";
        auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
        if (!obj || !obj->fields.count("_meshId"))
            return "";
        int meshId = std::get<int>(obj->fields["_meshId"]);
        if (!skeletons.count(meshId))
            return "";
        auto v0 = interp->evaluate(node->args[0].get());
        int boneIdx = std::holds_alternative<int>(v0) ? std::get<int>(v0) : (int)std::get<float>(v0);
        if (boneIdx < 0 || boneIdx >= (int)skeletons[meshId].size())
            return "";
        Bone &bone = skeletons[meshId][boneIdx];
        auto v1 = interp->evaluate(node->args[1].get());
        auto v2 = interp->evaluate(node->args[2].get());
        auto v3 = interp->evaluate(node->args[3].get());
        auto v4 = interp->evaluate(node->args[4].get());
        auto v5 = interp->evaluate(node->args[5].get());
        auto v6 = interp->evaluate(node->args[6].get());
        bone.position.x = std::holds_alternative<int>(v1) ? std::get<int>(v1) : std::get<float>(v1);
        bone.position.y = std::holds_alternative<int>(v2) ? std::get<int>(v2) : std::get<float>(v2);
        bone.position.z = std::holds_alternative<int>(v3) ? std::get<int>(v3) : std::get<float>(v3);
        bone.rotation.x = std::holds_alternative<int>(v4) ? std::get<int>(v4) : std::get<float>(v4);
        bone.rotation.y = std::holds_alternative<int>(v5) ? std::get<int>(v5) : std::get<float>(v5);
        bone.rotation.z = std::holds_alternative<int>(v6) ? std::get<int>(v6) : std::get<float>(v6);
        return "";
    }
};
class CreateAnimationBuiltin : public BuiltinFunction
{
public:
    //@desc Create animation with name and duration
    //@parent mesh
    std::string getName() const override { return "createAnimation"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 2)
            throw std::runtime_error("mesh.createAnimation(name, duration)");
        if (!node->callee)
            throw std::runtime_error("createAnimation must be called on mesh");
        auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
        Value objVal = interp->evaluate(fa->object.get());
        auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
        if (!obj->fields.count("_meshId"))
            throw std::runtime_error("createAnimation requires mesh object");
        int meshId = std::get<int>(obj->fields["_meshId"]);
        std::string name = std::get<std::string>(interp->evaluate(node->args[0].get()));
        auto v = interp->evaluate(node->args[1].get());
        float duration = std::holds_alternative<int>(v) ? std::get<int>(v) : std::get<float>(v);
        Animation anim;
        anim.name = name;
        anim.duration = duration;
        animations[meshId].push_back(anim);
        interp->lastValue = (int)animations[meshId].size() - 1;
        return std::to_string(animations[meshId].size() - 1);
    }
};
class AddAnimKeyBuiltin : public BuiltinFunction
{
public:
    //@desc Add keyframe to animation
    //@parent mesh
    std::string getName() const override { return "addAnimKey"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() < 6)
            throw std::runtime_error("mesh.addAnimKey(animIdx, boneName, time, x, y, z, [isRotation])");
        if (!node->callee)
            throw std::runtime_error("addAnimKey must be called on mesh");
        auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
        Value objVal = interp->evaluate(fa->object.get());
        auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
        if (!obj->fields.count("_meshId"))
            throw std::runtime_error("addAnimKey requires mesh object");
        int meshId = std::get<int>(obj->fields["_meshId"]);
        auto v0 = interp->evaluate(node->args[0].get());
        int animIdx = std::holds_alternative<int>(v0) ? std::get<int>(v0) : (int)std::get<float>(v0);
        std::string boneName = std::get<std::string>(interp->evaluate(node->args[1].get()));
        auto v2 = interp->evaluate(node->args[2].get());
        float time = std::holds_alternative<int>(v2) ? std::get<int>(v2) : std::get<float>(v2);
        auto v3 = interp->evaluate(node->args[3].get());
        auto v4 = interp->evaluate(node->args[4].get());
        auto v5 = interp->evaluate(node->args[5].get());
        Vec3 value;
        value.x = std::holds_alternative<int>(v3) ? std::get<int>(v3) : std::get<float>(v3);
        value.y = std::holds_alternative<int>(v4) ? std::get<int>(v4) : std::get<float>(v4);
        value.z = std::holds_alternative<int>(v5) ? std::get<int>(v5) : std::get<float>(v5);
        bool isRotation = node->args.size() >= 7 && std::get<int>(interp->evaluate(node->args[6].get())) != 0;
        if (isRotation)
            animations[meshId][animIdx].rotationKeys[boneName].push_back({time, value});
        else
            animations[meshId][animIdx].positionKeys[boneName].push_back({time, value});
        return "";
    }
};
class PlayAnimationBuiltin : public BuiltinFunction
{
public:
    //@desc Start playing animation by index
    //@parent mesh
    std::string getName() const override { return "playAnimation"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 1)
            throw std::runtime_error("mesh.playAnimation(animIndex)");
        if (!node->callee)
            throw std::runtime_error("playAnimation must be called on mesh");
        auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
        Value objVal = interp->evaluate(fa->object.get());
        auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
        if (!obj->fields.count("_meshId"))
            throw std::runtime_error("playAnimation requires mesh object");
        int meshId = std::get<int>(obj->fields["_meshId"]);
        auto v = interp->evaluate(node->args[0].get());
        int animIdx = std::holds_alternative<int>(v) ? std::get<int>(v) : (int)std::get<float>(v);
        animationStates[meshId] = {animIdx, 0.0f};
        return "";
    }
};
class UpdateAnimationBuiltin : public BuiltinFunction
{
public:
    //@desc Update animation with delta time
    //@parent mesh
    std::string getName() const override { return "updateAnimation"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 1)
            throw std::runtime_error("mesh.updateAnimation(deltaTime)");
        if (!node->callee)
            throw std::runtime_error("updateAnimation must be called on mesh");
        auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
        if (!fa)
            return "";
        Value objVal = interp->evaluate(fa->object.get());
        if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(objVal))
            return "";
        auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
        if (!obj || !obj->fields.count("_meshId"))
            return "";
        int meshId = std::get<int>(obj->fields["_meshId"]);
        if (!animationStates.count(meshId))
            return "";
        if (!animations.count(meshId))
            return "";
        auto v = interp->evaluate(node->args[0].get());
        float dt = std::holds_alternative<int>(v) ? std::get<int>(v) : std::get<float>(v);
        auto &[animIdx, time] = animationStates[meshId];
        if (animIdx < 0 || animIdx >= (int)animations[meshId].size())
            return "";
        if (!skeletons.count(meshId))
            return "";
        Animation &anim = animations[meshId][animIdx];
        time += dt;
        if (time > anim.duration)
            time = fmod(time, anim.duration);
        for (size_t i = 0; i < skeletons[meshId].size(); i++)
        {
            Bone &bone = skeletons[meshId][i];
            if (anim.positionKeys.count(bone.name) && anim.positionKeys[bone.name].size() > 1)
            {
                auto &keys = anim.positionKeys[bone.name];
                for (size_t k = 0; k < keys.size() - 1; k++)
                {
                    if (time >= keys[k].first && time <= keys[k + 1].first)
                    {
                        float t = (time - keys[k].first) / (keys[k + 1].first - keys[k].first);
                        bone.position.x = keys[k].second.x + t * (keys[k + 1].second.x - keys[k].second.x);
                        bone.position.y = keys[k].second.y + t * (keys[k + 1].second.y - keys[k].second.y);
                        bone.position.z = keys[k].second.z + t * (keys[k + 1].second.z - keys[k].second.z);
                        break;
                    }
                }
            }
            if (anim.rotationKeys.count(bone.name))
            {
                auto &keys = anim.rotationKeys[bone.name];
                for (size_t k = 0; k < keys.size() - 1; k++)
                {
                    if (time >= keys[k].first && time <= keys[k + 1].first)
                    {
                        float t = (time - keys[k].first) / (keys[k + 1].first - keys[k].first);
                        bone.rotation.x = keys[k].second.x + t * (keys[k + 1].second.x - keys[k].second.x);
                        bone.rotation.y = keys[k].second.y + t * (keys[k + 1].second.y - keys[k].second.y);
                        bone.rotation.z = keys[k].second.z + t * (keys[k + 1].second.z - keys[k].second.z);
                        break;
                    }
                }
            }
        }
        animationStates[meshId].second = time;
        return "";
    }
};
class AddAnimationBuiltin : public BuiltinFunction
{
public:
    //@desc Add animation with callback function
    //@parent mesh
    std::string getName() const override { return "addAnimation"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 2)
            throw std::runtime_error("mesh.addAnimation(name, callback)");
        if (!node->callee)
            throw std::runtime_error("addAnimation must be called on mesh");
        auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
        Value objVal = interp->evaluate(fa->object.get());
        auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
        if (!obj->fields.count("_meshId"))
            throw std::runtime_error("addAnimation requires mesh object");
        int meshId = std::get<int>(obj->fields["_meshId"]);
        std::string name = std::get<std::string>(interp->evaluate(node->args[0].get()));
        Value callbackVal = interp->evaluate(node->args[1].get());
        
        // Store the callback (can be FunctionDeclaration* or FunctionExpression*)
        if (!obj->fields.count("_animations"))
        {
            auto animMap = std::make_shared<ObjectValue>();
            obj->fields["_animations"] = animMap;
        }
        auto animMap = std::get<std::shared_ptr<ObjectValue>>(obj->fields["_animations"]);
        animMap->fields[name] = callbackVal;
        return "";
    }
};
class PlayAnimCallbackBuiltin : public BuiltinFunction
{
public:
    //@desc Play animation by name with callback
    //@parent mesh
    std::string getName() const override { return "playAnimCallback"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 1)
            throw std::runtime_error("mesh.playAnimCallback(name)");
        if (!node->callee)
            throw std::runtime_error("playAnimCallback must be called on mesh");
        auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
        Value objVal = interp->evaluate(fa->object.get());
        auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
        if (!obj->fields.count("_meshId"))
            throw std::runtime_error("playAnimCallback requires mesh object");
        std::string name = std::get<std::string>(interp->evaluate(node->args[0].get()));
        if (!obj->fields.count("_animations"))
            throw std::runtime_error("No animations defined");
        auto animMap = std::get<std::shared_ptr<ObjectValue>>(obj->fields["_animations"]);
        if (!animMap->fields.count(name))
            throw std::runtime_error("Animation not found: " + name);
        obj->fields["_currentAnim"] = name;
        obj->fields["_animTime"] = 0.0f;
        return "";
    }
};
class UpdateAnimCallbackBuiltin : public BuiltinFunction
{
public:
    //@desc Update callback-based animation
    //@parent mesh
    std::string getName() const override { return "updateAnimCallback"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override
    {
        if (node->args.size() != 1)
            throw std::runtime_error("mesh.updateAnimCallback(deltaTime)");
        if (!node->callee)
            throw std::runtime_error("updateAnimCallback must be called on mesh");
        auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
        Value objVal = interp->evaluate(fa->object.get());
        auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
        if (!obj->fields.count("_meshId"))
            throw std::runtime_error("updateAnimCallback requires mesh object");
        if (!obj->fields.count("_currentAnim"))
            return "";
        if (!obj->fields.count("_animations"))
            return "";
            
        std::string animName = std::get<std::string>(obj->fields["_currentAnim"]);
        auto animMap = std::get<std::shared_ptr<ObjectValue>>(obj->fields["_animations"]);
        if (!animMap->fields.count(animName))
            return "";
        
        Value callbackVal = animMap->fields[animName];
        auto v = interp->evaluate(node->args[0].get());
        float dt = std::holds_alternative<int>(v) ? std::get<int>(v) : std::get<float>(v);
        
        // Update time
        float time = 0.0f;
        if (obj->fields.count("_animTime"))
        {
            if (std::holds_alternative<float>(obj->fields["_animTime"]))
                time = std::get<float>(obj->fields["_animTime"]);
            else if (std::holds_alternative<int>(obj->fields["_animTime"]))
                time = std::get<int>(obj->fields["_animTime"]);
        }
        time += dt;
        obj->fields["_animTime"] = time;
        
        // Create state object with time
        auto stateObj = std::make_shared<ObjectValue>();
        stateObj->fields["time"] = time;
        
        try {
            // Call the function based on callback type
            if (std::holds_alternative<FunctionDeclaration*>(callbackVal))
            {
                auto funcDecl = std::get<FunctionDeclaration*>(callbackVal);
                if (!funcDecl || !funcDecl->body)
                    return "";
                    
                interp->environment.pushScope();
                if (!funcDecl->params.empty())
                {
                    interp->environment.define(funcDecl->params[0].first, Variable(stateObj, "object", false));
                }
                try {
                    interp->executeBlock(funcDecl->body.get());
                } catch (const ReturnException&) {}
                interp->environment.popScope();
            }
            else if (std::holds_alternative<FunctionExpression*>(callbackVal))
            {
                auto funcExpr = std::get<FunctionExpression*>(callbackVal);
                if (!funcExpr || !funcExpr->body)
                    return "";
                    
                interp->environment.pushScope();
                if (!funcExpr->params.empty())
                {
                    interp->environment.define(funcExpr->params[0].first, Variable(stateObj, "object", false));
                }
                try {
                    interp->executeBlock(funcExpr->body.get());
                } catch (const ReturnException&) {}
                interp->environment.popScope();
            }
        } catch (const std::exception& e) {
            // Silently catch errors to prevent crashes
            return "";
        }
        
        return "";
    }
};
REGISTER_BUILTIN(CreateBoneBuiltin)
REGISTER_BUILTIN(SetBonePoseBuiltin)
REGISTER_BUILTIN(CreateAnimationBuiltin)
REGISTER_BUILTIN(AddAnimKeyBuiltin)
REGISTER_BUILTIN(PlayAnimationBuiltin)
REGISTER_BUILTIN(UpdateAnimationBuiltin)
REGISTER_BUILTIN(AddAnimationBuiltin)
REGISTER_BUILTIN(PlayAnimCallbackBuiltin)
REGISTER_BUILTIN(UpdateAnimCallbackBuiltin)
