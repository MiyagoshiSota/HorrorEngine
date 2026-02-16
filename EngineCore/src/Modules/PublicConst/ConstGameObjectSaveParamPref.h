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
    static inline const char* kMeshRendererMaterialColorOverrides = "materialColorOverrides";
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
    static inline const char* kColliderOffset = "offset";

    // Trigger Component 内
    static inline const char* kTriggerCondition = "Trigger";
    static inline const char* kTriggerConditionName = "name";
    static inline const char* kTriggerAction = "Action";
    static inline const char* kTriggerActionName = "name";
    static inline const char* kTriggerActions = "Actions";
    static inline const char* kTriggerTaskName = "taskName";

    // PLayerController内
    static inline const char* kPlayerControllerMoveSpeed = "MoveSpeed";

    // Works (Day/Scene に紐づく)
    static inline const char* kWorks = "works";
    static inline const char* kWorkName = "name";
    static inline const char* kWorkStartCondition = "startCondition";
    static inline const char* kWorkRewardActions = "rewardActions";
    static inline const char* kWorkflows = "workflows";
    static inline const char* kWorkflowName = "name";
    static inline const char* kWorkflowMode = "mode";
    static inline const char* kWorkflowTasks = "tasks";
    static inline const char* kTaskRefGameObjectName = "gameObjectName";
    static inline const char* kTaskRefTriggerIndex = "triggerIndex";
    static inline const char* kWorkflowModeSequential = "Sequential";
    static inline const char* kWorkflowModeParallel = "Parallel";
    static inline const char* kRewardActionType = "type";
    static inline const char* kRewardActionWorkName = "workName";
    static inline const char* kRewardActionGameObjectName = "gameObjectName";
    static inline const char* kRewardActionPlacePosition = "placePosition";
    static inline const char* kRewardActionPlaceTargetObject = "placeTargetObject";
    static inline const char* kRewardActionSoundName = "soundName";
    static inline const char* kRewardActionSoundUse3d = "use3d";
};
