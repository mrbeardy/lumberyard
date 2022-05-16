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

#include "Substance_precompiled.h"

#if defined(AZ_USE_SUBSTANCE)
#include "SubstanceSystem.h"
#include "ProceduralMaterial.h"
#include "ProceduralTexture.h"

#include <Substance/SubstanceBus.h>

#include "IImage.h"
#include <Substance_Traits_Platform.h>


//--------------------------------------------------------------------------------------------
class GlobalCallbacksImpl
    : public SubstanceAir::GlobalCallbacks
{
public:
    virtual unsigned int getEnabledMask() const override
    {
        return Enable_UserAlloc;
    }

    virtual void* memoryAlloc(size_t bytesCount, size_t alignment) override
    {
        return CryModuleMemalign(bytesCount, alignment);
    }

    virtual void memoryFree(void* bufferPtr) override
    {
        CryModuleMemalignFree(bufferPtr);
    }
};

//--------------------------------------------------------------------------------------------
class RenderCallbacksImpl
    : public SubstanceAir::RenderCallbacks
{
public:
    virtual void outputComputed(SubstanceAir::UInt runUid, size_t userData, const SubstanceAir::GraphInstance* graphInstance, SubstanceAir::OutputInstance* outputInstance) override
    {
        switch ((ProceduralMaterialRenderMode)userData)
        {
        case ProceduralMaterialRenderMode::Normal:
            pSubstanceSystem->OnOutputComputed(outputInstance, true);
            break;
        case ProceduralMaterialRenderMode::SkipReloadTextures:
            pSubstanceSystem->OnOutputComputed(outputInstance, false);
            break;
        }
    }

    virtual void outputComputed(SubstanceAir::UInt runUid, const SubstanceAir::GraphInstance* graphInstance, SubstanceAir::OutputInstance* outputInstance) override
    {
        RenderCallbacksImpl::outputComputed(runUid, 0, graphInstance, outputInstance);
    }
};

//--------------------------------------------------------------------------------------------
static SubstanceAir::RenderOptions CreateRenderOptions()
{
    SubstanceAir::RenderOptions renderOpt;
    renderOpt.mCoresCount = gAPI->GetCoreCount();
    renderOpt.mMemoryBudget = MEM_MB(gAPI->GetMemoryBudget());
    return renderOpt;
}

//--------------------------------------------------------------------------------------------
void CSubstanceSystem::Init()
{
    //set callbacks
    static GlobalCallbacksImpl __globalCallbacks;
    SubstanceAir::GlobalCallbacks::setInstance(&__globalCallbacks);

    //if we're loading into editor, start with maximum CPU/Memory
    SubstanceAir::RenderOptions renderOpt = gAPI->IsEditor() ? SubstanceAir::RenderOptions() : CreateRenderOptions();
    m_Renderer = AIR_NEW(SubstanceAir::Renderer)(renderOpt, gAPI->GetSubstanceLibrary());

    static RenderCallbacksImpl sRenderCallbacksImpl;
    m_Renderer->setRenderCallbacks(&sRenderCallbacksImpl);
}

void CSubstanceSystem::Shutdown()
{
    //copy the material Map because material deletion modifies this map object
    ProceduralMaterialPathMap materialMapCopy = m_ProceduralMaterials;
    for (auto iter = materialMapCopy.begin(); iter != materialMapCopy.end(); iter++)
    {
        delete iter->second;
    }

    m_ProceduralTextureComputeVector.clear();

    m_ProceduralTextureResultMap.clear();

    AIR_DELETE(m_Renderer);
}

void CSubstanceSystem::Update()
{
    //process messages
    AZStd::lock_guard<AZStd::mutex> lock(m_MainThreadMessagesMutex);
    for (auto iter = m_MainThreadMessages.begin(); iter != m_MainThreadMessages.end(); iter++)
    {
        SCommandMessage& cmdMsg = *iter;
        cmdMsg.pfnInvoke(cmdMsg.UserData);
    }
    m_MainThreadMessages.clear();

    //upload texture changes
    for (auto iter = m_ProceduralTextureComputeVector.begin(); iter != m_ProceduralTextureComputeVector.end(); iter++)
    {
        (*iter)->OnOutputComputed();
    }
    m_ProceduralTextureComputeVector.clear();

    //clear render pending list
    ProceduralMaterialRenderUIDSet removeUIDSet;
    for (auto iter = m_ProceduralMaterialRenderUIDPending.begin(); iter != m_ProceduralMaterialRenderUIDPending.end(); iter++)
    {
        if (!m_Renderer->isPending(*iter))
        {
            removeUIDSet.emplace(*iter);
        }
    }
    
    for (auto iter = removeUIDSet.begin(); iter != removeUIDSet.end(); iter++)
    {
        m_ProceduralMaterialRenderUIDPending.erase(*iter);
    }

    GraphInstanceRenderUIDMap removeGraphInstanceUIDs;
    for (auto iter = m_GraphInstanceRenderUIDs.begin(); iter != m_GraphInstanceRenderUIDs.end(); iter++)
    {
        if (!m_Renderer->isPending(iter->second))
        {
            removeGraphInstanceUIDs.emplace(*iter);
        }
    }

    for (auto iter = removeGraphInstanceUIDs.begin(); iter != removeGraphInstanceUIDs.end(); iter++)
    {
        m_GraphInstanceRenderUIDs.erase(iter->first);
    }

    // Send notifications about which render jobs have finished
    for (auto iter = removeUIDSet.begin(); iter != removeUIDSet.end(); iter++)
    {
        SubstanceNotificationBus::Broadcast(&SubstanceNotifications::OnRenderFinished, *iter);
    }
}

CProceduralMaterial* CSubstanceSystem::GetMaterialFromPath(const char* path) const
{
    auto iter = m_ProceduralMaterials.find(string(path));
    if (iter != m_ProceduralMaterials.end())
    {
        return iter->second;
    }
    return nullptr;
}

CProceduralMaterial* CSubstanceSystem::GetMaterialFromGraphInstanceID(GraphInstanceID graphInstanceID) const
{
    ProceduralMaterialID materialID = graphInstanceID >> 16;
    auto iter = m_ProceduralMaterialIDMap.find(materialID);
    if (iter != m_ProceduralMaterialIDMap.end())
    {
        return iter->second;
    }
    return nullptr;
}

void CSubstanceSystem::RemoveMaterial(IProceduralMaterial* pMaterial)
{
    delete pMaterial;
}

CProceduralTexture* CSubstanceSystem::GetTextureFromPath(const char* path) const
{
    auto iter = m_ProceduralTextures.find(string(path));
    if (iter != m_ProceduralTextures.end())
    {
        return iter->second;
    }
    return nullptr;
}

void CSubstanceSystem::OnMaterialInstanced(CProceduralMaterial* pMaterial)
{
    //add to lookup tables
    m_ProceduralMaterials[pMaterial->GetPath()] = pMaterial;

    static ProceduralMaterialID sMaterialIDSeed = 1;
    pMaterial->SetID(sMaterialIDSeed);
    m_ProceduralMaterialIDMap[sMaterialIDSeed] = pMaterial;
    sMaterialIDSeed++;
}

void CSubstanceSystem::OnMaterialDestroyed(CProceduralMaterial* pMaterial)
{
    //remove from image queue
    for (auto iter = m_InputImageProcessMap.begin(); iter != m_InputImageProcessMap.end(); iter++)
    {
        SubstanceAir::InputInstanceImage* pImage = iter->first;
        if (CProceduralMaterial::GetFromGraphInstance(&pImage->mParentGraph) == pMaterial)
        {
            iter = m_InputImageProcessMap.erase(iter);
        }
    }

    //remove from lookup table
    m_ProceduralMaterialIDMap.erase(pMaterial->GetID());

    //remove from material map
    for (auto iter = m_ProceduralMaterials.begin(); iter != m_ProceduralMaterials.end(); iter++)
    {
        if (iter->second == pMaterial)
        {
            m_ProceduralMaterials.erase(iter);
            break;
        }
    }

    //remove all graphs
    for (int g = 0; g < pMaterial->GetGraphInstanceCount(); g++)
    {
        stl::find_and_erase_all(m_GraphRenderQueue, static_cast<CGraphInstance*>(pMaterial->GetGraphInstance(g))->GetSubstanceGraph());
    }
}

void CSubstanceSystem::OnTextureInstanced(const char* path, CProceduralTexture* pTexture)
{
    m_ProceduralTextures[string(path)] = pTexture;
}

void CSubstanceSystem::OnTextureDestroyed(CProceduralTexture* pTexture)
{
    for (auto iter = m_ProceduralTextures.begin(); iter != m_ProceduralTextures.end(); iter++)
    {
        if (iter->second == pTexture)
        {
            m_ProceduralTextures.erase(iter);
            break;
        }
    }

    for (auto iter = m_ProceduralTextureResultMap.begin(); iter != m_ProceduralTextureResultMap.end(); )
    {
        if (iter->first == pTexture)
        {
            iter = m_ProceduralTextureResultMap.erase(iter);
        }
        else
        {
            iter++;
        }
    }

    stl::find_and_erase_all(m_ProceduralTextureComputeVector, pTexture);
}

void CSubstanceSystem::OnEnterSimulation()
{
    //make sure we are using runtime budget for accurate FPS in game/physics mode
    OnRuntimeBudgetChanged(true);
}

void CSubstanceSystem::OnExitSimulation()
{
    //max our memory/cpu budget so exiting simulation mode happens rapidly
    m_Renderer->setOptions(SubstanceAir::RenderOptions());

    //reset material parameters and reload textures
    static int sLastResetID = 0;
    ++sLastResetID;

    for (auto iter = m_ProceduralMaterials.begin(); iter != m_ProceduralMaterials.end(); iter++)
    {
        iter->second->Reset(sLastResetID);
    }
}

void CSubstanceSystem::OnRuntimeBudgetChanged(bool bApplyChangesImmediately)
{
    m_Renderer->setOptions(CreateRenderOptions());

    if (bApplyChangesImmediately)
    {
        m_Renderer->run();
    }
}

void CSubstanceSystem::DoOnOutputComputed(SubstanceAir::OutputInstance* pOutput, bool bReloadTexture)
{
    SubstanceAir::OutputInstance::Result result = pOutput->grabResult();

    if (result.get() != nullptr)
    {
        const SubstanceTexture& texture = result->getTexture();

        //PC/Console builds require signed normal maps...
        //TODO: When we support signed BC5 in engine, just override the texture format!
#if AZ_TRAIT_SUBSTANCE_SIGNED_NORMAL_MAPS
        if (texture.pixelFormat == Substance_PF_BC5)
        {
            unsigned char* bufferU = static_cast<unsigned char*>(texture.buffer);
            char* bufferS = static_cast<char*>(texture.buffer);
            unsigned short width = texture.level0Width;
            unsigned short height = texture.level0Height;
            unsigned char mipCount = texture.mipmapCount;
            const unsigned int kBC5BlockSize = 16;

            while (mipCount--)
            {
                size_t mipSize = CProceduralTexture::CalcTextureSize(width, height, 1, Substance_PF_BC5);

                for (size_t s = 0; s < mipSize; s += kBC5BlockSize)
                {
                    bufferS[0] = (int)(bufferU[0]) - 128;
                    bufferS[1] = (int)(bufferU[1]) - 128;
                    bufferS[8] = (int)(bufferU[8]) - 128;
                    bufferS[9] = (int)(bufferU[9]) - 128;

                    //swap the XY components so the normal map is in the proper orientation
                    for (size_t bs = 0; bs < kBC5BlockSize / 2; bs++)
                    {
                        std::swap(bufferS[bs], bufferS[bs + kBC5BlockSize / 2]);
                    }

                    bufferU += kBC5BlockSize;
                    bufferS += kBC5BlockSize;
                }

                width >>= 1;
                height >>= 1;
            }
        }
#endif

        //handle dynamic/static output registration
        if (!pOutput->isDynamicOutput())
        {
            //non dynamic outputs are actual lumberyard textures and should be sent for GPU upload
            if (CProceduralTexture* pTexture = CProceduralTexture::GetFromOutputInstance(pOutput))
            {
                m_ProceduralTextureResultMap[pTexture].swap(result);

                if (bReloadTexture)
                {
                    stl::push_back_unique(m_ProceduralTextureComputeVector, pTexture);
                }
            }
        }
        else
        {
            //dynamic outputs are used by the plugin and should be stored for editor preview purposes
            SubstanceAir::UInt parentUid = pOutput->mUserData;

            for (auto& output : pOutput->mParentGraph.getOutputs())
            {
                if (output->mDesc.mUid == parentUid)
                {
                    if (CProceduralTexture* pTexture = CProceduralTexture::GetFromOutputInstance(output))
                    {
                        pTexture->SetEditorPreview(std::move(result));
                        break;
                    }
                }
            }
        }
    }
}

void CSubstanceSystem::OnOutputComputed(SubstanceAir::OutputInstance* pOutput, bool bReloadTexture)
{
    //issue texture load command on main thread
    SCommandMessage cmdMsg;

    if (bReloadTexture)
    {
        cmdMsg =
        {
            pOutput,
            [](void* userdata)
            {
                SubstanceAir::OutputInstance* pOutput = reinterpret_cast<SubstanceAir::OutputInstance*>(userdata);
                pSubstanceSystem->DoOnOutputComputed(pOutput, true);
            }
        };
    }
    else
    {
        cmdMsg =
        {
            pOutput,
            [](void* userdata)
            {
                SubstanceAir::OutputInstance* pOutput = reinterpret_cast<SubstanceAir::OutputInstance*>(userdata);
                pSubstanceSystem->DoOnOutputComputed(pOutput, false);
            }
        };
    }

    AZStd::lock_guard<AZStd::mutex> lock(m_MainThreadMessagesMutex);
    m_MainThreadMessages.push_back(cmdMsg);
}

bool CSubstanceSystem::QueueRender(CProceduralMaterial* pMaterial)
{
    bool queuedSomething = false;

    for (auto iter = pMaterial->m_Graphs.begin(); iter != pMaterial->m_Graphs.end(); iter++)
    {
        queuedSomething |= QueueRender((*iter).get());
    }

    return queuedSomething;
}

bool CSubstanceSystem::QueueRender(SubstanceAir::GraphInstance* pGraphInstance)
{
    if (m_GraphRenderQueue.end() == AZStd::find(m_GraphRenderQueue.begin(), m_GraphRenderQueue.end(), pGraphInstance))
    {
        m_GraphRenderQueue.push_back(pGraphInstance);
        return true;
    }
    else
    {
        return false;
    }
}

bool CSubstanceSystem::QueueRender(GraphInstanceID graphInstanceID)
{
    if (SubstanceAir::GraphInstance* pGraphInstance = GetSubstanceGraphInstance(graphInstanceID))
    {
        return QueueRender(pGraphInstance);
    }

    return false;
}

void CSubstanceSystem::RenderSync(ProceduralMaterialRenderMode mode)
{
    Render(SubstanceAir::Renderer::Run_Default, true, mode);

    //pump message queue so textures get uploaded
    Update();
}

ProceduralMaterialRenderUID CSubstanceSystem::RenderASync(bool force, ProceduralMaterialRenderMode mode)
{
    return Render(SubstanceAir::Renderer::Run_Asynchronous, force, mode);
}

bool CSubstanceSystem::IsGraphInstancePending(SubstanceAir::GraphInstance* graphInstance) const
{
    auto instanceIter = m_GraphInstanceRenderUIDs.find(graphInstance);

    if (instanceIter != m_GraphInstanceRenderUIDs.end())
    {
        ProceduralMaterialRenderUID renderUID = instanceIter->second;
        return m_Renderer->isPending(renderUID);
    }

    return false;
}

ProceduralMaterialRenderUID CSubstanceSystem::Render(int runOptions, bool force, ProceduralMaterialRenderMode mode)
{
    const bool synchronous = (0 == (runOptions & SubstanceAir::Renderer::Run_Asynchronous));

    GraphInstanceVector graphsToRenderNow;
    graphsToRenderNow.reserve(m_GraphRenderQueue.size());
    for (auto& graph : m_GraphRenderQueue)
    {
        // For synchronous renders, all queued items have to be forced through.
        // When the force flag is on, we force the whole queue through.
        // For asynchronous renders, when force=false, items will be left in the queue if they already
        // have pending renders. Otherwise, the render queue will get backed up during high frequency updates.
        if (synchronous || force || !IsGraphInstancePending(graph))
        {
            graphsToRenderNow.push_back(graph);

            CProceduralMaterial::GetFromGraphInstance(graph)->AddImageInputs(graph, m_InputImageProcessMap);
        }
    }

    //TODO: Mobile is giving us issues with this so we should consider either disabling this feature for all, or refactoring how it functions
#if AZ_TRAIT_SUBSTANCE_RENDER_IMAGE_INPUTS
    //process image inputs needed for this render batch
    RenderImageInputs();
#endif
    
    //enqueue graphs
    for (auto& graph : graphsToRenderNow)
    {
        m_Renderer->push(*graph);
    }

    if (!graphsToRenderNow.empty())
    {
        //dispatch render commands
        ProceduralMaterialRenderUID renderUID = m_Renderer->run(runOptions, (size_t)mode);

        m_ProceduralMaterialRenderUIDPending.emplace(renderUID);

        for (auto& graph : graphsToRenderNow)
        {
            m_GraphRenderQueue.erase(AZStd::find(m_GraphRenderQueue.begin(), m_GraphRenderQueue.end(), graph));
            m_GraphInstanceRenderUIDs[graph] = renderUID;
        }

        return renderUID;
    }
    else
    {
        return INVALID_PROCEDURALMATERIALRENDERUID;
    }
}

void CSubstanceSystem::RenderImageInputs()
{
    for (auto iter = m_InputImageProcessMap.begin(); iter != m_InputImageProcessMap.end(); iter++)
    {
        SubstanceAir::InputInstanceImage* pInput = iter->first;
        const char* path = iter->second.c_str();

        SSubstanceLoadData loadData;

        if (gAPI->ReadTexture(path, loadData))
        {
            SubstanceTexFormat texFormat = loadData.m_Format;

            SubstanceTexture substanceTexture;
            substanceTexture.level0Width = loadData.m_Width;
            substanceTexture.level0Height = loadData.m_Height;
            substanceTexture.mipmapCount = loadData.m_NumMips;
            substanceTexture.pixelFormat = CProceduralTexture::ToSubstanceFormat(texFormat);
            substanceTexture.channelsOrder = CProceduralTexture::GetChannelsOrder(texFormat);
            substanceTexture.buffer = loadData.m_pData;

            SubstanceAir::InputImage::SPtr imagePtr = SubstanceAir::InputImage::create(substanceTexture, loadData.m_DataSize);
            pInput->setImage(imagePtr);

            gAPI->ReleaseSubstanceLoadData(loadData);
        }
    }

    m_InputImageProcessMap.clear();
}

bool CSubstanceSystem::HasRenderCompleted(ProceduralMaterialRenderUID uid) const
{
    auto iter = m_ProceduralMaterialRenderUIDPending.find(uid);
    if (iter == m_ProceduralMaterialRenderUIDPending.end())
    {
        return true;
    }
    else
    {
        return false;
    }
}

void CSubstanceSystem::RenderFence()
{
    m_Renderer->flush();
    m_ProceduralMaterialRenderUIDPending.clear();
    m_GraphInstanceRenderUIDs.clear();
}

GraphInstanceID CSubstanceSystem::EncodeGraphInstanceID(CProceduralMaterial* pMaterial, uint16 graphIndex)
{
    //quick sanity check
    if (graphIndex < 0 || graphIndex >= pMaterial->GetGraphInstanceCount())
    {
        return INVALID_GRAPHINSTANCEID;
    }

    return pMaterial->GetID() << 16 | graphIndex;
}

IGraphInstance* CSubstanceSystem::GetGraphInstance(GraphInstanceID graphInstanceID) const
{
    ProceduralMaterialID materialID = graphInstanceID >> 16;
    uint16 graphIndex = graphInstanceID & 0xFFFF;

    auto iter = m_ProceduralMaterialIDMap.find(materialID);
    if (iter != m_ProceduralMaterialIDMap.end())
    {
        CProceduralMaterial* pMaterial = iter->second;

        if (graphIndex < pMaterial->GetGraphInstanceCount())
        {
            return pMaterial->GetGraphInstance(graphIndex);
        }
    }

    return nullptr;
}

SubstanceAir::GraphInstance* CSubstanceSystem::GetSubstanceGraphInstance(GraphInstanceID graphInstanceID) const
{
    ProceduralMaterialID materialID = graphInstanceID >> 16;
    uint16 graphIndex = graphInstanceID & 0xFFFF;

    auto iter = m_ProceduralMaterialIDMap.find(materialID);
    if (iter != m_ProceduralMaterialIDMap.end())
    {
        CProceduralMaterial* pMaterial = iter->second;

        if (graphIndex < pMaterial->GetGraphInstanceCount())
        {
            return pMaterial->GetSubstanceGraphInstance(graphIndex);
        }
    }

    return nullptr;
}

SubstanceAir::OutputInstance::Result CSubstanceSystem::GetOutputResult(CProceduralTexture* pTexture)
{
    SubstanceAir::OutputInstance::Result result;
    auto iter = m_ProceduralTextureResultMap.find(pTexture);
    if (iter != m_ProceduralTextureResultMap.end())
    {
        result.reset(iter->second.release());
        m_ProceduralTextureResultMap.erase(iter);
    }

    return result;
}

bool CSubstanceSystem::LoadTextureData(const char* path, SSubstanceLoadData& loadData)
{
    string strPath(path);
    CProceduralTexture* pTexture = CProceduralTexture::CreateFromPath(path);

    if (pTexture != NULL)
    {
        //PBR Support for Lumberyard means when the engine asks for a normal map
        //but with FT_ALPHA set, it actually wants the Glossiness map. Read that
        //request in here and redirect to the proper texture.
        if (SubstanceAir::OutputInstance* pOutput = pTexture->GetOutputInstance())
        {
            if (pOutput->mDesc.mChannel == SubstanceAir::Channel_Normal)
            {
                if ((loadData.m_nFlags & FT_ALPHA) == FT_ALPHA)
                {
                    for (auto& output : pOutput->mParentGraph.getOutputs())
                    {
                        if (output->mDesc.mChannel == SubstanceAir::Channel_Glossiness)
                        {
                            if (CProceduralTexture* texture = CProceduralTexture::GetFromOutputInstance(output))
                            {
                                if (texture->PopulateTextureLoadData(loadData))
                                {
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (pTexture->PopulateTextureLoadData(loadData))
        {
            return true;
        }
    }

    //texture unable to be loaded, ignore
    return false;
}

#endif // AZ_USE_SUBSTANCE

