#pragma once
#include <vector>
#include <memory>
#include <unordered_map>

struct Vec3 { float x, y, z; };

struct PhysicsMaterial {
    float friction = 0.5f;
    float restitution = 0.3f;
};

class PhysicsBody {
public:
    Vec3 position{0, 0, 0};
    Vec3 velocity{0, 0, 0};
    Vec3 angularVelocity{0, 0, 0};
    Vec3 force{0, 0, 0};
    Vec3 torque{0, 0, 0};
    float mass = 1.0f;
    float inverseMass = 1.0f;
    bool isStatic = false;
    PhysicsMaterial material;
    int meshId = -1;
    
    void clearForces() { force = {0, 0, 0}; torque = {0, 0, 0}; }
    void addForce(Vec3 f) { force.x += f.x; force.y += f.y; force.z += f.z; }
};

class PhysicsLaw {
public:
    virtual ~PhysicsLaw() = default;
    virtual void apply(std::vector<PhysicsBody*>& bodies) = 0;
};

class GravityLaw : public PhysicsLaw {
    Vec3 direction{0, -1, 0};
    float strength = 9.81f;
public:
    GravityLaw(Vec3 dir, float str) : direction(dir), strength(str) {}
    void apply(std::vector<PhysicsBody*>& bodies) override;
};

class RadialGravityLaw : public PhysicsLaw {
    Vec3 center{0, 0, 0};
    float strength = 0.02f;
public:
    RadialGravityLaw(Vec3 c, float str) : center(c), strength(str) {}
    void apply(std::vector<PhysicsBody*>& bodies) override;
};

class Universe {
    std::vector<std::unique_ptr<PhysicsBody>> bodies;
    std::vector<std::unique_ptr<PhysicsLaw>> laws;
    float timeStep = 1.0f / 60.0f;
public:
    int addBody(int meshId, float mass, bool isStatic);
    PhysicsBody* getBody(int bodyId);
    void addLaw(std::unique_ptr<PhysicsLaw> law);
    void step(float dt);
    void integrate(float dt);
};
