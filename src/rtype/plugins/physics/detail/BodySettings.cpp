/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** BodySettings
*/

#include "plugins/physics/detail/BodySettings.hpp"

#include <Jolt/Jolt.h>  // IWYU pragma: keep
#include <Jolt/Core/Reference.h>
#include <Jolt/Math/Quat.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Physics/Body/AllowedDOFs.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/MotionQuality.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSettings.h>

#include <algorithm>
#include <cstdint>
#include <format>
#include <glm/ext/vector_float2.hpp>

#include "plugins/physics/Collider.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/physics/detail/JoltMath.hpp"
#include "plugins/physics/detail/Layers.hpp"
#include "plugins/physics/exceptions/PhysicsExceptions.hpp"
#include "plugins/transform/Transform.hpp"

namespace rtype::plugins::physics::detail {
namespace {
/// @brief Turns Jolt's result into the shape, or throws its error.
/// @throws exceptions::PhysicsException When Jolt refused the shape.
JPH::RefConst<JPH::Shape> unwrap(const JPH::Shape::ShapeResult& result) {
    if (result.HasError()) {
        throw exceptions::PhysicsException{std::format("Invalid collider shape: {}", result.GetError())};
    }
    return result.Get();
}

/// @throws exceptions::PhysicsException When the value is not positive.
void requirePositive(float value, const char* name) {
    if (!(value > 0.0F)) {
        throw exceptions::PhysicsException{std::format("Collider::{} must be positive, got {}", name, value)};
    }
}

/// @return The shape before its offset, in meters.
JPH::RefConst<JPH::Shape> makeCenteredShape(const Collider& collider, float unitsPerMeter) {
    switch (collider.shape) {
        case ShapeType::kBox: {
            requirePositive(collider.size.x, "size.x");
            requirePositive(collider.size.y, "size.y");
            const glm::vec2 half = collider.size / (2.0F * unitsPerMeter);
            const float depth = std::min(half.x, half.y);
            const float convexRadius = std::min(JPH::cDefaultConvexRadius, depth);
            return unwrap(JPH::BoxShapeSettings{JPH::Vec3{half.x, half.y, depth}, convexRadius}.Create());
        }
        case ShapeType::kCircle:
            requirePositive(collider.radius, "radius");
            return unwrap(JPH::SphereShapeSettings{collider.radius / unitsPerMeter}.Create());
        case ShapeType::kCapsule:
            requirePositive(collider.radius, "radius");
            requirePositive(collider.halfHeight, "halfHeight");
            return unwrap(
                JPH::CapsuleShapeSettings{collider.halfHeight / unitsPerMeter, collider.radius / unitsPerMeter}
                    .Create());
    }
    throw exceptions::PhysicsException{"Unknown Collider::shape"};
}

/// @return The Jolt motion type of a body type.
JPH::EMotionType toMotionType(BodyType type) noexcept {
    switch (type) {
        case BodyType::kStatic:
            return JPH::EMotionType::Static;
        case BodyType::kKinematic:
            return JPH::EMotionType::Kinematic;
        case BodyType::kDynamic:
            return JPH::EMotionType::Dynamic;
    }
    return JPH::EMotionType::Static;
}
}  // namespace

JPH::RefConst<JPH::Shape> makeShape(const Collider& collider, float unitsPerMeter) {
    JPH::RefConst<JPH::Shape> shape = makeCenteredShape(collider, unitsPerMeter);
    if (collider.offset == glm::vec2{0.0F, 0.0F}) {
        return shape;
    }
    return unwrap(
        JPH::RotatedTranslatedShapeSettings{toJolt2D(collider.offset / unitsPerMeter), JPH::Quat::sIdentity(), shape}
            .Create());
}

JPH::BodyCreationSettings makeBodySettings(const Collider& collider, const RigidBody* body,
                                           const transform::Transform& transform, float unitsPerMeter,
                                           std::uint64_t userData) {
    const JPH::EMotionType motion = body != nullptr ? toMotionType(body->type) : JPH::EMotionType::Static;
    const JPH::ObjectLayer layer = motion == JPH::EMotionType::Static ? layers::kNonMoving : layers::kMoving;
    JPH::BodyCreationSettings settings{makeShape(collider, unitsPerMeter), toJolt2D(transform.position / unitsPerMeter),
                                       toJoltRotation2D(transform.rotation), motion, layer};
    settings.mUserData = userData;
    settings.mIsSensor = collider.sensor;
    settings.mFriction = collider.friction;
    settings.mRestitution = collider.restitution;
    if (motion == JPH::EMotionType::Static) {
        return settings;
    }
    settings.mAllowedDOFs = body->fixedRotation ? JPH::EAllowedDOFs::TranslationX | JPH::EAllowedDOFs::TranslationY
                                                : JPH::EAllowedDOFs::Plane2D;
    settings.mLinearDamping = body->linearDamping;
    settings.mAngularDamping = body->angularDamping;
    settings.mGravityFactor = body->gravityScale;
    settings.mMotionQuality =
        body->continuousCollision ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete;
    if (motion == JPH::EMotionType::Dynamic) {
        if (!(body->mass > 0.0F)) {
            throw exceptions::PhysicsException{std::format("RigidBody::mass must be positive, got {}", body->mass)};
        }
        settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
        settings.mMassPropertiesOverride.mMass = body->mass;
    }
    return settings;
}

}  // namespace rtype::plugins::physics::detail
