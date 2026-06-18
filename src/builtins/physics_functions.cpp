#include "include/physics_functions.h"
#include "include/builtins.h"
#include <cmath>
#include <algorithm>
#include <SDL2/SDL.h>

// Define structures locally to match canvas_3d_functions.cpp
struct Vec2 { float x, y; };

struct Mesh {
    std::vector<Vec3> vertices;
    std::vector<Vec2> uvs;
    std::vector<int> indices;
    std::vector<int> uvIndices;
    std::vector<std::string> faceMaterials;
    std::unordered_map<std::string, void*> materialTextures;  // GLuint simplified
    std::unordered_map<std::string, SDL_Color> materialColors;
    std::unordered_map<std::string, void*> materialTexOpts;  // TextureOptions simplified
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
    void* textureId = nullptr;
    Vec3 groundNormal{0, 1, 0};
    bool enableDropShadow = false;
    float dropShadowOpacity = 0.5f;
    float dropShadowOffset = 0.1f;
};

struct Scene {
    std::vector<int> meshIds;
    std::vector<int> lightIds;
};

// Forward declarations from canvas_3d_functions.cpp
extern std::unordered_map<int, Mesh> meshes;
extern std::unordered_map<int, std::shared_ptr<Scene>> scenes;

// Global physics contexts (one per scene)
static std::unordered_map<int, std::unique_ptr<PhysicsContext>> physicsContexts;
static std::unordered_map<int, std::unordered_map<int, std::unique_ptr<PhysicsHandle>>> physicsHandles;

// ============================================================================
// PhysicsContext Implementation
// ============================================================================

PhysicsHandle* PhysicsContext::attachPhysics(Mesh* mesh, int meshId, float mass) {
    auto handle = std::make_unique<PhysicsHandle>(mesh, meshId);
    handle->mass = mass;
    handle->velocity = {0, 0, 0};
    PhysicsHandle* ptr = handle.get();
    bodies.push_back(std::move(handle));
    return ptr;
}

PhysicsHandle* PhysicsContext::getHandle(int meshId) {
    for (auto& body : bodies) {
        if (body->meshId == meshId) {
            return body.get();
        }
    }
    return nullptr;
}

static float distance(Vec3 a, Vec3 b) {
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float dz = b.z - a.z;
    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

static Vec3 normalize(Vec3 v) {
    float len = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    if (len < 0.0001f) return {0, 0, 0};
    return {v.x/len, v.y/len, v.z/len};
}

static float dot(Vec3 a, Vec3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

void PhysicsContext::step(float dt) {
    // Apply gravity
    for (auto& body : bodies) {
        if (!body->isKinematic && body->affectedByGravity) {
            body->accumulatedForce.y += gravity.y * body->mass;
        }
    }
    
    // Integrate velocity
    for (auto& body : bodies) {
        if (!body->isKinematic) {
            // F = ma => a = F/m
            float ax = body->accumulatedForce.x / body->mass;
            float ay = body->accumulatedForce.y / body->mass;
            float az = body->accumulatedForce.z / body->mass;
            
            body->velocity.x += ax * dt;
            body->velocity.y += ay * dt;
            body->velocity.z += az * dt;
            
            // Apply drag
            body->velocity.x *= (1.0f - body->drag * dt);
            body->velocity.y *= (1.0f - body->drag * dt);
            body->velocity.z *= (1.0f - body->drag * dt);
            
            // Integrate position
            body->mesh->position.x += body->velocity.x * dt;
            body->mesh->position.y += body->velocity.y * dt;
            body->mesh->position.z += body->velocity.z * dt;
            
            // Apply angular drag
            body->angularVelocity.x *= (1.0f - body->angularDrag * dt);
            body->angularVelocity.y *= (1.0f - body->angularDrag * dt);
            body->angularVelocity.z *= (1.0f - body->angularDrag * dt);
            
            // Integrate rotation
            body->mesh->rotation.x += body->angularVelocity.x * dt;
            body->mesh->rotation.y += body->angularVelocity.y * dt;
            body->mesh->rotation.z += body->angularVelocity.z * dt;
        }
        
        // Clear forces after integration
        body->accumulatedForce = {0, 0, 0};
    }
    
    // Simple ground check (y < 0.1)
    for (auto& body : bodies) {
        body->isGrounded = body->mesh->position.y < 0.1f;
        if (body->isGrounded) {
            body->velocity.y = 0.0f;  // Clamp to ground
            body->mesh->position.y = 0.0f;
        }
    }
}

// ============================================================================
// Motion API Implementation
// ============================================================================

namespace motion {
    void moveTowards(PhysicsHandle* handle, Vec3 target, float speed, float stopDistance) {
        if (!handle) return;
        
        Vec3 delta = {
            target.x - handle->mesh->position.x,
            target.y - handle->mesh->position.y,
            target.z - handle->mesh->position.z
        };
        float dist = distance({0,0,0}, delta);
        
        if (dist > stopDistance) {
            Vec3 dir = normalize(delta);
            handle->velocity.x = dir.x * speed;
            handle->velocity.y = dir.y * speed;
            handle->velocity.z = dir.z * speed;
        } else {
            handle->velocity = {0, 0, 0};
        }
    }
    
    void slide(PhysicsHandle* handle, Vec3 direction, float speed, float friction) {
        if (!handle) return;
        
        Vec3 dir = normalize(direction);
        handle->velocity.x = dir.x * speed;
        handle->velocity.y = dir.y * speed;
        handle->velocity.z = dir.z * speed;
        handle->drag = 1.0f - friction;
    }
    
    void rotateTowards(PhysicsHandle* handle, Vec3 targetDir, float turnSpeed) {
        if (!handle) return;
        // Simplified: rotate towards (full implementation would use quaternions)
        handle->angularVelocity = normalize(targetDir);
        handle->angularVelocity.x *= turnSpeed;
        handle->angularVelocity.y *= turnSpeed;
        handle->angularVelocity.z *= turnSpeed;
    }
    
    void setVelocity(PhysicsHandle* handle, Vec3 vel) {
        if (!handle) return;
        handle->velocity = vel;
    }
    
    void stop(PhysicsHandle* handle) {
        if (!handle) return;
        handle->velocity = {0, 0, 0};
        handle->angularVelocity = {0, 0, 0};
    }
}

// ============================================================================
// Forces API Implementation
// ============================================================================

namespace forces {
    void push(PhysicsHandle* handle, Vec3 direction, float strength) {
        if (!handle) return;
        Vec3 dir = normalize(direction);
        handle->accumulatedForce.x += dir.x * strength;
        handle->accumulatedForce.y += dir.y * strength;
        handle->accumulatedForce.z += dir.z * strength;
    }
    
    void explosion(PhysicsContext* ctx, Vec3 origin, float radius, float force) {
        if (!ctx) return;
        for (auto& body : ctx->bodies) {
            float dist = distance(origin, body->mesh->position);
            if (dist < radius && dist > 0.001f) {
                float falloff = 1.0f - (dist / radius);
                Vec3 dir = normalize({
                    body->mesh->position.x - origin.x,
                    body->mesh->position.y - origin.y,
                    body->mesh->position.z - origin.z
                });
                push(body.get(), dir, force * falloff);
            }
        }
    }
    
    void spring(PhysicsHandle* handle, Vec3 anchor, float stiffness, float damping) {
        if (!handle) return;
        Vec3 delta = {
            anchor.x - handle->mesh->position.x,
            anchor.y - handle->mesh->position.y,
            anchor.z - handle->mesh->position.z
        };
        
        // F = -k * x - c * v
        handle->accumulatedForce.x += stiffness * delta.x - damping * handle->velocity.x;
        handle->accumulatedForce.y += stiffness * delta.y - damping * handle->velocity.y;
        handle->accumulatedForce.z += stiffness * delta.z - damping * handle->velocity.z;
    }
    
    void drag(PhysicsHandle* handle, float coefficient) {
        if (!handle) return;
        handle->drag = coefficient;
    }
    
    void clear(PhysicsHandle* handle) {
        if (!handle) return;
        handle->accumulatedForce = {0, 0, 0};
    }
}

// ============================================================================
// Collision API Implementation
// ============================================================================

namespace collision {
    bool isGrounded(PhysicsHandle* handle, PhysicsContext* ctx, float rayDistance) {
        if (!handle) return false;
        return handle->isGrounded;
    }
    
    float distance_between(PhysicsHandle* a, PhysicsHandle* b) {
        if (!a || !b) return 0.0f;
        return distance(a->mesh->position, b->mesh->position);
    }
    
    bool overlaps(PhysicsHandle* handle, Vec3 center, float radius) {
        if (!handle) return false;
        float dist = distance(handle->mesh->position, center);
        return dist < radius;
    }
    
    RaycastHit raycast(PhysicsContext* ctx, Vec3 origin, Vec3 direction, float maxDist) {
        RaycastHit hit;
        if (!ctx) return hit;
        
        Vec3 dir = normalize(direction);
        float closestDist = maxDist;
        
        for (auto& body : ctx->bodies) {
            // Simple sphere raycast
            Vec3 oc = {
                origin.x - body->mesh->position.x,
                origin.y - body->mesh->position.y,
                origin.z - body->mesh->position.z
            };
            float radius = 0.5f;  // TODO: get from mesh
            
            float a = dot(dir, dir);
            float b = 2.0f * dot(oc, dir);
            float c = dot(oc, oc) - radius*radius;
            float discriminant = b*b - 4*a*c;
            
            if (discriminant >= 0) {
                float t = (-b - std::sqrt(discriminant)) / (2*a);
                if (t > 0 && t < closestDist) {
                    closestDist = t;
                    hit.hit = true;
                    hit.handle = body.get();
                    hit.distance = t;
                    hit.point = {
                        origin.x + dir.x * t,
                        origin.y + dir.y * t,
                        origin.z + dir.z * t
                    };
                }
            }
        }
        
        return hit;
    }
}

// ============================================================================
// Constraints API Implementation
// ============================================================================

namespace constraints {
    void lockPosition(PhysicsHandle* handle, const std::vector<char>& axes) {
        if (!handle) return;
        for (char axis : axes) {
            switch (axis) {
                case 'x': handle->velocity.x = 0; break;
                case 'y': handle->velocity.y = 0; break;
                case 'z': handle->velocity.z = 0; break;
            }
        }
    }
    
    void limitRotation(PhysicsHandle* handle, Vec3 minAngles, Vec3 maxAngles) {
        if (!handle) return;
        handle->mesh->rotation.x = std::max(minAngles.x, std::min(maxAngles.x, handle->mesh->rotation.x));
        handle->mesh->rotation.y = std::max(minAngles.y, std::min(maxAngles.y, handle->mesh->rotation.y));
        handle->mesh->rotation.z = std::max(minAngles.z, std::min(maxAngles.z, handle->mesh->rotation.z));
    }
    
    void distance(PhysicsHandle* a, PhysicsHandle* b, float targetDist, float stiffness) {
        if (!a || !b) return;
        float dist = collision::distance(a, b);
        if (dist < 0.001f) return;
        
        float error = dist - targetDist;
        Vec3 delta = {
            b->mesh->position.x - a->mesh->position.x,
            b->mesh->position.y - a->mesh->position.y,
            b->mesh->position.z - a->mesh->position.z
        };
        Vec3 dir = normalize(delta);
        
        Vec3 force = {dir.x * error * stiffness, dir.y * error * stiffness, dir.z * error * stiffness};
        a->accumulatedForce.x += force.x;
        a->accumulatedForce.y += force.y;
        a->accumulatedForce.z += force.z;
    }
}

// ============================================================================
// Character Controller Implementation
// ============================================================================

namespace character {
    std::unique_ptr<Controller> create(PhysicsHandle* handle, float height, 
                                       float radius, float stepHeight) {
        auto ctrl = std::make_unique<Controller>(handle);
        ctrl->height = height;
        ctrl->radius = radius;
        ctrl->stepHeight = stepHeight;
        handle->isCharacter = true;
        return ctrl;
    }
    
    void move(Controller* ctrl, Vec3 inputDir, float speed) {
        if (!ctrl || !ctrl->handle) return;
        
        Vec3 dir = normalize(inputDir);
        ctrl->handle->velocity.x = dir.x * speed;
        ctrl->handle->velocity.z = dir.z * speed;
        // Don't override y (gravity handles it)
    }
    
    void jump(Controller* ctrl, float impulse) {
        if (!ctrl || !ctrl->handle) return;
        if (ctrl->handle->isGrounded) {
            ctrl->handle->velocity.y = impulse;
            ctrl->handle->isGrounded = false;
        }
    }
    
    bool isGrounded(Controller* ctrl, PhysicsContext* ctx) {
        if (!ctrl || !ctrl->handle) return false;
        return ctrl->handle->isGrounded;
    }
    
    Vec3 getSurfaceNormal(Controller* ctrl, PhysicsContext* ctx) {
        if (!ctrl) return {0, 1, 0};
        return {0, 1, 0};  // TODO: raycast for actual normal
    }
}

// ============================================================================
// Animation ↔ Physics Bridge Implementation
// ============================================================================

namespace animation_physics {
    void enableRagdoll(PhysicsHandle* handle) {
        if (!handle) return;
        handle->ragdollEnabled = true;
        handle->ragdollBlend = 1.0f;
    }
    
    void disableRagdoll(PhysicsHandle* handle) {
        if (!handle) return;
        handle->ragdollEnabled = false;
        handle->ragdollBlend = 0.0f;
    }
    
    void blendToAnimation(PhysicsHandle* handle, float t) {
        if (!handle) return;
        handle->ragdollBlend = std::max(0.0f, std::min(1.0f, t));
    }
    
    void attachBone(PhysicsHandle* handle, const std::string& boneName, 
                    float boneMass, float swingLimit, float twistLimit) {
        if (!handle) return;
        // TODO: implement when bone system is integrated
    }
}

// ============================================================================
// Projectile API Implementation
// ============================================================================

namespace projectiles {
    PhysicsHandle* spawn(PhysicsContext* ctx, const ProjectileConfig& config) {
        if (!ctx || !config.mesh) return nullptr;
        
        PhysicsHandle* handle = ctx->attachPhysics(config.mesh, -1);  // TODO: real mesh ID
        handle->mesh->position = config.position;
        
        Vec3 dir = normalize(config.direction);
        handle->velocity.x = dir.x * config.speed;
        handle->velocity.y = dir.y * config.speed;
        handle->velocity.z = dir.z * config.speed;
        
        handle->mass = 0.1f;
        handle->affectedByGravity = true;
        
        return handle;
    }
}

// ============================================================================
// Utility Functions Implementation
// ============================================================================

namespace util {
    Vec3 predictTrajectory(Vec3 start, Vec3 velocity, Vec3 gravity, float time) {
        return {
            start.x + velocity.x * time,
            start.y + velocity.y * time + 0.5f * gravity.y * time * time,
            start.z + velocity.z * time
        };
    }
    
    Vec3 computeImpulse(float mass, Vec3 desiredVelocity) {
        return {
            desiredVelocity.x * mass,
            desiredVelocity.y * mass,
            desiredVelocity.z * mass
        };
    }
    
    void stabilize(PhysicsHandle* handle) {
        if (!handle) return;
        handle->velocity = {0, 0, 0};
        handle->angularVelocity = {0, 0, 0};
        handle->accumulatedForce = {0, 0, 0};
    }
}

// ============================================================================
// Debug API Implementation
// ============================================================================

namespace debug_physics {
    void drawColliders(PhysicsContext* ctx) {
        // TODO: implement with wireframe visualization
    }
    
    void drawForces(PhysicsHandle* handle) {
        // TODO: implement with debug lines
    }
    
    void freeze(PhysicsHandle* handle) {
        if (!handle) return;
        handle->isKinematic = true;
        handle->velocity = {0, 0, 0};
    }
    
    void stepOnce(PhysicsContext* ctx) {
        if (!ctx) return;
        ctx->step(ctx->timeStep);
    }
}

// ============================================================================
// Builtin Functions (Axolotl bindings)
// ============================================================================

class CreatePhysicsContextBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "createPhysicsContext"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override {
        int sceneId = 0;
        
        if (node->args.size() > 0) {
            Value arg = interp->evaluate(node->args[0].get());
            if (std::holds_alternative<int>(arg)) {
                sceneId = std::get<int>(arg);
            }
        }
        
        Vec3 gravity = {0, -9.81f, 0};
        float timeStep = 1.0f / 60.0f;
        
        auto ctx = std::make_unique<PhysicsContext>(gravity, timeStep);
        physicsContexts[sceneId] = std::move(ctx);
        
        auto obj = std::make_shared<ObjectValue>();
        obj->fields["_contextId"] = sceneId;
        interp->lastValue = obj;
        return "{object}";
    }
};

class AttachPhysicsBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "attachPhysics"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override {
        if (node->args.size() < 1) {
            throw std::runtime_error("attachPhysics requires mesh argument");
        }
        
        Value meshVal = interp->evaluate(node->args[0].get());
        if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(meshVal)) {
            throw std::runtime_error("Expected mesh object");
        }
        
        auto meshObj = std::get<std::shared_ptr<ObjectValue>>(meshVal);
        int meshId = std::get<int>(meshObj->fields["_meshId"]);
        float mass = 1.0f;
        
        if (node->args.size() > 1) {
            Value massVal = interp->evaluate(node->args[1].get());
            if (std::holds_alternative<float>(massVal)) {
                mass = std::get<float>(massVal);
            }
        }
        
        if (meshes.find(meshId) == meshes.end()) {
            throw std::runtime_error("Mesh not found");
        }
        
        Mesh* mesh = &meshes[meshId];
        int contextId = 0;  // TODO: get from context
        
        if (physicsContexts.find(contextId) == physicsContexts.end()) {
            physicsContexts[contextId] = std::make_unique<PhysicsContext>();
        }
        
        PhysicsHandle* handle = physicsContexts[contextId]->attachPhysics(mesh, meshId, mass);
        
        auto obj = std::make_shared<ObjectValue>();
        obj->fields["_meshId"] = meshId;
        obj->fields["_handlePtr"] = (int)(intptr_t)handle;
        interp->lastValue = obj;
        return "{object}";
    }
};

class PhysicsStepBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "physicsStep"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override {
        float dt = 1.0f / 60.0f;
        if (node->args.size() > 0) {
            Value dtVal = interp->evaluate(node->args[0].get());
            if (std::holds_alternative<float>(dtVal)) {
                dt = std::get<float>(dtVal);
            }
        }
        
        for (auto& [id, ctx] : physicsContexts) {
            if (ctx) {
                ctx->step(dt);
            }
        }
        
        interp->lastValue = 0.0f;
        return "null";
    }
};

// Character controller storage
static std::unordered_map<int, std::unique_ptr<character::Controller>> controllers;
static int nextControllerId = 1;

class CreateCharacterControllerBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "createCharacterController"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override {
        if (node->args.size() != 1)
            throw std::runtime_error("createCharacterController(mesh)");
        
        Value meshVal = interp->evaluate(node->args[0].get());
        auto meshObj = std::get<std::shared_ptr<ObjectValue>>(meshVal);
        int meshId = std::get<int>(meshObj->fields["_meshId"]);
        
        if (meshes.find(meshId) == meshes.end())
            throw std::runtime_error("Mesh not found");
        
        Mesh* mesh = &meshes[meshId];
        int contextId = 0;
        
        if (physicsContexts.find(contextId) == physicsContexts.end())
            physicsContexts[contextId] = std::make_unique<PhysicsContext>();
        
        PhysicsHandle* handle = physicsContexts[contextId]->attachPhysics(mesh, meshId, 70.0f);
        auto ctrl = character::create(handle, 2.0f, 0.5f, 0.3f);
        
        int ctrlId = nextControllerId++;
        controllers[ctrlId] = std::move(ctrl);
        
        auto obj = std::make_shared<ObjectValue>();
        obj->fields["_meshId"] = meshId;
        obj->fields["_ctrlId"] = ctrlId;
        interp->lastValue = obj;
        return "{object}";
    }
};

class ControllerMoveBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "move"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override {
        if (node->args.size() != 4 || !node->callee)
            throw std::runtime_error("controller.move(forward, right, run, dt)");
        
        auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
        if (!fa) throw std::runtime_error("move must be called on controller object");
        
        Value ctrlVal = interp->evaluate(fa->object.get());
        if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(ctrlVal))
            throw std::runtime_error("Expected controller object");
        
        auto ctrlObj = std::get<std::shared_ptr<ObjectValue>>(ctrlVal);
        
        if (ctrlObj->fields.find("_ctrlId") == ctrlObj->fields.end())
            throw std::runtime_error("Controller object missing _ctrlId field");
        
        int ctrlId = std::get<int>(ctrlObj->fields["_ctrlId"]);
        if (controllers.find(ctrlId) == controllers.end())
            throw std::runtime_error("Controller not found with ID: " + std::to_string(ctrlId));
        
        character::Controller* ctrl = controllers[ctrlId].get();
        
        auto v0 = interp->evaluate(node->args[0].get());
        auto v1 = interp->evaluate(node->args[1].get());
        auto v2 = interp->evaluate(node->args[2].get());
        auto v3 = interp->evaluate(node->args[3].get());
        
        float forward = std::holds_alternative<int>(v0) ? std::get<int>(v0) : std::get<float>(v0);
        float right = std::holds_alternative<int>(v1) ? std::get<int>(v1) : std::get<float>(v1);
        int run = std::holds_alternative<int>(v2) ? std::get<int>(v2) : (int)std::get<float>(v2);
        float dt = std::holds_alternative<int>(v3) ? std::get<int>(v3) : std::get<float>(v3);
        
        float speed = run ? 20.0f : 10.0f;
        
        float rotation = ctrl->handle->mesh->rotation.y;
        
        float mag = std::sqrt(forward * forward + right * right);
        if (mag > 0.0f) {
            forward /= mag;
            right /= mag;
        }
        
        float cosR = std::cos(rotation);
        float sinR = std::sin(rotation);
        Vec3 dir = {
            forward * sinR + right * cosR,
            0,
            forward * cosR - right * sinR
        };
        
        character::move(ctrl, dir, speed);
        interp->lastValue = ctrlObj;
        return "{object}";
    }
};

class ControllerRotateBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "rotate"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override {
        if (node->args.size() != 2 || !node->callee)
            throw std::runtime_error("controller.rotate(direction, dt)");
        
        auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
        if (!fa) throw std::runtime_error("rotate must be called on controller object");
        
        Value ctrlVal = interp->evaluate(fa->object.get());
        if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(ctrlVal))
            throw std::runtime_error("Expected controller object");
        
        auto ctrlObj = std::get<std::shared_ptr<ObjectValue>>(ctrlVal);
        
        if (ctrlObj->fields.find("_ctrlId") == ctrlObj->fields.end())
            throw std::runtime_error("Controller object missing _ctrlId field in rotate");
        
        int ctrlId = std::get<int>(ctrlObj->fields["_ctrlId"]);
        if (controllers.find(ctrlId) == controllers.end())
            throw std::runtime_error("Controller not found in rotate with ID: " + std::to_string(ctrlId));
        
        character::Controller* ctrl = controllers[ctrlId].get();
        
        auto v0 = interp->evaluate(node->args[0].get());
        auto v1 = interp->evaluate(node->args[1].get());
        
        float dir = std::holds_alternative<int>(v0) ? std::get<int>(v0) : std::get<float>(v0);
        float dt = std::holds_alternative<int>(v1) ? std::get<int>(v1) : std::get<float>(v1);
        
        ctrl->handle->mesh->rotation.y += dir * 3.0f * dt;
        
        while (ctrl->handle->mesh->rotation.y > 3.14159f) ctrl->handle->mesh->rotation.y -= 6.28318f;
        while (ctrl->handle->mesh->rotation.y < -3.14159f) ctrl->handle->mesh->rotation.y += 6.28318f;
        
        interp->lastValue = ctrlObj;
        return "{object}";
    }
};

class ControllerJumpBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "jump"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override {
        if (!node->callee)
            throw std::runtime_error("jump must be called on controller");
        
        auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
        if (!fa) throw std::runtime_error("jump: not a field access");
        
        Value ctrlVal = interp->evaluate(fa->object.get());
        if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(ctrlVal))
            throw std::runtime_error("Expected controller object in jump");
        
        auto ctrlObj = std::get<std::shared_ptr<ObjectValue>>(ctrlVal);
        
        if (ctrlObj->fields.find("_ctrlId") == ctrlObj->fields.end())
            throw std::runtime_error("Controller object missing _ctrlId field in jump");
        
        int ctrlId = std::get<int>(ctrlObj->fields["_ctrlId"]);
        if (controllers.find(ctrlId) == controllers.end())
            throw std::runtime_error("Controller not found in jump with ID: " + std::to_string(ctrlId));
        
        character::Controller* ctrl = controllers[ctrlId].get();
        character::jump(ctrl, 10.0f);
        interp->lastValue = ctrlObj;
        return "{object}";
    }
};

class ControllerUpdateBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "update"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override {
        if (node->args.size() != 1 || !node->callee)
            throw std::runtime_error("controller.update(dt)");
        
        auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
        if (!fa) throw std::runtime_error("update must be called on controller object");
        
        Value ctrlVal = interp->evaluate(fa->object.get());
        if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(ctrlVal))
            throw std::runtime_error("Expected controller object in update");
        
        auto ctrlObj = std::get<std::shared_ptr<ObjectValue>>(ctrlVal);
        
        auto v = interp->evaluate(node->args[0].get());
        float dt = std::holds_alternative<int>(v) ? std::get<int>(v) : std::get<float>(v);
        
        for (auto& [id, ctx] : physicsContexts) {
            if (ctx) ctx->step(dt);
        }
        
        interp->lastValue = ctrlObj;
        return "{object}";
    }
};

class ControllerGetPositionBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "getPosition"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override {
        if (!node->callee)
            throw std::runtime_error("getPosition must be called on controller");
        
        auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
        if (!fa) throw std::runtime_error("getPosition: not a field access");
        
        Value ctrlVal = interp->evaluate(fa->object.get());
        if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(ctrlVal))
            throw std::runtime_error("Expected controller object in getPosition");
        
        auto ctrlObj = std::get<std::shared_ptr<ObjectValue>>(ctrlVal);
        
        if (ctrlObj->fields.find("_ctrlId") == ctrlObj->fields.end())
            throw std::runtime_error("Controller object missing _ctrlId field in getPosition");
        
        int ctrlId = std::get<int>(ctrlObj->fields["_ctrlId"]);
        if (controllers.find(ctrlId) == controllers.end())
            throw std::runtime_error("Controller not found in getPosition with ID: " + std::to_string(ctrlId));
        
        character::Controller* ctrl = controllers[ctrlId].get();
        
        auto obj = std::make_shared<ObjectValue>();
        obj->fields["x"] = ctrl->handle->mesh->position.x;
        obj->fields["y"] = ctrl->handle->mesh->position.y;
        obj->fields["z"] = ctrl->handle->mesh->position.z;
        interp->lastValue = obj;
        return "{object}";
    }
};

class ControllerGetRotationBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "getRotation"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override {
        if (!node->callee)
            throw std::runtime_error("getRotation must be called on controller");
        
        auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
        if (!fa) throw std::runtime_error("getRotation: not a field access");
        
        Value ctrlVal = interp->evaluate(fa->object.get());
        if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(ctrlVal))
            throw std::runtime_error("Expected controller object in getRotation");
        
        auto ctrlObj = std::get<std::shared_ptr<ObjectValue>>(ctrlVal);
        
        if (ctrlObj->fields.find("_ctrlId") == ctrlObj->fields.end())
            throw std::runtime_error("Controller object missing _ctrlId field in getRotation");
        
        int ctrlId = std::get<int>(ctrlObj->fields["_ctrlId"]);
        if (controllers.find(ctrlId) == controllers.end())
            throw std::runtime_error("Controller not found in getRotation with ID: " + std::to_string(ctrlId));
        
        character::Controller* ctrl = controllers[ctrlId].get();
        interp->lastValue = ctrl->handle->mesh->rotation.y;
        return std::to_string(ctrl->handle->mesh->rotation.y);
    }
};

class ControllerIsGroundedBuiltin : public BuiltinFunction {
public:
    std::string getName() const override { return "isGrounded"; }
    std::string execute(Interpreter *interp, FunctionCall *node) override {
        if (!node->callee)
            throw std::runtime_error("isGrounded must be called on controller");
        
        auto fa = dynamic_cast<FieldAccess *>(node->callee.get());
        if (!fa) throw std::runtime_error("isGrounded: not a field access");
        
        Value ctrlVal = interp->evaluate(fa->object.get());
        if (!std::holds_alternative<std::shared_ptr<ObjectValue>>(ctrlVal))
            throw std::runtime_error("Expected controller object in isGrounded");
        
        auto ctrlObj = std::get<std::shared_ptr<ObjectValue>>(ctrlVal);
        
        if (ctrlObj->fields.find("_ctrlId") == ctrlObj->fields.end())
            throw std::runtime_error("Controller object missing _ctrlId field in isGrounded");
        
        int ctrlId = std::get<int>(ctrlObj->fields["_ctrlId"]);
        if (controllers.find(ctrlId) == controllers.end())
            throw std::runtime_error("Controller not found in isGrounded with ID: " + std::to_string(ctrlId));
        
        character::Controller* ctrl = controllers[ctrlId].get();
        int grounded = ctrl->handle->isGrounded ? 1 : 0;
        interp->lastValue = grounded;
        return std::to_string(grounded);
    }
};

REGISTER_BUILTIN(CreatePhysicsContextBuiltin)
REGISTER_BUILTIN(AttachPhysicsBuiltin)
REGISTER_BUILTIN(PhysicsStepBuiltin)
REGISTER_BUILTIN(CreateCharacterControllerBuiltin)
REGISTER_BUILTIN(ControllerMoveBuiltin)
REGISTER_BUILTIN(ControllerRotateBuiltin)
REGISTER_BUILTIN(ControllerJumpBuiltin)
REGISTER_BUILTIN(ControllerUpdateBuiltin)
REGISTER_BUILTIN(ControllerGetPositionBuiltin)
REGISTER_BUILTIN(ControllerGetRotationBuiltin)
REGISTER_BUILTIN(ControllerIsGroundedBuiltin)
