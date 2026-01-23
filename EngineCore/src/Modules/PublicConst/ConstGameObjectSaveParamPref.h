#pragma once

class ConstGameObjectSaveParamPref
{
public:
    static inline const char* kSceneName = "scene_name";
    static inline const char* kGameObjects = "gameObjects";
    static inline const char* kGameObjectName = "name";

    static inline const char* kTransformPosition = "position";
    static inline const char* kTransformRotation = "rotation";
    static inline const char* kTransformScale = "scale";

    // Component
    static inline const char* kComponents = "components";
    static inline const char* kComponentType = "type";
    static inline const char* kComponentMeshRenderer = "MeshRenderer";
    static inline const char* kComponentRigidbody = "Rigidbody";
    static inline const char* kComponentTrigger = "Trigger";
    static inline const char* kComponentPlayerController = "PlayerController";

    // Physics (Rigidbody Component 内)
    static inline const char* kRigidbodyIsGravityEnabled = "isGravityEnabled";
    static inline const char* kRigidbodyBodyType = "bodyType";
    static inline const char* kRigidbodyCollider = "collider";
    static inline const char* kColliderShape = "shape";
    static inline const char* kColliderShapeBox = "Box";
    static inline const char* kColliderShapeSphere = "Sphere";
    static inline const char* kColliderShapeCapsule = "Capsule";
    static inline const char* kColliderBoxHalfExtents = "halfExtents";
    static inline const char* kColliderSphereRadius = "radius";
    static inline const char* kColliderCapsuleRadius = "radius";
    static inline const char* kColliderCapsuleHeight = "height";
    static inline const char* kColliderIsTrigger = "isTrigger";

    // Trigger Component 内
    static inline const char* kTriggerCondition = "Trigger";
    static inline const char* kTriggerConditionName = "name";
    static inline const char* kTriggerAction = "Action"; 
    static inline const char* kTriggerActionName = "name";
    static inline const char* kTriggerActions = "Actions";

    // PLayerController内
    static inline const char* kPlayerControllerMoveSpeed = "MoveSpeed";
};
