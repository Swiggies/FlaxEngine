#include "PhysicsBone.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Level/SceneObjectsFactory.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Scripting/Scripting.h"
#include "AnimatedModel.h"
#include "Engine/Physics/PhysicsScene.h"
#include "Engine/Engine/Time.h"
#include "Engine/Profiler/ProfilerCPU.h"

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
        SetupBones();
    }
}

void PhysicsBone::OnEnable()
{
    GetScene()->Ticking.LateUpdate.AddTick<PhysicsBone, &PhysicsBone::OnLateUpdate>(this);

    SetupBones();
}

void PhysicsBone::OnDisable()
{
    GetScene()->Ticking.Update.RemoveTick(this);
}

void PhysicsBone::SetupBones()
{
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

        _lastParentTransform = parent->GetTransform().LocalToWorld(_model->SkinnedModel->Skeleton.GetNodeTransform(_index));

        for (size_t i = 0; i < _chainLength; i++)
        {
            _bones[i].boneName = _model->SkinnedModel->Skeleton.Nodes[_index + i].Name;
            _bones[i].transform = GetNodeTransform(_index + i);
            _bones[i].position = _bones[i].transform.Translation;
            _bones[i].previousPosition = _bones[i].transform.Translation;
            _bones[i].velocity = Vector3::Zero;
            _bones[i].targetPosition = Vector3::Zero;
        }

        // Calculate bone lengths
        for (size_t i = 0; i < _chainLength - 1; i++)
        {
            _bones[i].boneLength = Vector3::Distance(_bones[i].position, _bones[i + 1].position);
        }
        _bones[_chainLength].boneLength = 0;

        // Debug
        for (size_t i = 1; i < _chainLength; i++)
        {
            Transform localPos = _bones[i].transform;

            localPos = _bones[i-1].transform.WorldToLocal(localPos);

            _bones[i].localTransform = localPos;

            LOG(Info, "{0} | Local Position: {1}", _bones[i].boneName, localPos);
        }
    }
}

void PhysicsBone::OnUpdate() 
{

}

void PhysicsBone::OnLateUpdate() 
{
    PROFILE_CPU_NAMED("PhysBone");
    if (_model && _index != -1) 
    {

        for (size_t i = 0; i < _chainLength; i++)
        {
            Transform nodeTransform = GetNodeTransform(_index + i);
            if (i == 0) _bones[0].position = nodeTransform.Translation;

            // Get Target Positions
            _bones[i].targetPosition = nodeTransform.Translation;
            _bones[i].restTransform = nodeTransform;
        }

        for (size_t i = 1; i < _chainLength; i++)
        {
            // Apply parent movement to all bones first
            Vector3 parentMovement = _bones[i - 1].targetPosition - _bones[i - 1].previousPosition;

            _bones[i].targetPosition += parentMovement;

            Vector3 displacement = _bones[i].targetPosition - _bones[i].position;
            Vector3 acceleration = (_stiffness * displacement) / _mass;
            acceleration += GetPhysicsScene()->GetGravity() * _gravityScale;

            _bones[i].velocity += acceleration * Time::GetDeltaTime();
            _bones[i].velocity *= (1 - _damping * Time::GetDeltaTime());

            Vector3 restDisplacement = _bones[i].restTransform.Translation - _bones[i].position;
            _bones[i].velocity += restDisplacement * (_elasticity * Time::GetDeltaTime());

            _bones[i].position += _bones[i].velocity * Time::GetDeltaTime();

            float angle = Quaternion::AngleBetween(_bones[i].transform.Orientation, _bones[i].restTransform.Orientation);
            //Quaternion::Slerp(_bones[i].transform.Orientation, _bones[i].restTransform.Orientation, angle * _elasticity, _bones[i].targetRotation);
        }
        ApplyConstraints();

        ApplyTransforms();
    }
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void PhysicsBone::OnDebugDraw()
{
    if (_showDebugLines) 
    {
        for (size_t i = 1; i < _chainLength; i++)
        {
            PhysBone pb = _bones[i];

            Ray ray = Ray(pb.transform.Translation, pb.transform.GetForward().GetNormalized());
            Ray ray2 = Ray(pb.transform.Translation, pb.transform.GetUp().GetNormalized());
            Ray ray3 = Ray(pb.transform.Translation, pb.transform.GetRight().GetNormalized());
            DEBUG_DRAW_RAY(ray, Color::Blue, pb.boneLength, 0, false);
            DEBUG_DRAW_RAY(ray2, Color::Green, pb.boneLength, 0, false);
            DEBUG_DRAW_RAY(ray3, Color::Red, pb.boneLength, 0, false);

            //Vector3 restPos = _bones[i - 1].tar.LocalToWorld(_bones[i].localTransform).Translation;

            BoundingSphere sphere = BoundingSphere(_bones[i].restTransform.Translation, 1);
            DEBUG_DRAW_SPHERE(sphere, Color::Red, 0, false);
        }
    }
}

#endif

void PhysicsBone::ApplyConstraints()
{
    for (size_t iter = 0; iter < 5; iter++)
    {
        for (size_t i = 1; i < _chainLength; i++)
        {
            Vector3 direction = (_bones[i].position - _bones[i - 1].position).GetNormalized();
            float desiredLength = _bones[i-1].boneLength;

            if (direction == Vector3::Zero) 
            {
                direction = (_bones[i].transform.Translation - _bones[i - 1].transform.Translation).GetNormalized();
                if (direction == Vector3::Zero)
                    direction = Vector3::Forward;
            }

            _bones[i].position = _bones[i - 1].position + direction * _bones[i-1].boneLength;
        }
    }
}

void PhysicsBone::ApplyTransforms()
{
    // Calculate inverse matrix
    // Outside loop so we only do it once
    Matrix world;
    _model->GetLocalToWorldMatrix(world);
    Matrix invWorld;
    Matrix::Invert(world, invWorld);


    for (size_t i = 1; i < _chainLength; ++i)
    {
        if (i > 0) 
        {
            //LOG(Info, "{0}", _bones[i].localTransform.Translation);
            Vector3 transformedPos = _bones[i - 1].transform.LocalToWorldVector(_bones[i].localTransform.Translation);

            Transform dirToLastBone;
            dirToLastBone.Translation = (_bones[i].position - _bones[i - 1].position);

            Quaternion targetRot = Quaternion::GetRotationFromTo(transformedPos, dirToLastBone.Translation, Vector3::Zero);

            _bones[i-1].transform.Orientation = targetRot * _bones[i-1].transform.Orientation;
        }
        _bones[i].transform.Translation = _bones[i].position;

        Matrix result;
        Matrix::Transformation(_bones[i].transform.Scale, _bones[i].transform.Orientation, _bones[i].transform.Translation, result);

        _model->SetNodeTransformation(_index + i, result * invWorld);
    }

    for (size_t i = 0; i < _chainLength; i++)
    {
        _bones[i].previousPosition = GetNodeTransform(_index + i).Translation;
    }

}

Transform PhysicsBone::GetNodeTransform(int node, bool isWorldSpace)
{
    Transform value;
    Matrix m;
    _model->GetNodeTransformation(node, m, isWorldSpace);
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
    DESERIALIZE_MEMBER(GravityScale, _gravityScale);
}

