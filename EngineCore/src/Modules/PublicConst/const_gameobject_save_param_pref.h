#pragma once

class const_gameobject_save_param_pref
{
public:
    static inline const char* SceneName = "scene_name";
    static inline const char* GameObjects = "gameObjects";
    static inline const char* GameObjectName = "name";

    static inline const char* TransformPosition = "position";
    static inline const char* TransformRotation = "rotation";
    static inline const char* TransformScale = "scale";

    // Component
    static inline const char* Components = "components";
    static inline const char* ComponentType = "type";
    static inline const char* ComponentMeshRenderer = "MeshRenderer";
    static inline const char* ComponentRigidbody = "Rigidbody";
    static inline const char* ComponentTrigger = "Trigger";
    static inline const char* ComponentPlayerController = "PlayerController";

    // Physics (Rigidbody Component 内)
    static inline const char* RigidbodyIsGravityEnabled = "isGravityEnabled";
    static inline const char* RigidbodyBodyType = "bodyType";
    static inline const char* RigidbodyCollider = "collider";
    static inline const char* ColliderShape = "shape";
    static inline const char* ColliderShapeBox = "Box";
    static inline const char* ColliderShapeSphere = "Sphere";
    static inline const char* ColliderShapeCapsule = "Capsule";
    static inline const char* ColliderBoxHalfExtents = "halfExtents";
    static inline const char* ColliderSphereRadius = "radius";
    static inline const char* ColliderCapsuleRadius = "radius";
    static inline const char* ColliderCapsuleHeight = "height";
    static inline const char* ColliderIsTrigger = "isTrigger";

    // Trigger Component 内
    static inline const char* TriggerCondition = "Trigger";
    static inline const char* TriggerConditionName = "name";
    static inline const char* TriggerAction = "Action"; 
    static inline const char* TriggerActionName = "name";
    static inline const char* TriggerActions = "Actions";

    // PLayerController内
    static inline const char* PlayerControllerMoveSpeed = "MoveSpeed";
};
