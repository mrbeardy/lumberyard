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

#ifndef CRYINCLUDE_LYSUBSTANCE_PROCEDURALTEXTURE_H
#define CRYINCLUDE_LYSUBSTANCE_PROCEDURALTEXTURE_H
#pragma once

#if defined(AZ_USE_SUBSTANCE)
class CProceduralMaterial;

#include "Substance/ISubstanceAPI.h"

class CProceduralTexture
{
public:
    CProceduralTexture(CProceduralMaterial* pParent, SubstanceAir::OutputInstance* instance);
    ~CProceduralTexture();

    inline CProceduralMaterial* GetProceduralMaterial() const
    {
        return m_pParent;
    }
    inline SubstanceAir::OutputInstance* GetOutputInstance() const
    {
        return m_pOutput;
    }

    inline IDeviceTexture* GetDeviceTexture() const
    {
        return m_pDeviceTexture;
    }
    inline void SetDeviceTexture(IDeviceTexture* pDeviceTexture)
    {
        m_pDeviceTexture = pDeviceTexture;
    }

    void Init(const SubstanceAir::OutputFormat& outputFormat);
    bool PopulateTextureLoadData(SSubstanceLoadData& loadData);

    void OnOutputComputed();

    SubstanceAir::RenderResult* GetEditorPreview() const;
    void SetEditorPreview(SubstanceAir::OutputInstance::Result result);

    bool IsCompressed() const;

    //static functions
    static int CalcNumMips(int nWidth, int nHeight);
    static size_t CalcTextureSize(int width, int height, int mips, unsigned char pixelFormat);
    static int GetBlockDim(unsigned int pixelFormat);
    static int GetBitsPerPixel(unsigned int pixelFormat);
    static unsigned char GetChannelsOrder(SubstanceTexFormat format);
    static SubstanceTexFormat ToEngineFormat(unsigned char pixelFormat);
    static unsigned char ToSubstanceFormat(SubstanceTexFormat format);
    static CProceduralTexture* CreateFromPath(const char* path);
    static CProceduralTexture* GetFromOutputInstance(SubstanceAir::OutputInstance* pOutput);

private:
    CProceduralMaterial*                                        m_pParent;
    SubstanceAir::OutputInstance*                               m_pOutput;
    IDeviceTexture*                                             m_pDeviceTexture;

    SubstanceAir::OutputInstance::Result                        m_EditorResult;
};

#endif // AZ_USE_SUBSTANCE
#endif //CRYINCLUDE_LYSUBSTANCE_PROCEDURALTEXTURE_H
