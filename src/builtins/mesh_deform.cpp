#include "include/builtins.h"
#include <SDL2/SDL.h>
#include <cmath>
#include <vector>
#include <unordered_map>

struct Vec3 { float x, y, z; };

struct Mesh {
    std::vector<Vec3> vertices;
    std::vector<int> indices;
    Vec3 position{0, 0, 0};
    Vec3 rotation{0, 0, 0};
    Vec3 scale{1, 1, 1};
    SDL_Color color{100, 200, 255, 255};
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
};

void calcAABB(Mesh& mesh);

extern std::unordered_map<int, Mesh> meshes;

class DeformVerticesBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "deformVertices"; }
    std::string execute(Interpreter* interp, FunctionCall* node) override {
        if (node->args.size() != 3) throw std::runtime_error("deformVertices(amplitude, frequency, seed)");
        if (!node->callee) throw std::runtime_error("deformVertices must be called on mesh");
        
        auto fa = dynamic_cast<FieldAccess*>(node->callee.get());
        Value objVal = interp->evaluate(fa->object.get());
        auto obj = std::get<std::shared_ptr<ObjectValue>>(objVal);
        
        if (obj->fields.count("_meshId")) {
            int meshId = std::get<int>(obj->fields["_meshId"]);
            Mesh& mesh = meshes[meshId];
            
            auto v0 = interp->evaluate(node->args[0].get());
            auto v1 = interp->evaluate(node->args[1].get());
            auto v2 = interp->evaluate(node->args[2].get());
            float amplitude = std::holds_alternative<int>(v0) ? std::get<int>(v0) : std::get<float>(v0);
            float frequency = std::holds_alternative<int>(v1) ? std::get<int>(v1) : std::get<float>(v1);
            float seed = std::holds_alternative<int>(v2) ? std::get<int>(v2) : std::get<float>(v2);
            
            for (auto& v : mesh.vertices) {
                float noise = sin(v.x * frequency + seed) * cos(v.z * frequency + seed * 1.3f);
                v.y += noise * amplitude;
            }
            
            if (!mesh.vertices.empty()) {
                mesh.aabbMin = mesh.aabbMax = mesh.vertices[0];
                for (const auto& v : mesh.vertices) {
                    if (v.x < mesh.aabbMin.x) mesh.aabbMin.x = v.x;
                    if (v.y < mesh.aabbMin.y) mesh.aabbMin.y = v.y;
                    if (v.z < mesh.aabbMin.z) mesh.aabbMin.z = v.z;
                    if (v.x > mesh.aabbMax.x) mesh.aabbMax.x = v.x;
                    if (v.y > mesh.aabbMax.y) mesh.aabbMax.y = v.y;
                    if (v.z > mesh.aabbMax.z) mesh.aabbMax.z = v.z;
                }
            }
        }
        return "";
    }
};

REGISTER_BUILTIN(DeformVerticesBuiltin)
