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
/** @file Substance_precompiled.h
    @brief Precompiled Header
    @author Josh Coyne - Allegorithmic (josh.coyne@allegorithmic.com)
    @date 09-14-2015
    @copyright Allegorithmic. All rights reserved.
*/
#pragma once

#if defined(AZ_USE_SUBSTANCE)
// substance library uses the old allocator typedefs that have been deprecated in C++17
// MSVC warns that these typedef members are being used and therefore must be acknowledged by using the following macro
// Furthermore the macros must be placed around the first include of the standard <memory> header
// so that warning is able to be suppressed, otherwise #pragma once would have the 
// <memory> include occurring earlier before the _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING is defined
#pragma push_macro("_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING")
#ifndef _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#endif
#include <substance/handle.h>
#include <substance/version.h>
#include <substance/framework/renderer.h>
#include <substance/framework/framework.h>
#pragma pop_macro("_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING")

#include <platform.h>


#define MEM_KB(mem) (mem * 1024)
#define MEM_MB(mem) (MEM_KB(mem) * 1024)

#define DIMOF(x) (sizeof(x) / sizeof(x[0]))

#include <CryWindows.h> // needed for UINT defines used by CImageExtensionHelper
#include <CryName.h>
#include <I3DEngine.h>
#include <ISerialize.h>
#include <IGem.h>


#include <stdint.h>

#include "Substance/IProceduralMaterial.h"

typedef std::map<SubstanceAir::InputInstanceImage*, string> InputImageMap;

struct ISubstanceAPI;
extern ISubstanceAPI* gAPI;

struct ISubstanceLibAPI;
extern ISubstanceLibAPI* gLibAPI;

//CVars
extern int substance_coreCount;
extern int substance_memoryBudget;

#define DIMOF(x) (sizeof(x) / sizeof(x[0]))
#endif


