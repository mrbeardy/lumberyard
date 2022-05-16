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
#include "ProceduralTexture.h"
#include "ProceduralMaterial.h"
#include "SubstanceSystem.h"

#include "IImage.h"

static uint8 QuantizeValue(uint16 qvalue)
{
    return (uint8)((((uint32)qvalue) * 0xFF) / 0xFFFF);
}

//--------------------------------------------------------------------------------------------------------------
CProceduralTexture::CProceduralTexture(CProceduralMaterial* pParent, SubstanceAir::OutputInstance* output)
    : m_pParent(pParent)
    , m_pOutput(output)
    , m_pDeviceTexture(nullptr)
{
}

CProceduralTexture::~CProceduralTexture()
{
    if (m_pDeviceTexture)
    {
        gAPI->ReleaseDeviceTexture(m_pDeviceTexture);
    }

    pSubstanceSystem->OnTextureDestroyed(this);
}

void CProceduralTexture::Init(const SubstanceAir::OutputFormat& outputFormat)
{
    m_pOutput->overrideFormat(outputFormat);
}

CProceduralTexture* CProceduralTexture::CreateFromPath(const char* path)
{
    //see if this texture has already been loaded
    if (CProceduralTexture* pPathTexture = pSubstanceSystem->GetTextureFromPath(path))
    {
        return pPathTexture;
    }

    unsigned int outputID = 0;
    const char* material;

    string sPath(path);

    if (gAPI->LoadTextureXML(sPath.c_str(), outputID, &material))
    {
        CProceduralMaterial* pMaterial = CProceduralMaterial::CreateFromPath(material);
        if (pMaterial == nullptr)
        {
            AZ_Error("Substance", false, "ProceduralTexture: Malformed xml (invalid Material path) for sub file (%s)", sPath.c_str());
            return nullptr;
        }

        CProceduralTexture* pTexture = pMaterial->GetTextureByUID(outputID);
        if (pTexture == nullptr)
        {
            AZ_Error("Substance", false, "ProceduralTexture: Procedural Material (%s) has invalid output id for sub file (%s)", material, sPath.c_str());
            return nullptr;
        }

        //register with substance system
        pSubstanceSystem->OnTextureInstanced(sPath.c_str(), pTexture);

        return pTexture;
    }

    return nullptr;
}

CProceduralTexture* CProceduralTexture::GetFromOutputInstance(SubstanceAir::OutputInstance* pOutput)
{
    //dynamic outputs use the user data in different ways, so ignore here
    if (pOutput->isDynamicOutput())
    {
        return nullptr;
    }
    else
    {
        return reinterpret_cast<CProceduralTexture*>(pOutput->mUserData);
    }
}

SubstanceAir::RenderResult* CProceduralTexture::GetEditorPreview() const
{
    return m_EditorResult.get();
}

void CProceduralTexture::SetEditorPreview(SubstanceAir::OutputInstance::Result result)
{
    m_EditorResult.swap(result);
}

bool CProceduralTexture::IsCompressed() const
{
    const SubstanceAir::OutputFormat& format = m_pOutput->getFormatOverride();
    unsigned int pixelFormat = (format.format != SubstanceAir::OutputFormat::UseDefault)
        ? format.format : m_pOutput->mDesc.mFormat;

    return ((pixelFormat & Substance_PF_MASK_RAWFormat) != Substance_PF_RAW);
}

SubstanceTexFormat CProceduralTexture::ToEngineFormat(unsigned char pixelFormat)
{
    switch (pixelFormat & 0x1F)
    {
    case Substance_PF_RGBA:
        return SubstanceTex_R8G8B8A8;
    case Substance_PF_RGBx:
        return SubstanceTex_R8G8B8A8;
    case Substance_PF_RGB:
        return SubstanceTex_R8G8B8A8;
    case Substance_PF_L:
        return SubstanceTex_L8;
    case Substance_PF_RGBA | Substance_PF_16b:
        return SubstanceTex_R16G16B16A16;
    case Substance_PF_BC1:
        return SubstanceTex_BC1;
    case Substance_PF_BC2:
        return SubstanceTex_BC2;
    case Substance_PF_BC3:
        return SubstanceTex_BC3;
    case Substance_PF_BC4:
        return SubstanceTex_BC4;
    case Substance_PF_BC5:
        return SubstanceTex_BC5;
    case Substance_PF_PVRTC2:
        return SubstanceTex_PVRTC2;
    case Substance_PF_PVRTC4:
        return SubstanceTex_PVRTC4;
    default:
    case Substance_PF_RGB | Substance_PF_16b:
    case Substance_PF_RGBx | Substance_PF_16b:
    case Substance_PF_L   | Substance_PF_16b:
    case Substance_PF_DXT2:
    case Substance_PF_DXT4:
    case Substance_PF_JPEG:
        return SubstanceTex_Unknown;
    }
}

int CProceduralTexture::GetBlockDim(unsigned int pixelFormat)
{
    switch (pixelFormat & 0x1F)
    {
    case Substance_PF_BC1:
    case Substance_PF_BC2:
    case Substance_PF_BC3:
    case Substance_PF_BC4:
    case Substance_PF_BC5:
        return 4;
    case Substance_PF_PVRTC2:
    case Substance_PF_PVRTC4:
        return 8;
    default:
        return 1;
    }
}

int CProceduralTexture::GetBitsPerPixel(unsigned int pixelFormat)
{
    switch (pixelFormat & 0x1F)
    {
    case Substance_PF_RGBA | Substance_PF_16b:
        return 64;
    case Substance_PF_RGBA:
    case Substance_PF_RGBx:
        return 32;
    case Substance_PF_RGB:
        return 24;
    case Substance_PF_L:
    case Substance_PF_BC2:
    case Substance_PF_BC3:
    case Substance_PF_BC5:
        return 8;
    case Substance_PF_BC1:
    case Substance_PF_BC4:
    case Substance_PF_PVRTC4:
        return 4;
    case Substance_PF_PVRTC2:
        return 2;
    default:
        return 0;
    }
}

unsigned char CProceduralTexture::GetChannelsOrder(SubstanceTexFormat format)
{
    switch (format)
    {
    case SubstanceTex_R8G8B8A8:
        return Substance_ChanOrder_BGRA;
    default:
        return Substance_ChanOrder_ARGB;
    }
}

unsigned char CProceduralTexture::ToSubstanceFormat(SubstanceTexFormat format)
{
    switch (format)
    {
    case SubstanceTex_R8G8B8A8:
        return Substance_PF_RGBA;
    case SubstanceTex_L8:
        return Substance_PF_L;
    case SubstanceTex_R16G16B16A16:
        return Substance_PF_RGBA | Substance_PF_16b;
    case SubstanceTex_BC1:
        return Substance_PF_BC1;
    case SubstanceTex_BC2:
        return Substance_PF_BC2;
    case SubstanceTex_BC3:
        return Substance_PF_BC3;
    case SubstanceTex_BC4:
        return Substance_PF_BC4;
    case SubstanceTex_BC5:
        return Substance_PF_BC5;
    case SubstanceTex_PVRTC2:
        return Substance_PF_PVRTC2;
    case SubstanceTex_PVRTC4:
        return Substance_PF_PVRTC4;
    default:
        return 0;
    }
}

int CProceduralTexture::CalcNumMips(int nWidth, int nHeight)
{
    //copied from CTexture::CalcNumMips
    int nMips = 0;
    while (nWidth || nHeight)
    {
        if (!nWidth)
        {
            nWidth = 1;
        }
        if (!nHeight)
        {
            nHeight = 1;
        }
        nWidth >>= 1;
        nHeight >>= 1;
        nMips++;
    }
    return nMips;
}

size_t CProceduralTexture::CalcTextureSize(int width, int height, int mips, unsigned char pixelFormat)
{
    //mostly copypasta from CTexture due to dll boundary
    const int BlockDim = GetBlockDim(pixelFormat);
    const int BytesPerBlock = BlockDim * BlockDim * GetBitsPerPixel(pixelFormat) / 8;
    int size = 0;
    while ((width || height) && mips)
    {
        width = max(1, width);
        height = max(1, height);
        size += ((width + BlockDim - 1) / BlockDim) * ((height + BlockDim - 1) / BlockDim) * BytesPerBlock;

        width >>= 1;
        height >>= 1;
        --mips;
    }

    return size;
}

bool CProceduralTexture::PopulateTextureLoadData(SSubstanceLoadData& loadData)
{
    AZ_Assert(loadData.m_dataAllocFunc != nullptr, "Allocation function is required.");

    if (m_pOutput->mEnabled == false)
    {
        return false;
    }

    SubstanceAir::OutputInstance::Result result = pSubstanceSystem->GetOutputResult(this);

    if (result.get() != nullptr)
    {
        const SubstanceTexture& substanceTexture = result->getTexture();
        const SubstanceAir::ChannelUse channel = m_pOutput->mDesc.mChannel;

        loadData.m_Width = substanceTexture.level0Width;
        loadData.m_Height = substanceTexture.level0Height;
        loadData.m_NumMips = substanceTexture.mipmapCount == 0 ? CalcNumMips(loadData.m_Width, loadData.m_Height) : substanceTexture.mipmapCount;
        loadData.m_Format = ToEngineFormat(substanceTexture.pixelFormat);
        loadData.m_nFlags = 0;

        if (channel == SubstanceAir::Channel_Normal)
        {
            loadData.m_nFlags |= FIM_NORMALMAP;

            //see if we have gloss to add as well
            for (auto& output : m_pOutput->mParentGraph.getOutputs())
            {
                if (output->mDesc.mChannel == SubstanceAir::Channel_Glossiness)
                {
                    loadData.m_nFlags |= FIM_HAS_ATTACHED_ALPHA | FIM_SPLITTED;
                }
            }
        }
        else if (channel == SubstanceAir::Channel_Diffuse ||
                 channel == SubstanceAir::Channel_BaseColor ||
                 channel == SubstanceAir::Channel_Specular)
        {
            loadData.m_nFlags |= FIM_SRGB_READ;
        }
        else if (channel == SubstanceAir::Channel_Glossiness)
        {
            loadData.m_nFlags |= FIM_SPLITTED;
        }

        size_t dataSize = CalcTextureSize(loadData.m_Width, loadData.m_Height, loadData.m_NumMips, substanceTexture.pixelFormat);
        loadData.m_pData = static_cast<byte*>(loadData.m_dataAllocFunc(dataSize));
        memcpy(loadData.m_pData, substanceTexture.buffer, dataSize);
        loadData.m_DataSize = dataSize;

        //Set device texture
        if (loadData.m_pTexture)
        {
            gAPI->AddRefDeviceTexture(loadData.m_pTexture);
        }

        if (m_pDeviceTexture)
        {
            gAPI->ReleaseDeviceTexture(m_pDeviceTexture);
        }

        m_pDeviceTexture = loadData.m_pTexture;

        return true;
    }

    return false;
}

void CProceduralTexture::OnOutputComputed()
{
    if (m_pDeviceTexture != nullptr)
    {
        //TODO: We shouldn't go through this for ship because it requires a fair number of
        //steps such as trying to load texture images, etc. Instead we should provide an
        //STextureLoadData and reload from that
        gAPI->ReloadDeviceTexture(m_pDeviceTexture);
    }
}
#endif // AZ_USE_SUBSTANCE