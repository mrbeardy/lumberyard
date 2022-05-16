/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "Substance_precompiled.h"

#include "SubstanceGemSystemComponent.h"
#include "ProceduralMaterialHandle.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

//////////////////////////////////////////////////////////////////////////

void SubstanceGemSystemComponent::Reflect(AZ::ReflectContext* context)
{
    ProceduralMaterialHandle::Reflect(context);

    if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serialize->Class<SubstanceGemSystemComponent, AZ::Component>()
            ->Version(1);
    }
}

void SubstanceGemSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
{
    provided.push_back(AZ_CRC("SubstanceService", 0xbd6fe2f5));
}

void SubstanceGemSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
{
    incompatible.push_back(AZ_CRC("SubstanceService", 0xbd6fe2f5));
}

void SubstanceGemSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
{
    (void)required;
}

void SubstanceGemSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
{
    (void)dependent;
}

void SubstanceGemSystemComponent::Activate()
{
#if defined(AZ_USE_SUBSTANCE)
    m_SubstanceLibAPI = nullptr;
    SubstanceRequestBus::Handler::BusConnect();
#endif // AZ_USE_SUBSTANCE
}

void SubstanceGemSystemComponent::Deactivate()
{
#if defined(AZ_USE_SUBSTANCE)
    m_SubstanceLibAPI = nullptr;
    SubstanceRequestBus::Handler::BusDisconnect();
#endif // AZ_USE_SUBSTANCE
}

bool SubstanceGemSystemComponent::CheckInitialized() const
{
#if defined(AZ_USE_SUBSTANCE)
    if (m_SubstanceLibAPI)
    {
        return true;
    }
    else
#endif // AZ_USE_SUBSTANCE
    {
        AZ_Assert(false, "SubstanceGemSystemComponent::m_SubstanceLibAPI isn't initialized");
        return false;
    }
}

#if defined(AZ_USE_SUBSTANCE)

int SubstanceGemSystemComponent::GetMinimumOutputSize() const
{
    if (!CheckInitialized())
    {
        return 0;
    }

    return m_SubstanceLibAPI->GetMinimumOutputSize();
}

int SubstanceGemSystemComponent::GetMaximumOutputSize() const
{
    if (!CheckInitialized())
    {
        return 0;
    }

    return m_SubstanceLibAPI->GetMaximumOutputSize();
}

IProceduralMaterial* SubstanceGemSystemComponent::GetMaterialFromPath(const char* path, bool bForceLoad) const
{
    if (!CheckInitialized())
    {
        return nullptr;
    }

    return m_SubstanceLibAPI->GetMaterialFromPath(CryStringUtils::ToLower(path).c_str(), bForceLoad);
}

IGraphInstance* SubstanceGemSystemComponent::GetGraphInstance(GraphInstanceID graphInstanceID) const
{
    if (!CheckInitialized())
    {
        return nullptr;
    }

    return m_SubstanceLibAPI->GetGraphInstance(graphInstanceID);
}

bool SubstanceGemSystemComponent::QueueRender(IGraphInstance* pGraphInstance)
{
    if (!CheckInitialized())
    {
        return false;
    }

    return m_SubstanceLibAPI->QueueRender(pGraphInstance);
}

ProceduralMaterialRenderUID SubstanceGemSystemComponent::RenderASync(bool force)
{
    if (!CheckInitialized())
    {
        return INVALID_PROCEDURALMATERIALRENDERUID;
    }

    return m_SubstanceLibAPI->RenderASync(force);
}

void SubstanceGemSystemComponent::RenderSync()
{
    if (!CheckInitialized())
    {
        return;
    }

    m_SubstanceLibAPI->RenderSync();
}

bool SubstanceGemSystemComponent::HasRenderCompleted(ProceduralMaterialRenderUID uid) const
{
    if (!CheckInitialized())
    {
        return true;
    }

    return m_SubstanceLibAPI->HasRenderCompleted(uid);
}

bool SubstanceGemSystemComponent::CreateProceduralMaterial(const char* sbsarPath, const char* smtlPath)
{
    if (!CheckInitialized())
    {
        return false;
    }

    return m_SubstanceLibAPI->CreateProceduralMaterial(sbsarPath, smtlPath);
}

bool SubstanceGemSystemComponent::SaveProceduralMaterial(IProceduralMaterial* pMaterial, const char* path)
{
    if (!CheckInitialized())
    {
        return false;
    }

    return m_SubstanceLibAPI->SaveProceduralMaterial(pMaterial, path);
}

void SubstanceGemSystemComponent::RemoveProceduralMaterial(IProceduralMaterial* pMaterial)
{
    if (!CheckInitialized())
    {
        return;
    }

    m_SubstanceLibAPI->RemoveProceduralMaterial(pMaterial);
}

ISubstanceLibAPI* SubstanceGemSystemComponent::GetSubstanceLibAPI() const
{
    return m_SubstanceLibAPI;
}

void SubstanceGemSystemComponent::SetSubstanceLibAPI(ISubstanceLibAPI* api)
{
    m_SubstanceLibAPI = api;
}

#endif // AZ_USE_SUBSTANCE


