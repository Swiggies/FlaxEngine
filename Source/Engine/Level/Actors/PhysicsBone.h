#pragma once

#include "../Actor.h"

class AnimatedModel;

struct PhysBone {
    String boneName;
    Transform transform;
    Transform localTransform;
    float boneLength;
    Vector3 position;
    Vector3 velocity;
    Vector3 previousPosition;
    Vector3 targetPosition;
    Quaternion targetRotation;
    Transform restTransform;
};

/// <summary>
/// Actor that links to the animated model skeleton node transformation.
/// </summary>
API_CLASS(Attributes = "ActorContextMenu(\"New/Other/Physics Bone\"), ActorToolbox(\"Other\")")
class FLAXENGINE_API PhysicsBone : public Actor
{
    DECLARE_SCENE_OBJECT(PhysicsBone);
private:
    String _node;
    int32 _index;
    AnimatedModel* _model;

    PhysBone _bones[12];

    Transform _lastParentTransform;

public:

    API_FIELD(Attributes = "EditorOrder(20), EditorDisplay(\"Physics Bone\")")
    int _chainLength;

    API_FIELD(Attributes = "EditorOrder(30), EditorDisplay(\"Physics Bone\")")
    float _stiffness = 10.0;

    API_FIELD(Attributes = "EditorOrder(40), EditorDisplay(\"Physics Bone\")")
    float _damping = 10.0;

    API_FIELD(Attributes = "EditorOrder(50), EditorDisplay(\"Physics Bone\")")
    float _mass = 0.1;

    API_FIELD(Attributes = "EditorOrder(60), EditorDisplay(\"Physics Bone\")")
    float _elasticity = 10.0;

    API_FIELD(Attributes = "EditorOrder(70), EditorDisplay(\"Physics Bone\")")
    float _gravityScale = 0.2;

    API_FIELD()
    bool _showDebugLines = false;

    API_FIELD()
    Vector3 _test;
    
    /// <summary>
    /// Gets the target node name to link to it.
    /// </summary>
    API_PROPERTY(Attributes = "EditorOrder(10), EditorDisplay(\"Physics Bone\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.SkeletonNodeEditor\")")
    FORCE_INLINE const String& GetNode() const
    {
        return _node;
    }

    /// <summary>
    /// Sets the target node to link to it.
    /// </summary>
    API_PROPERTY()
    void SetNode(const StringView& name);

    void OnEnable() override;

    void SetupBones();

    void OnDisable() override;

    void OnLateUpdate();
    void OnUpdate();

#if USE_EDITOR
    void OnDebugDraw() override;
#endif

private:
    void ApplyConstraints();
    void ApplyTransforms();
    Transform GetNodeTransform(int node, bool isWorldSpace = true);

public:

    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};
