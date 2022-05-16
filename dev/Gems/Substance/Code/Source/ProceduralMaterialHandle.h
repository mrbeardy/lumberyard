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

#pragma once

#include <Substance/Types.h>
#include <Substance/IProceduralMaterial.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace AZ
{
    class ReflectContext;
}


//! Wraps a ProceduralMaterial Graph to expose it to Behavior Context for scripting
class ProceduralMaterialHandle
{
public:
    AZ_TYPE_INFO(ProceduralMaterialHandle, "{4179EC76-2E5C-4214-A8EE-D86A41A8CB58}")
        
    ProceduralMaterialHandle() : m_graphInstance(INVALID_GRAPHINSTANCEID){}
    GraphInstanceID m_graphInstance;

    static void Reflect(AZ::ReflectContext* serializeContext);
};

