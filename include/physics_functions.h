#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>

struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
};

struct Mesh;
class PhysicsHandle;
class PhysicsContext;

class PhysicsHandle {
public:
    Mesh* mesh;
    int meshId;
    Vec3 velocity, angularVelocity, accumulatedForce;
    float mass, drag, angularDrag;
    bool isKinematic, affectedByGravity, isCharacter, isGrounded, ragdollEnabled;
    float groundDrag, ragdollBlend;
    
    PhysicsHandle(Mesh* m, int id) : mesh(m), meshId(id), mass(1.0f), drag(0.1f), angularDrag(0.1f),
                                      isKinematic(false), affectedByGravity(true), isCharacter(false),
                                      isGrounded(false), ragdollEnabled(false), groundDrag(0.95f), ragdollBlend(0.0f) {}
};

class PhysicsContext {
public:
    Vec3 gravity;
    float timeStep, damping;
    std::vector<std::unique_ptr<PhysicsHandle>> bodies;
    
    PhysicsContext(Vec3 g = Vec3(0, -9.81f, 0), float dt = 1.0f/60.0f)
        : gravity(g), timeStep(dt), damping(0.99f) {}
    
    PhysicsHandle* attachPhysics(Mesh* mesh, int meshId, float mass = 1.0f);
    PhysicsHandle* getHandle(int meshId);
    void step(float dt);
    void preStep(float dt) {}
    void postStep() {}
};

namespace motion {
    void moveTowards(PhysicsHandle* handle, Vec3 target, float speed, float stopDistance = 0.1f);
    void slide(PhysicsHandle* handle, Vec3 direction, float speed, float friction = 0.9f);
    void rotateTowards(PhysicsHandle* handle, Vec3 targetDir, float turnSpeed);
    void setVelocity(PhysicsHandle* handle, Vec3 vel);
    void stop(PhysicsHandle* handle);
}

namespace forces {
    void push(PhysicsHandle* handle, Vec3 direction, float strength);
    void explosion(PhysicsContext* ctx, Vec3 origin, float radius, float force);
    void spring(PhysicsHandle* handle, Vec3 anchor, float stiffness, float damping = 0.2f);
    void drag(PhysicsHandle* handle, float coefficient);
    void clear(PhysicsHandle* handle);
}

namespace collision {
    bool isGrounded(PhysicsHandle* handle, PhysicsContext* ctx, float rayDistance = 0.1f);
    float distance(PhysicsHandle* a, PhysicsHandle* b);
    bool overlaps(PhysicsHandle* handle, Vec3 center, float radius);
    
    struct RaycastHit {
        bool hit;
        PhysicsHandle* handle;
        Vec3 point;
        float distance;
    };
    RaycastHit raycast(PhysicsContext* ctx, Vec3 origin, Vec3 direction, float maxDist);
}

namespace constraints {
    void lockPosition(PhysicsHandle* handle, const std::vector<char>& axes);
    void limitRotation(PhysicsHandle* handle, Vec3 minAngles, Vec3 maxAngles);
    void distance(PhysicsHandle* a, PhysicsHandle* b, float targetDist, float stiffness = 0.5f);
}

namespace character {
    struct Controller {
        PhysicsHandle* handle;
        float height, radius, stepHeight, slopeLimit, jumpForce;
        Vec3 moveInput;
        bool wantsToJump;
        
        Controller(PhysicsHandle* h) : handle(h), height(1.8f), radius(0.3f), stepHeight(0.4f),
                                        slopeLimit(45.0f), jumpForce(5.0f), wantsToJump(false) {}
    };
    
    std::unique_ptr<Controller> create(PhysicsHandle* handle, float height = 1.8f, 
                                       float radius = 0.3f, float stepHeight = 0.4f);
    void move(Controller* ctrl, Vec3 inputDir, float speed);
    void jump(Controller* ctrl, float impulse = 5.0f);
    bool isGrounded(Controller* ctrl, PhysicsContext* ctx);
    Vec3 getSurfaceNormal(Controller* ctrl, PhysicsContext* ctx);
}

namespace animation_physics {
    void enableRagdoll(PhysicsHandle* handle);
    void disableRagdoll(PhysicsHandle* handle);
    void blendToAnimation(PhysicsHandle* handle, float t);
    void attachBone(PhysicsHandle* handle, const std::string& boneName, 
                    float boneMass = 2.0f, float swingLimit = 30.0f, float twistLimit = 15.0f);
}

namespace projectiles {
    struct ProjectileConfig {
        Vec3 position, direction;
        float speed, gravity;
        Mesh* mesh;
        std::function<void(collision::RaycastHit)> onHit;
        
        ProjectileConfig() : position(), direction(0, 0, 1), speed(10.0f), gravity(9.81f), mesh(nullptr) {}
    };
    
    PhysicsHandle* spawn(PhysicsContext* ctx, const ProjectileConfig& config);
}

namespace util {
    Vec3 predictTrajectory(Vec3 start, Vec3 velocity, Vec3 gravity, float time);
    Vec3 computeImpulse(float mass, Vec3 desiredVelocity);
    void stabilize(PhysicsHandle* handle);
}

namespace debug_physics {
    void drawColliders(PhysicsContext* ctx);
    void drawForces(PhysicsHandle* handle);
    void freeze(PhysicsHandle* handle);
    void stepOnce(PhysicsContext* ctx);
}