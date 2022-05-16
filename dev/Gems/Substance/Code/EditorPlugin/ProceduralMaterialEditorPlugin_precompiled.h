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
#include <AzCore/PlatformDef.h>

/////////////////////////////////////////////////////////////////////////////
// CRY Stuff ////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// substance library uses the old allocator typedefs that have been deprecated in C++17
// MSVC warns that these typedef members are being used and therefore must be acknowledged by using the following macro
// Now the reason the _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING is defined in this file is
// is that CryMath is the first place the standard <memory> header is included and the _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
// must be defined at that point to suppress the warning.
#pragma push_macro("_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING")
#ifndef _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#endif
#include <Cry_Math.h>
#pragma pop_macro("_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING")
#include <ISystem.h>
#include <ISerialize.h>
#include <CryName.h>
#include <Util/PathUtil.h>

/////////////////////////////////////////////////////////////////////////////
// STL
/////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <list>
#include <map>
#include <set>
#include <algorithm>

/////////////////////////////////////////////////////////////////////////////
// Qt
/////////////////////////////////////////////////////////////////////////////
#include <QtCore>
#include <QtGui>
#include <QtWidgets>

/////////////////////////////////////////////////////////////////////////////
// Misc
/////////////////////////////////////////////////////////////////////////////
#define DIMOF(x) (sizeof(x)/sizeof(x[0]))
