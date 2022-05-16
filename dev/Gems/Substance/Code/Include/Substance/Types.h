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
/** @file Types.h
    @brief Header for the Procedural Material Interface
    @author Josh Coyne - Allegorithmic (josh.coyne@allegorithmic.com)
    @date 09-14-2015
    @copyright Allegorithmic. All rights reserved.
*/
#pragma once

#include <stdint.h>

/// Encodes a procedural material and graph index
typedef uint32_t GraphInstanceID;
#define INVALID_GRAPHINSTANCEID ((GraphInstanceID)(0))

/// Encodes a graphinput UID
typedef uint32_t GraphInputID;
#define INVALID_GRAPHINPUTID ((GraphInputID)(0))

/// Encodes a graphoutput UID
typedef uint32_t GraphOutputID;
#define INVALID_GRAPHOUTPUTID ((GraphOutputID)(0))

/// A unique render batch ID
typedef unsigned int ProceduralMaterialRenderUID;
#define INVALID_PROCEDURALMATERIALRENDERUID ((ProceduralMaterialRenderUID)0)

/// Maps to substance unique ID's
typedef unsigned int SubstanceUID;

/// References a procedural material ID in CSubstanceSystem
typedef uint16_t ProceduralMaterialID;
#define INVALID_PROCEDURALMATERIALID ((ProceduralMaterialID)0)

