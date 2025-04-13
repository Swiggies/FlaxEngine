#pragma once

#include "../Actor.h"

class AnimatedModel;

/// <summary>
/// Actor that links to the animated model skeleton node transformation.
/// </summary>
API_CLASS(Attributes = "ActorContextMenu(\"New/Other/Physics BOne\"), ActorToolbox(\"Other\")")
class FLAXENGINE_API PhysicsBone : public Actor
{
    DECLARE_SCENE_OBJECT(PhysicsBone);
private:
    String _node;
    int32 _index;
    AnimatedModel* _model;

    Transform _startTransform;

    Transform _bones[12];
    float _boneLengths[12];
    Vector3 _positions[12];
    Vector3 _velocities[12];
    Vector3 _previousPositions[12];
    Vector3 _targetPositions[12];

    Transform _lastParentTransform;

public:
    
    API_FIELD()
    Vector3 _position;

    API_FIELD()
    int _chainLength;

    API_FIELD()
    float _stiffness = 50.0;

    API_FIELD()
    float _damping = 3.0;

    API_FIELD()
    float _mass = 0.1;

    API_FIELD()
    float _gravityScale = 0.2;

    /// <summary>
    /// Gets the target node name to link to it.
    /// </summary>
    API_PROPERTY(Attributes = "EditorOrder(10), EditorDisplay(\"Bone Socket\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.SkeletonNodeEditor\")")
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

    void OnDisable() override;

    void OnLateUpdate();

private:
    void ApplyConstraints();
    void ApplyTransforms();
    Transform GetNodeTransform(int node);

public:

    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};
