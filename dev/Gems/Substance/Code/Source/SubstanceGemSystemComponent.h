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
/** @file SubstanceGem.h
    @brief Header for the Substance Gem Implementation.
    @author Josh Coyne - Allegorithmic (josh.coyne@allegorithmic.com)
    @date 09-14-2015
    @copyright Allegorithmic. All rights reserved.
*/

#pragma once


#include "Substance/SubstanceBus.h"
#include "SubstanceAPI.h"
#include <AzCore/Component/Component.h>

class SubstanceGemSystemComponent
    : public AZ::Component
    , public SubstanceRequestBus::Handler
{
public:
    AZ_COMPONENT(SubstanceGemSystemComponent, "{DB6FC7E8-7B16-43A9-AC9B-8D973626FB41}");

    static void Reflect(AZ::ReflectContext* context);
    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
    static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    ////////////////////////////////////////////////////////////////////////
    // AZ::Component interface implementation
    void Activate() override;
    void Deactivate() override;
    ////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////
    // SubstanceRequestBus::Handler interface implementation
#if defined(AZ_USE_SUBSTANCE)
    int GetMinimumOutputSize() const override;
    int GetMaximumOutputSize() const override;

    IProceduralMaterial* GetMaterialFromPath(const char* path, bool bForceLoad) const override;
    IGraphInstance* GetGraphInstance(GraphInstanceID graphInstanceID) const override;

    bool QueueRender(IGraphInstance* pGraphInstance) override;
    ProceduralMaterialRenderUID RenderASync(bool force) override;
    void RenderSync() override;
    bool HasRenderCompleted(ProceduralMaterialRenderUID uid) const override;

    bool CreateProceduralMaterial(const char* sbsarPath, const char* smtlPath) override;
    bool SaveProceduralMaterial(IProceduralMaterial* pMaterial, const char* path) override;
    void RemoveProceduralMaterial(IProceduralMaterial* pMaterial) override;

    ISubstanceLibAPI* GetSubstanceLibAPI() const override;
    void SetSubstanceLibAPI(ISubstanceLibAPI*) override;
#endif // AZ_USE_SUBSTANCE
    ////////////////////////////////////////////////////////////////////////

protected:

    bool CheckInitialized() const;


private:
#if defined(AZ_USE_SUBSTANCE)
    ISubstanceLibAPI* m_SubstanceLibAPI = nullptr;
#endif // AZ_USE_SUBSTANCE
};

