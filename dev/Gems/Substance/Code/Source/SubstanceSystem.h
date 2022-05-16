/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#ifndef CRYINCLUDE_LYSUBSTANCE_SUBSTANCESYSTEM_H
#define CRYINCLUDE_LYSUBSTANCE_SUBSTANCESYSTEM_H
#pragma once

#if defined(AZ_USE_SUBSTANCE)
#include <unordered_set>

#include <Substance/ISubstanceAPI.h>

struct IGraphInstance;

class CProceduralMaterial;
class CProceduralTexture;

/**/
enum class ProceduralMaterialRenderMode
{
    Normal = 1,
    SkipReloadTextures,
};

/**/
class CSubstanceSystem
{
public:
    void Init();
    void Shutdown();
    void Update();

    CProceduralMaterial* GetMaterialFromPath(const char* path) const;
    CProceduralMaterial* GetMaterialFromGraphInstanceID(GraphInstanceID graphInstanceID) const;

    void RemoveMaterial(IProceduralMaterial* pMaterial);

    CProceduralTexture* GetTextureFromPath(const char* path) const;

    void OnMaterialInstanced(CProceduralMaterial* pMaterial);
    void OnMaterialDestroyed(CProceduralMaterial* pMaterial);

    void OnTextureInstanced(const char* path, CProceduralTexture* pTexture);
    void OnTextureDestroyed(CProceduralTexture* pTexture);

    void OnEnterSimulation();
    void OnExitSimulation();
    void OnRuntimeBudgetChanged(bool bApplyChangesImmediately);

    void OnOutputComputed(SubstanceAir::OutputInstance* pOutput, bool bReloadTexture);

    bool QueueRender(CProceduralMaterial* pMaterial);
    bool QueueRender(SubstanceAir::GraphInstance* pGraphInstance);
    bool QueueRender(GraphInstanceID graphInstanceID);

    //! Triggers asynchronous rendering of queued Graphs.
    //! \param  force  If true, all queued Graphs will be sent to the renderer. This is better for accasional updates, where you need to guarantee that the job is submitted.
    //!                If false, each queued Graph will be sent to the renderer only if all prior renders of that Graph have finished. This is better for high frequency 
    //!                updates, to prevent backing up the internal render queue.
    //! \param  mode   Which render mode to use
    //! \return        A handle so you can query for completion (or INVALID_PROCEDURALMATERIALRENDERUID if no Graphs were submitted for rendering)
    ProceduralMaterialRenderUID RenderASync(bool force = true, ProceduralMaterialRenderMode mode = ProceduralMaterialRenderMode::Normal);

    void RenderSync(ProceduralMaterialRenderMode mode = ProceduralMaterialRenderMode::Normal);

    bool HasRenderCompleted(ProceduralMaterialRenderUID uid) const;

    void RenderFence();

    GraphInstanceID EncodeGraphInstanceID(CProceduralMaterial* pMaterial, uint16 graphIndex);
    IGraphInstance* GetGraphInstance(GraphInstanceID graphInstanceID) const;
    SubstanceAir::GraphInstance* GetSubstanceGraphInstance(GraphInstanceID graphInstanceID) const;

    SubstanceAir::OutputInstance::Result GetOutputResult(CProceduralTexture* pTexture);

    //called from Cry3DEngine
    bool LoadTextureData(const char* path, SSubstanceLoadData& loadData);

private:

    //! Triggers rendering of queued Graphs.
    //! \param  runOptions  Bitmask of Renderer::RunOption flags to send to the render.
    //! \param  force       Only applies when runOptions includes Run_Asynchronous.
    //!                     If true, all queued Graphs will be sent to the renderer. This is better for accasional updates, where you need to guarantee that the job is submitted.
    //!                     If false, each queued Graph will be sent to the renderer only if all prior renders of that Graph have finished. This is better for high frequency 
    //!                     updates, to prevent backing up the internal render queue.
    //! \param  mode        Which render mode to use
    //! \return             A handle so you can query for completion. Will be INVALID_PROCEDURALMATERIALRENDERUID if no Graphs were submitted for rendering.
    ProceduralMaterialRenderUID Render(int runOptions, bool force, ProceduralMaterialRenderMode mode);

    void RenderImageInputs();

    void DoOnOutputComputed(SubstanceAir::OutputInstance* pOutput, bool bReloadTexture);

    // Returns true if the GraphInstance is currently pending inside the Substance Renderer;
    // does not necessarily reflect whether the GraphInstance is queued in the CSubstanceSystem
    bool IsGraphInstancePending(SubstanceAir::GraphInstance* graphInstance) const;

private:
    SubstanceAir::Renderer* m_Renderer;

    typedef std::vector<SubstanceAir::GraphInstance*>   GraphInstanceVector;
    GraphInstanceVector m_GraphRenderQueue;

    typedef std::map<SubstanceAir::GraphInstance*, ProceduralMaterialRenderUID>   GraphInstanceRenderUIDMap;
    GraphInstanceRenderUIDMap m_GraphInstanceRenderUIDs;

    typedef std::map<string, CProceduralMaterial*> ProceduralMaterialPathMap;
    ProceduralMaterialPathMap m_ProceduralMaterials;

    typedef std::map<string, CProceduralTexture*> ProceduralTexturePathMap;
    ProceduralTexturePathMap m_ProceduralTextures;

    typedef std::map<ProceduralMaterialID, CProceduralMaterial*> ProceduralMaterialIDMap;
    ProceduralMaterialIDMap m_ProceduralMaterialIDMap;

    typedef std::map<CProceduralTexture*, SubstanceAir::OutputInstance::Result> ProceduralTextureResultMap;
    ProceduralTextureResultMap  m_ProceduralTextureResultMap;

    typedef std::vector<CProceduralTexture*> ProceduralTextureVector;
    ProceduralTextureVector m_ProceduralTextureComputeVector;

    typedef std::unordered_set<ProceduralMaterialRenderUID> ProceduralMaterialRenderUIDSet;
    ProceduralMaterialRenderUIDSet m_ProceduralMaterialRenderUIDPending;

    InputImageMap m_InputImageProcessMap;

    //command messages to enqueue on another thread
    struct SCommandMessage
    {
        void* UserData;
        void (* pfnInvoke)(void*);
    };

    typedef std::vector<SCommandMessage> CommandMessageVector;
    CommandMessageVector m_MainThreadMessages;
    AZStd::mutex m_MainThreadMessagesMutex;
};

extern CSubstanceSystem* pSubstanceSystem;
#endif // AZ_USE_SUBSTANCE
#endif //CRYINCLUDE_LYSUBSTANCE_PROCEDURALTEXTURE_H
