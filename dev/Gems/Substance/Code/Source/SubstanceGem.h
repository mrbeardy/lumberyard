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
#ifndef GEM_46AED0DF_955D_4582_9583_0B4D2422A727_CODE_SOURCE_SUBSTANCEGEM_H
#define GEM_46AED0DF_955D_4582_9583_0B4D2422A727_CODE_SOURCE_SUBSTANCEGEM_H
#pragma once

#include <IGem.h>
#if defined(AZ_USE_SUBSTANCE)
#include "SubstanceAPI.h"

struct CTextureLoadHandler_Substance;
#endif // AZ_USE_SUBSTANCE

enum class SubstanceEngineMode
{
    CPU,
    GPU
};

#if defined(AZ_USE_SUBSTANCE)
//! Manages setup and teardown of the Substance Engine and its APIs
//! (This is separated from the main SubstanceGem class to support unit tests)
class SubstanceEngineHarness
{
public:
    void Setup(SubstanceEngineMode mode);
    void TearDown();

    ISubstanceLibAPI* GetLibAPI();

private:
    bool LoadEngineLibrary(SubstanceEngineMode mode);
    void RegisterTextureHandler();
    void UnregisterTextureHandler();

    ISubstanceLibAPI* m_SubstanceLibAPI = nullptr;
    CSubstanceAPI     m_SubstanceAPI;
    CTextureLoadHandler_Substance* m_TextureLoadHandler = nullptr;
};
#endif // AZ_USE_SUBSTANCE

//! Main entry point for the Substance Gem
class SubstanceGem : public CryHooksModule
{
public:
    AZ_RTTI(SubstanceGem, "{4BCD80A7-A8C7-47A1-96A8-A6474D898E7B}", CryHooksModule);
    
    SubstanceGem();
    ~SubstanceGem() override;

    AZ::ComponentTypeList GetRequiredSystemComponents() const override;

    void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;

#if defined(AZ_USE_SUBSTANCE)
    void RegisterConsole();

private:
    SubstanceEngineHarness m_substanceEngine;

#endif // AZ_USE_SUBSTANCE
};


#endif//GEM_46AED0DF_955D_4582_9583_0B4D2422A727_CODE_SOURCE_SUBSTANCEGEM_H
