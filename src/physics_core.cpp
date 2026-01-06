#include "include/physics_core.h"
#include <cmath>

void GravityLaw::apply(std::vector<PhysicsBody*>& bodies) {
    for (auto* body : bodies) {
        if (!body->isStatic) {
            body->addForce({direction.x * strength * body->mass, 
                           direction.y * strength * body->mass, 
                           direction.z * strength * body->mass});
        }
    }
}

void RadialGravityLaw::apply(std::vector<PhysicsBody*>& bodies) {
    for (auto* body : bodies) {
        if (!body->isStatic) {
            Vec3 dir = {center.x - body->position.x, center.y - body->position.y, center.z - body->position.z};
            float dist = sqrt(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
            if (dist > 0.001f) {
                dir.x /= dist; dir.y /= dist; dir.z /= dist;
                body->addForce({dir.x * strength * body->mass, 
                               dir.y * strength * body->mass, 
                               dir.z * strength * body->mass});
            }
        }
    }
}

int Universe::addBody(int meshId, float mass, bool isStatic) {
    auto body = std::make_unique<PhysicsBody>();
    body->meshId = meshId;
    body->mass = mass;
    body->isStatic = isStatic;
    body->inverseMass = (mass > 0 && !isStatic) ? 1.0f / mass : 0.0f;
    bodies.push_back(std::move(body));
    return bodies.size() - 1;
}

PhysicsBody* Universe::getBody(int bodyId) {
    if (bodyId >= 0 && bodyId < (int)bodies.size()) return bodies[bodyId].get();
    return nullptr;
}

void Universe::addLaw(std::unique_ptr<PhysicsLaw> law) {
    laws.push_back(std::move(law));
}

void Universe::integrate(float dt) {
    for (auto& body : bodies) {
        if (!body->isStatic) {
            body->velocity.x += body->force.x * body->inverseMass * dt;
            body->velocity.y += body->force.y * body->inverseMass * dt;
            body->velocity.z += body->force.z * body->inverseMass * dt;
            body->position.x += body->velocity.x * dt;
            body->position.y += body->velocity.y * dt;
            body->position.z += body->velocity.z * dt;
        }
    }
}

void Universe::step(float dt) {
    std::vector<PhysicsBody*> bodyPtrs;
    for (auto& b : bodies) bodyPtrs.push_back(b.get());
    
    for (auto& body : bodies) body->clearForces();
    for (auto& law : laws) law->apply(bodyPtrs);
    integrate(dt);
}
