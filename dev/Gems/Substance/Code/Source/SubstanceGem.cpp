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
/** @file SubstanceGem.cpp
    @brief Source file for the Substance Gem Implementation.
    @author Josh Coyne - Allegorithmic (josh.coyne@allegorithmic.com)
    @date 09-14-2015
    @copyright Allegorithmic. All rights reserved.
*/
#include "Substance_precompiled.h"
#include <platform_impl.h>
#include "SubstanceGem.h"

#include "SubstanceGemSystemComponent.h"
#include <Substance/SubstanceBus.h>
#include <Substance_Traits_Platform.h>

#if defined(AZ_USE_SUBSTANCE)
#include "SubstanceAPI.h"
#include <CryLibrary.h>
#include <I3DEngine.h>
#include <IRenderer.h>


//Cvars
int substance_coreCount;
int substance_memoryBudget;
ICVar* substance_engineLibrary = nullptr;

void* gSubstanceEngineModule = nullptr;

//Engine DLL Paths
#if defined(AIR_NO_DEFAULT_ENGINE)
static const char* gszSubstanceEngineCPULibrary = AZ_TRAIT_SUBSTANCE_ENGINE_CPU_LIBRARY;
static const char* gszSubstanceEngineGPULibrary = AZ_TRAIT_SUBSTANCE_ENGINE_GPU_LIBRARY;
#endif

//////////////////////////////////////////////////////////////////////////
struct CTextureLoadHandler_Substance
    : public ITextureLoadHandler
{
    CTextureLoadHandler_Substance(ISubstanceLibAPI* pAPI)
        : m_SubstanceLibAPI(pAPI)
    {
    }

    virtual bool SupportsExtension(const char* ext) const override
    {
        if (!strcmp(ext, PROCEDURALTEXTURE_EXTENSION))
        {
            return true;
        }

        return false;
    }

    ETEX_Format SubstanceFormatToEngineFormat(SubstanceTexFormat substanceFormat) const
    {
        switch (substanceFormat)
        {
        case SubstanceTex_BC1:
            return eTF_BC1;
        case SubstanceTex_BC2:
            return eTF_BC2;
        case SubstanceTex_BC3:
            return eTF_BC3;
        case SubstanceTex_BC4:
            return eTF_BC4U;
        case SubstanceTex_BC5:
            return eTF_BC5S;
        case SubstanceTex_L8:
            return eTF_L8;
        case SubstanceTex_PVRTC2:
            return eTF_PVRTC2;
        case SubstanceTex_PVRTC4:
            return eTF_PVRTC4;
        case SubstanceTex_R8G8B8A8:
            return eTF_R8G8B8A8;
        case SubstanceTex_R16G16B16A16:
            return eTF_R16G16B16A16;
        default:
            return eTF_Unknown;
        }
    }

    virtual bool LoadTextureData(const char* path, STextureLoadData& loadData) override
    {
        if (m_SubstanceLibAPI)
        {
            SSubstanceLoadData subData;

            memset(&subData, 0, sizeof(subData));
            subData.m_pTexture = reinterpret_cast<IDeviceTexture*>(loadData.m_pTexture);
            subData.m_nFlags = loadData.m_nFlags;
            subData.m_dataAllocFunc = STextureLoadData::AllocateData;
            if (m_SubstanceLibAPI->LoadTextureData(path, subData))
            {
                loadData.m_pData = subData.m_pData;
                loadData.m_DataSize = subData.m_DataSize;
                loadData.m_Width = subData.m_Width;
                loadData.m_Height = subData.m_Height;
                loadData.m_Format = SubstanceFormatToEngineFormat(subData.m_Format);
                loadData.m_NumMips = subData.m_NumMips;
                loadData.m_nFlags = subData.m_nFlags;

                return true;
            }
        }

        return false;
    }

    virtual void Update() override
    {
        if (m_SubstanceLibAPI)
        {
            m_SubstanceLibAPI->Update();
        }
    }

private:
    ISubstanceLibAPI* m_SubstanceLibAPI;
};

//////////////////////////////////////////////////////////////////////////
void OnSubstanceRuntimeBudgetChangled(bool budgetChanged)
{
    ISubstanceLibAPI* pSubstanceLibAPI = nullptr;
    EBUS_EVENT_RESULT(pSubstanceLibAPI, SubstanceRequestBus, GetSubstanceLibAPI);
    if (pSubstanceLibAPI)
    {
        pSubstanceLibAPI->OnRuntimeBudgetChanged(budgetChanged);
    }
}

void OnCVarCoreCountChange(ICVar* pArgs)
{
    substance_coreCount = pArgs->GetIVal();
    OnSubstanceRuntimeBudgetChangled(false);
}

void OnCVarMemoryBudgetChange(ICVar* pArgs)
{
    substance_memoryBudget = pArgs->GetIVal();
    OnSubstanceRuntimeBudgetChangled(false);
}

void OnCommitRenderOptions(IConsoleCmdArgs* pArgs)
{
    OnSubstanceRuntimeBudgetChangled(true);
}


//////////////////////////////////////////////////////////////////////////
SubstanceGem::SubstanceGem()
    : CryHooksModule()
{
    m_descriptors.insert(m_descriptors.end(), {
        SubstanceGemSystemComponent::CreateDescriptor(),
    });
}

SubstanceGem::~SubstanceGem() { }

AZ::ComponentTypeList SubstanceGem::GetRequiredSystemComponents() const
{
    return AZ::ComponentTypeList{
        azrtti_typeid<SubstanceGemSystemComponent>(),
    };
}

void SubstanceGem::RegisterConsole()
{
    REGISTER_CVAR_CB(substance_coreCount, 32, 0, "Set how many CPU Cores are used for Substance (32 = All). Only relevant when using CPU based engines.", OnCVarCoreCountChange);
    REGISTER_CVAR_CB(substance_memoryBudget, 512, 0, "Set how much memory is used for Substance in MB", OnCVarMemoryBudgetChange);

    substance_engineLibrary = REGISTER_STRING("substance_engineLibrary", "cpu", VF_NULL, "Select Substance engine to load (cpu/gpu), changes take effect on Lumberyard restart.");

    REGISTER_COMMAND("substance_commitRenderOptions", OnCommitRenderOptions, VF_NULL, "Apply cpu and memory changes immediately, rather than wait for next render call");
}


void SubstanceGem::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
    switch (event)
    {
    case ESYSTEM_EVENT_GAME_POST_INIT:
        RegisterConsole();
        m_substanceEngine.Setup( (!_stricmp(substance_engineLibrary->GetString(), "gpu")) ? SubstanceEngineMode::GPU : SubstanceEngineMode::CPU );
        break;
    case ESYSTEM_EVENT_EDITOR_GAME_MODE_CHANGED:
    case ESYSTEM_EVENT_EDITOR_SIMULATION_MODE_CHANGED:
    {
        if (gEnv->IsEditor() && m_substanceEngine.GetLibAPI())
        {
            if (wparam == 0)
            {
                m_substanceEngine.GetLibAPI()->OnExitSimulation();
            }
            else
            {
                m_substanceEngine.GetLibAPI()->OnEnterSimulation();
            }
        }
    }
    break;
    case ESYSTEM_EVENT_FAST_SHUTDOWN:
    case ESYSTEM_EVENT_FULL_SHUTDOWN:
    {
        m_substanceEngine.TearDown();
    }
    break;
    }
}

//////////////////////////////////////////////////////////////////////////

void SubstanceEngineHarness::Setup(SubstanceEngineMode mode)
{
    if (LoadEngineLibrary(mode))
    {
        RegisterTextureHandler();
        CryLogAlways("Substance Initialized");
    }
    else
    {
        CryLogAlways("Substance Not Initialized\n");
    }
}

void SubstanceEngineHarness::TearDown()
{
    UnregisterTextureHandler();

    if (m_SubstanceLibAPI)
    {
        SubstanceRequestBus::Broadcast(&SubstanceRequests::SetSubstanceLibAPI, nullptr);

        m_SubstanceLibAPI->Release();
        m_SubstanceLibAPI = nullptr;
    }

    if (gSubstanceEngineModule != nullptr)
    {
        CryFreeLibrary(gSubstanceEngineModule);
        gSubstanceEngineModule = nullptr;
    }
}

bool SubstanceEngineHarness::LoadEngineLibrary(SubstanceEngineMode mode)
{
    //Load substance engine DLL if configured
#if defined (AIR_NO_DEFAULT_ENGINE)
    const char* engineLib = nullptr;
    switch(mode)
    {
    case SubstanceEngineMode::CPU:
        engineLib = gszSubstanceEngineCPULibrary;
        break;
    case SubstanceEngineMode::GPU:
        engineLib = gszSubstanceEngineGPULibrary;
        break;
    default:
        AZ_Assert(false, "Unsupported mode %d", mode);
        return false;
    }

    gSubstanceEngineModule = CryLoadLibrary(engineLib);

    //if load failed, try the CPU engine
    if (!gSubstanceEngineModule && gszSubstanceEngineCPULibrary != engineLib)
    {
        gSubstanceEngineModule = CryLoadLibrary(gszSubstanceEngineCPULibrary);
    }

    //don't load if we can't find the DLL
    if (gSubstanceEngineModule == nullptr)
    {
        return false;
    }
#endif

    GetSubstanceAPI(&m_SubstanceAPI, &m_SubstanceLibAPI);

    SubstanceRequestBus::Broadcast(&SubstanceRequests::SetSubstanceLibAPI, m_SubstanceLibAPI);
        
    if (m_SubstanceLibAPI)
    {
        ISubstanceLibAPI* apiCheck = nullptr;
        SubstanceRequestBus::BroadcastResult(apiCheck, &SubstanceRequests::GetSubstanceLibAPI);
        if (apiCheck == m_SubstanceLibAPI)
        {
            return true;
        }
        else
        {
            AZ_Error("Substance", false, "m_SubstanceLibAPI was not successfully passed to the SubstanceRequestBus");
        }
    }

    return false;
}

void SubstanceEngineHarness::RegisterTextureHandler()
{
    if (gEnv && gEnv->p3DEngine)
    {
        m_TextureLoadHandler = new CTextureLoadHandler_Substance(m_SubstanceLibAPI);
        gEnv->p3DEngine->AddTextureLoadHandler(m_TextureLoadHandler);
    }
    else
    {
        AZ_Error("Substance", false, "3DEngine is uninitialized. Substance Gem texture handler can't be registered.");
    }
}

void SubstanceEngineHarness::UnregisterTextureHandler()
{
    if (m_TextureLoadHandler)
    {
        AZ_Assert(gEnv, "gEnv was destroyed before SubstanceEngineHarness::UnregisterTextureHandler()");
        AZ_Assert(gEnv && gEnv->p3DEngine, "3DEngine was destroyed before SubstanceEngineHarness::UnregisterTextureHandler()");

        if (gEnv && gEnv->p3DEngine)
        {
            gEnv->p3DEngine->RemoveTextureLoadHandler(m_TextureLoadHandler);
        }

        delete m_TextureLoadHandler;
        m_TextureLoadHandler = nullptr;
    }
}

ISubstanceLibAPI* SubstanceEngineHarness::GetLibAPI()
{
    return m_SubstanceLibAPI;
}

//////////////////////////////////////////////////////////////////////////



#else

SubstanceGem::SubstanceGem()
    : CryHooksModule()
{
    m_descriptors.insert(m_descriptors.end(), {
        SubstanceGemSystemComponent::CreateDescriptor(),
    });
}

SubstanceGem::~SubstanceGem() { }

AZ::ComponentTypeList SubstanceGem::GetRequiredSystemComponents() const
{
    return AZ::ComponentTypeList{
        azrtti_typeid<SubstanceGemSystemComponent>(),
    };
}

void SubstanceGem::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
    switch (event)
    {
    case ESYSTEM_EVENT_GAME_POST_INIT:
        // This warning message is located here to correspond to where the "Substance Initialized" log message would normally occur.
        AZ_Warning("Substance", false, "The Substance Gem is enabled, but the Substance runtime is not supported in the current configuration.");
        break;
    }
}

#endif // AZ_USE_SUBSTANCE

AZ_DECLARE_MODULE_CLASS(Substance_a2f08ba9713f485a8485d7588e5b120f, SubstanceGem)

