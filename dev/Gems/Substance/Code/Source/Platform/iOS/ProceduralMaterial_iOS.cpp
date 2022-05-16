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

#include "ProceduralMaterial.h"

namespace Platform
{
    unsigned int GetPlatformPixelFormat(int32 format, SubstanceAir::ChannelUse channelUse)
    {
        AZ_UNUSED(format);
        AZ_UNUSED(channelUse);

        return Substance_PF_PVRTC4;
    }
}

#endif