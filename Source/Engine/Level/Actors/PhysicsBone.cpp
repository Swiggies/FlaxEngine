#include "PhysicsBone.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Level/SceneObjectsFactory.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Scripting/Scripting.h"
#include "AnimatedModel.h"
#include "Engine/Physics/PhysicsScene.h"
#include "Engine/Engine/Time.h"

PhysicsBone::PhysicsBone(const SpawnParams& params)
    : Actor(params)
    , _index(-1)
{
}

void PhysicsBone::SetNode(const StringView& name)
{
    LOG_STR(Warning, TEXT("Testing!"));
    if (_node != name)
    {
        _node = name;
        _index = -1;
    }
}

void PhysicsBone::OnEnable()
{
    GetScene()->Ticking.LateUpdate.AddTick<PhysicsBone, &PhysicsBone::OnLateUpdate>(this);

    const auto parent = dynamic_cast<AnimatedModel*>(GetParent());
    if (parent && parent->SkinnedModel)
    {
        if (_index == -1)
        {
            _index = parent->SkinnedModel->Skeleton.FindNode(_node);
            if (_index == -1)
                return;
            // TODO: maybe track when skinned model gets unloaded to clear cached node _index?
        }

        auto& nodes = parent->GraphInstance.NodesPose;
        Transform t;
        if (nodes.IsValidIndex(_index))
            nodes.Get()[_index].Decompose(t);
        else
            t = parent->SkinnedModel->Skeleton.GetNodeTransform(_index);

        _model = parent;
        _startTransform = _model->SkinnedModel->Skeleton.GetNodeTransform(_index);

        _lastParentTransform = parent->GetTransform().LocalToWorld(_model->SkinnedModel->Skeleton.GetNodeTransform(_index));

        for (size_t i = 0; i < _chainLength; i++)
        {
            _bones[i] = GetNodeTransform(_index + i);
            _velocities[i] = Vector3::Zero;
            _positions[i] = _bones[i].Translation;
            _previousPositions[i] = _bones[i].Translation;
            _targetPositions[i] = _bones[i].Translation;
        }

        // Calculate bone lengths
        for (size_t i = 0; i < _chainLength - 1; i++)
        {
            _boneLengths[i] = Vector3::Distance(_positions[i], _positions[i + 1]);
        }
    }
}

void PhysicsBone::OnDisable()
{
    GetScene()->Ticking.LateUpdate.RemoveTick(this);
}

void PhysicsBone::OnLateUpdate() 
{
    if (_model && _index != -1) 
    {
        _positions[0] = GetNodeTransform(_index).Translation;

        for (size_t i = 0; i < _chainLength; i++)
        {
            // Get Target Positions
            _targetPositions[i] = GetNodeTransform(_index + i).Translation;
        }

        for (size_t i = 1; i < _chainLength; i++)
        {
            // Apply parent movement to all bones first
            Vector3 parentMovement = _targetPositions[i - 1] - _previousPositions[i - 1];

            _targetPositions[i] += parentMovement;

            Vector3 displacement = _targetPositions[i] - _positions[i];
            Vector3 acceleration = (_stiffness * displacement) / _mass;
            acceleration += GetPhysicsScene()->GetGravity() * _gravityScale;

            _velocities[i] += acceleration * Time::GetDeltaTime();
            _velocities[i] *= (1 - _damping * Time::GetDeltaTime());

            _positions[i] += _velocities[i] * Time::GetDeltaTime();
        }
        ApplyConstraints();

        ApplyTransforms();
    }
}

void PhysicsBone::ApplyConstraints()
{
    for (size_t iter = 0; iter < 5; iter++)
    {
        for (size_t i = 1; i < _chainLength; i++)
        {
            Vector3 direction = (_positions[i] - _positions[i - 1]).GetNormalized();
            float desiredLength = _boneLengths[i-1];

            if (direction == Vector3::Zero) 
            {
                direction = (_bones[i].Translation - _bones[i - 1].Translation).GetNormalized();
                if (direction == Vector3::Zero)
                    direction = Vector3::Forward;
            }

            _positions[i] = _positions[i - 1] + direction * _boneLengths[i-1];
        }
    }
}

void PhysicsBone::ApplyTransforms()
{
    for (size_t i = 0; i < _chainLength; i++)
    {
        _bones[i].Translation = _positions[i];
        
        //if (i < _chainLength - 1 && i + 1 < _chainLength)
        //{
        //    Vector3 lookDirection = (_positions[i + 1] - _positions[i]).GetNormalized();
        //    if (lookDirection != Vector3::Zero)
        //    {
        //        Vector3 originalUp = _bones[i].GetUp();
        //        _bones[i].Orientation = Quaternion::LookRotation(lookDirection, originalUp);
        //    }
        //}

        Matrix result;
        Matrix::Transformation(_bones[i].Scale, _bones[i].Orientation, _bones[i].Translation, result);
        _model->SetNodeTransformation(_index + i, result, true);
    }

    for (size_t i = 0; i < _chainLength; i++)
    {
        _previousPositions[i] = GetNodeTransform(_index + i).Translation;
    }

}

Transform PhysicsBone::GetNodeTransform(int node)
{
    Transform value;
    Matrix m;
    _model->GetNodeTransformation(node, m, true);
    m.Decompose(value);
    return value;
}

void PhysicsBone::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(PhysicsBone);

    SERIALIZE_MEMBER(Node, _node);
    SERIALIZE_MEMBER(ChainLength, _chainLength);
    SERIALIZE_MEMBER(Mass, _mass);
    SERIALIZE_MEMBER(Damping, _damping);
    SERIALIZE_MEMBER(Stiffness, _stiffness);
    SERIALIZE_MEMBER(Position, _position);
    SERIALIZE_MEMBER(GravityScale, _gravityScale);
}

void PhysicsBone::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    _index = -1;
    DESERIALIZE_MEMBER(Node, _node);
    DESERIALIZE_MEMBER(ChainLength, _chainLength);
    DESERIALIZE_MEMBER(Mass, _mass);
    DESERIALIZE_MEMBER(Damping, _damping);
    DESERIALIZE_MEMBER(Stiffness, _stiffness);
    DESERIALIZE_MEMBER(Position, _position);
    DESERIALIZE_MEMBER(GravityScale, _gravityScale);
}
