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
#include "ProceduralTexture.h"
#include "SubstanceSystem.h"

// Forward declare platform specific functions
namespace Platform
{
    unsigned int GetPlatformPixelFormat(int32 format, SubstanceAir::ChannelUse channelUse);
}

//--------------------------------------------------------------------------------------------
template<typename T>
const char* TGetScalarFormatString();
template<>
const char* TGetScalarFormatString<float>()
{
    return "%f";
}
template<>
const char* TGetScalarFormatString<int>()
{
    return "%i";
}

template<typename T, typename ST>
void TSetInputScalar(SubstanceAir::InputInstanceBase* pInput, const char* xmlValue)
{
    T value;
    azsscanf(xmlValue, TGetScalarFormatString<T>(), &value);
    static_cast<ST*>(pInput)->setValue(value);
}

template<typename T, typename ST, typename VT>
void TSetInputVector(SubstanceAir::InputInstanceBase* pInput, const char* xmlValue)
{
    VT value;
    int valueIndex = 0;

    char* wValue = new char[strlen(xmlValue) + 1];
    azstrcpy(wValue, strlen(xmlValue) + 1, xmlValue);

    char* nextToken = nullptr;
    const char* token = azstrtok(wValue, strlen(xmlValue) + 1, ",", &nextToken);
    while (token)
    {
        T v;
        azsscanf(token, TGetScalarFormatString<T>(), &v);

        value[valueIndex] = v;
        ++valueIndex;

        if (valueIndex == VT::Size)
        {
            break;
        }

        token = azstrtok(NULL, 0, ",", &nextToken);
    }

    static_cast<ST*>(pInput)->setValue(value);

    delete [] wValue;
}

static void SetInput(CProceduralMaterial* pMaterial, SubstanceAir::InputInstanceBase* pInput, const char* xmlValue)
{
    switch (pInput->mDesc.mType)
    {
    case Substance_IType_Float:
        TSetInputScalar<float, SubstanceAir::InputInstanceFloat>(pInput, xmlValue);
        break;
    case Substance_IType_Float2:
        TSetInputVector<float, SubstanceAir::InputInstanceFloat2, SubstanceAir::Vec2Float>(pInput, xmlValue);
        break;
    case Substance_IType_Float3:
        TSetInputVector<float, SubstanceAir::InputInstanceFloat3, SubstanceAir::Vec3Float>(pInput, xmlValue);
        break;
    case Substance_IType_Float4:
        TSetInputVector<float, SubstanceAir::InputInstanceFloat4, SubstanceAir::Vec4Float>(pInput, xmlValue);
        break;
    case Substance_IType_Integer:
        TSetInputScalar<int, SubstanceAir::InputInstanceInt>(pInput, xmlValue);
        break;
    case Substance_IType_Integer2:
        TSetInputVector<int, SubstanceAir::InputInstanceInt2, SubstanceAir::Vec2Int>(pInput, xmlValue);
        break;
    case Substance_IType_Integer3:
        TSetInputVector<int, SubstanceAir::InputInstanceInt3, SubstanceAir::Vec3Int>(pInput, xmlValue);
        break;
    case Substance_IType_Integer4:
        TSetInputVector<int, SubstanceAir::InputInstanceInt4, SubstanceAir::Vec4Int>(pInput, xmlValue);
        break;
    case Substance_IType_Image:
        pMaterial->BindImageInput(static_cast<SubstanceAir::InputInstanceImage*>(pInput), xmlValue);
        break;
    case Substance_IType_String:
        static_cast<SubstanceAir::InputInstanceString*>(pInput)->setString(xmlValue);
        break;
    default:
        break;
    }
}

//--------------------------------------------------------------------------------------------
void ApplySubstanceParameterXML(CProceduralMaterial* pMaterial, const ISubstanceParameterXML* pParameter)
{
    //set parameters
    int graphIndex = pParameter->GetGraphIndex();

    if (graphIndex < 0 || graphIndex >= pMaterial->GetGraphInstanceCount())
    {
        return;
    }

    if (SubstanceAir::InputInstanceBase* pInput = pMaterial->GetInputByName(graphIndex, pParameter->GetName()))
    {
        SetInput(pMaterial, pInput, pParameter->GetValue());
    }
}

//--------------------------------------------------------------------------------------------
CProceduralMaterial::CProceduralMaterial(const char* path, const char* sourcePath, SubstanceAir::unique_ptr<SubstanceAir::PackageDesc> package)
    : IProceduralMaterial()
    , m_Path(path)
    , m_SubstancePath(sourcePath)
    , m_ID(INVALID_PROCEDURALMATERIALID)
    , m_Package(std::move(package))
    , m_LastResetID(0)
{
}

CProceduralMaterial::~CProceduralMaterial()
{
    pSubstanceSystem->OnMaterialDestroyed(this);

    Dispose();
}

void CProceduralMaterial::Init()
{
    //instantiate graph instances
    SubstanceAir::instantiate(m_Graphs, *m_Package);

    m_GraphHandles.clear();
    m_GraphHandles.reserve(m_Graphs.size());
    for (auto& pGraph : m_Graphs)
    {
        m_GraphHandles.push_back(CGraphInstance(pGraph.get()));

        //add editor outputs for preview purposes
        if (gAPI->IsEditor())
        {
            std::vector<std::pair<SubstanceAir::OutputFormat, SubstanceAir::OutputDesc> > previewOutputs;

            for (auto& output : pGraph->getOutputs())
            {
                SubstanceAir::OutputFormat format;
                SubstanceAir::OutputDesc desc;

                if (output->mDesc.mFormat & Substance_PF_L)
                {
                    format.format = Substance_PF_L;
                }
                else
                {
                    format.format = Substance_PF_RGBA;

                    //swizzle components around since Qt expects BGR
                    format.perComponent[0].shuffleIndex = 2;
                    format.perComponent[1].shuffleIndex = 1;
                    format.perComponent[2].shuffleIndex = 0;
                }

                for (int i = 0; i < SubstanceAir::OutputFormat::ComponentsCount; i++)
                {
                    format.perComponent[i].outputUid = output->mDesc.mUid;
                }

                //We don't need mip maps for editor preview
                format.mipmapLevelsCount = SubstanceAir::OutputFormat::MipmapNone;

                //We actually DO need to preserve width/height because we use editor preview images
                //in order to allow bake to disk feature within the Substance Editor
                //format.forceWidth = 256;
                //format.forceHeight = 256;

                desc.mIdentifier = output->mDesc.mIdentifier;
                desc.mFormat = format.format;
                desc.mChannel = SubstanceAir::Channel_UNKNOWN;
                desc.mMipmaps = 1;
                desc.mLabel = "Editor Preview";
                desc.mUid = output->mDesc.mUid; //just embedding this here so we can store it later

                previewOutputs.push_back(std::make_pair(format, desc));
            }

            for (auto& poutput : previewOutputs)
            {
                SubstanceAir::OutputInstance* editorOutput = pGraph->createOutput(poutput.first, poutput.second);
                editorOutput->mUserData = poutput.second.mUid;
            }
        }
    }
}

void CProceduralMaterial::Dispose()
{
    //delete texture objects
    for (auto iter = m_Textures.begin(); iter != m_Textures.end(); iter++)
    {
        delete *iter;
    }
    m_Textures.clear();

    m_Graphs.clear();
    m_GraphHandles.clear();
    m_LastResetID = 0;
    m_ImageInputs.clear();
    m_Package.reset(nullptr);
}

SubstanceAir::OutputFormat CProceduralMaterial::CreateOutputFormat(SubstanceAir::OutputInstance* output, const SubstanceAir::OutputFormat& baseFormat, bool bCompressed)
{
    SubstanceAir::OutputFormat newFormat = baseFormat;

    //should this texture be compressed?
    if (bCompressed)
    {
        unsigned int pixelFormat = Platform::GetPlatformPixelFormat(output->mDesc.mFormat, output->mDesc.mChannel);

        if (pixelFormat != ~0u)
        {
            newFormat.format = pixelFormat;
        }
    }

    return newFormat;
}

CProceduralTexture* CProceduralMaterial::GetTextureByUID(SubstanceUID uid) const
{
    for (auto iter = m_Textures.begin(); iter != m_Textures.end(); iter++)
    {
        CProceduralTexture* texture = *iter;

        if (texture->GetOutputInstance()->mDesc.mUid == uid)
        {
            return texture;
        }
    }

    return nullptr;
}

SubstanceAir::InputInstanceBase* CProceduralMaterial::GetInputByName(int graphIndex, const char* name) const
{
    assert(graphIndex >= 0 && graphIndex < m_Graphs.size());
    return GetInputByName(m_Graphs[graphIndex].get(), name);
}

SubstanceAir::InputInstanceBase* CProceduralMaterial::GetInputByName(SubstanceAir::GraphInstance* pGraphInstance, const char* name)
{
    for (auto iter = pGraphInstance->getInputs().begin(); iter != pGraphInstance->getInputs().end(); iter++)
    {
        SubstanceAir::InputInstanceBase* pInput = *iter;
        if (pInput->mDesc.mIdentifier == name)
        {
            return pInput;
        }
    }
    return nullptr;
}

void CProceduralMaterial::BindImageInput(SubstanceAir::InputInstanceImage* pImage, const char* path)
{
    m_ImageInputs[pImage] = path;
}

const char* CProceduralMaterial::GetInputImagePath(SubstanceAir::InputInstanceImage* pImage) const
{
    auto iter = m_ImageInputs.find(pImage);
    if (iter != m_ImageInputs.end())
    {
        return iter->second.c_str();
    }
    return "";
}

void CProceduralMaterial::AddImageInputs(SubstanceAir::GraphInstance* pGraphInstance, InputImageMap& mapOut) const
{
    for (auto iter = m_ImageInputs.begin(); iter != m_ImageInputs.end(); iter++)
    {
        SubstanceAir::InputInstanceImage* pInput = iter->first;
        const string& path = iter->second;

        if (pGraphInstance == nullptr || pInput->mParentGraph.mInstanceUid == pGraphInstance->mInstanceUid)
        {
            mapOut[pInput] = path;
        }
    }
}

void CProceduralMaterial::Reset(int nLastResetID)
{
    //in case this material has already been reset through dependency, skip
    if (m_LastResetID == nLastResetID)
    {
        return;
    }

    //reset values to default (this is sbsar only)
    for (auto iter = m_Graphs.begin(); iter != m_Graphs.end(); iter++)
    {
        SubstanceAir::shared_ptr<SubstanceAir::GraphInstance> pGraph = *iter;

        for (auto iiter = pGraph->getInputs().begin(); iiter != pGraph->getInputs().end(); iiter++)
        {
            SubstanceAir::InputInstanceBase* pInput = *iiter;
            pInput->reset();
        }
    }

    if (ISubstanceMaterialXMLData* pXmlData = gAPI->LoadMaterialXML(GetPath(), true))
    {
        for (int i = 0; i < pXmlData->GetParameterCount(); i++)
        {
            ApplySubstanceParameterXML(this, pXmlData->GetParameter(i));
        }

        pXmlData->Release();
    }

    //issue render command
    pSubstanceSystem->QueueRender(this);
    pSubstanceSystem->RenderSync();

    //save reset ID
    m_LastResetID = nLastResetID;
}

IGraphInstance* CProceduralMaterial::GetIGraphInstance(SubstanceAir::GraphInstance* pGraph)
{
    for (int i = 0; i < m_GraphHandles.size(); i++)
    {
        if (m_GraphHandles[i].GetSubstanceGraph() == pGraph)
        {
            return &(m_GraphHandles[i]);
        }
    }
    return nullptr;
}

SubstanceAir::GraphInstance* CProceduralMaterial::GetSubstanceGraphInstance(int index) const
{
    return m_Graphs[index].get();
}

CProceduralMaterial* CProceduralMaterial::CreateFromPath(const char* path)
{
    //see if material has already been loaded
    CProceduralMaterial* pMaterial = pSubstanceSystem->GetMaterialFromPath(path);
    if (pMaterial)
    {
        return pMaterial;
    }

    string sPath(path);

    if (ISubstanceMaterialXMLData* pXmlData = gAPI->LoadMaterialXML(sPath.c_str()))
    {
        //we only support int and compressed formats right now
        SubstanceAir::OutputOptions packageOptions;
        packageOptions.mAllowedFormats = SubstanceAir::Format_Mask_Raw8bpc | SubstanceAir::Format_Mask_Compressed;

        //load substance archive
        SubstanceAir::unique_ptr<SubstanceAir::PackageDesc> package(
            AIR_NEW(SubstanceAir::PackageDesc)(pXmlData->GetSourceData(), pXmlData->GetSourceDataSize(), packageOptions)
            );

        if (package->isValid() == false)
        {
            AZ_Error("Substance", false, "ProceduralMaterial: Error loading substance (%s) for material (%s)", pXmlData->GetSource(), sPath.c_str());
            pXmlData->Release();
            return nullptr;
        }

        //instance material and graphs
        CProceduralMaterial* pMaterial = new CProceduralMaterial(sPath.c_str(), pXmlData->GetSource(), std::move(package));
        pMaterial->Init();

        //apply xml parameter values
        for (int i = 0; i < pXmlData->GetParameterCount(); i++)
        {
            ApplySubstanceParameterXML(pMaterial, pXmlData->GetParameter(i));
        }

        //add procedural textures and bind user data
        for (auto& graph : pMaterial->m_Graphs)
        {
            graph->mUserData = reinterpret_cast<size_t>(pMaterial);

            for (auto& output : graph->getOutputs())
            {
                const SubstanceAir::OutputDesc& desc = output->mDesc;
                bool bCompressed = true;
                SubstanceAir::OutputFormat outputFormat;

                //ignore dynamic generated outputs (used for editor previews)
                if (output->isDynamicOutput())
                {
                    continue;
                }

                //see if we have any output settings
                if (const SSubstanceOutputInfoXML* outputXML = pXmlData->GetOutputInfo(desc.mUid))
                {
                    if (!outputXML->bEnabled)
                    {
                        output->mEnabled = false;
                    }

                    bCompressed = outputXML->bCompressed;
                    outputFormat = outputXML->OverrideFormat;
                }

                //create texture
                CProceduralTexture* pTexture = new CProceduralTexture(pMaterial, output);
                pTexture->Init(CreateOutputFormat(output, outputFormat, bCompressed));
                pMaterial->m_Textures.push_back(pTexture);
                output->mUserData = reinterpret_cast<size_t>(pTexture);
            }
        }

        //let substancesystem know we've been instanced (mem tracking)
        pSubstanceSystem->OnMaterialInstanced(pMaterial);

        //issue an initial render of the texture data
        pSubstanceSystem->QueueRender(pMaterial);
        pSubstanceSystem->RenderSync(ProceduralMaterialRenderMode::SkipReloadTextures);

        pXmlData->Release();
        return pMaterial;
    }

    return nullptr;
}

CProceduralMaterial* CProceduralMaterial::GetFromGraphInstance(SubstanceAir::GraphInstance* pGraphInstance)
{
    return reinterpret_cast<CProceduralMaterial*>(pGraphInstance->mUserData);
}

int CProceduralMaterial::GetGraphInstanceCount() const
{
    return (int)m_Graphs.size();
}

IGraphInstance* CProceduralMaterial::GetGraphInstance(int index)
{
    AZ_Assert(0 <= index && index < m_GraphHandles.size(), "Index %d out of range. %d graphs available.", index, m_GraphHandles.size());
    return &(m_GraphHandles[index]);
}

void CProceduralMaterial::ReimportSubstance()
{
    //make sure no render ops are pending
    pSubstanceSystem->RenderFence();

    std::list<string> stringStorage;
    std::map<GraphInputID, GraphValueVariant> inputValueMap;
    std::map<GraphOutputID, std::pair<bool, SubstanceAir::OutputFormat> > outputValueMap;
    std::map<GraphOutputID, IDeviceTexture*> outputDeviceTextureMap;

    //steal device texture pointers from texture objects
    for (auto iter = m_Textures.begin(); iter != m_Textures.end(); iter++)
    {
        CProceduralTexture* pTexture = *iter;

        if (pTexture->GetDeviceTexture() != nullptr)
        {
            outputDeviceTextureMap[pTexture->GetOutputInstance()->mDesc.mUid] = pTexture->GetDeviceTexture();
            pTexture->SetDeviceTexture(nullptr);
        }
    }

    //save some data
    for (auto iter = m_Graphs.begin(); iter != m_Graphs.end(); iter++)
    {
        SubstanceAir::GraphInstance* pGraph = (*iter).get();
        size_t index = std::distance(m_Graphs.begin(), iter);
        CGraphInstance* pGraphInstance = &(m_GraphHandles[index]);

        //save changed inputs
        for (auto iiter = pGraph->getInputs().begin(); iiter != pGraph->getInputs().end(); iiter++)
        {
            SubstanceAir::InputInstanceBase* pInput = *iiter;

            if (pInput->isNonDefault())
            {
                if (IGraphInput* pGraphInput = pGraphInstance->GetInputByID(pInput->mDesc.mUid))
                {
                    GraphValueVariant value;

                    //strings need to be backed, since we will be deleting the underlying data structure
                    if (pGraphInput->GetInputType() == GraphInputType::Image)
                    {
                        stringStorage.push_back(string(pGraphInput->GetValue()));
                        value = stringStorage.back().c_str();
                    }
                    else
                    {
                        value = pGraphInput->GetValue();
                    }

                    inputValueMap[pGraphInput->GetGraphInputID()] = value;
                }
            }
        }

        //save output info
        for (auto oiter = pGraph->getOutputs().begin(); oiter != pGraph->getOutputs().end(); oiter++)
        {
            SubstanceAir::OutputInstance* pOutput = *oiter;

            for (auto titer = m_Textures.begin(); titer != m_Textures.end(); titer++)
            {
                CProceduralTexture* pTexture = *titer;

                if (pTexture->GetOutputInstance() == pOutput)
                {
                    bool bCompressed = pTexture->IsCompressed();
                    bool bEnabled = pOutput->mEnabled;
                    GraphOutputID outputID = pTexture->GetOutputInstance()->mDesc.mUid;

                    outputValueMap[outputID] = std::make_pair(bEnabled, pTexture->GetOutputInstance()->getFormatOverride());
                }

                //remove sub file
                string subPath = CGraphOutput::GetPath(pOutput, (int)m_Graphs.size(), m_Path.c_str());
                gAPI->RemoveFile(subPath.c_str());
            }
        }
    }

    //remove smtl file
    gAPI->RemoveFile(m_Path.c_str());

    //reset all member variables/datasets
    Dispose();

    //reload substance archive
    IFileContents* pFileContents = nullptr;

    if (!gAPI->ReadFile(m_SubstancePath.c_str(), &pFileContents))
    {
        AZ_Error("Substance", false, "ProceduralMaterial: Unable to load substance (%s) for material (%s)", m_SubstancePath.c_str(), m_Path.c_str());
        for (auto texture : outputDeviceTextureMap)
        {
            gAPI->ReleaseDeviceTexture(texture.second);
        }
        return;
    }

    SubstanceAir::unique_ptr<SubstanceAir::PackageDesc> package(AIR_NEW(SubstanceAir::PackageDesc)(pFileContents->GetData(), pFileContents->GetDataSize()));

    pFileContents->Release();

    if (package->isValid() == false)
    {
        AZ_Error("Substance", false, "ProceduralMaterial: Error reading substance (%s) for material (%s)", m_SubstancePath.c_str(), m_Path.c_str());
        for (auto texture : outputDeviceTextureMap)
        {
            gAPI->ReleaseDeviceTexture(texture.second);
        }
        return;
    }

    m_Package = std::move(package);

    Init();

    //reload textures
    for (auto giter = m_Graphs.begin(); giter != m_Graphs.end(); giter++)
    {
        SubstanceAir::shared_ptr<SubstanceAir::GraphInstance> graph = (*giter);
        IGraphInstance* pGraphInstance = &(m_GraphHandles[std::distance(m_Graphs.begin(), giter)]);

        graph->mUserData = reinterpret_cast<size_t>(this);

        //reload outputs
        for (auto& pOutput : graph->getOutputs())
        {
            const SubstanceAir::OutputDesc& desc = pOutput->mDesc;
            SubstanceAir::OutputFormat format;

            //ignore dynamic outputs (editor previews)
            if (pOutput->isDynamicOutput())
            {
                continue;
            }

            //see if we have any output settings
            auto settingsIter = outputValueMap.find(desc.mUid);
            if (settingsIter != outputValueMap.end())
            {
                pOutput->mEnabled = settingsIter->second.first;
                format = settingsIter->second.second;
            }

            //create texture
            CProceduralTexture* pTexture = new CProceduralTexture(this, pOutput);
            pTexture->Init(format);
            m_Textures.push_back(pTexture);
            pOutput->mUserData = reinterpret_cast<size_t>(pTexture);

            //rebind device texture
            auto dtIter = outputDeviceTextureMap.find(pOutput->mDesc.mUid);
            if (dtIter != outputDeviceTextureMap.end())
            {
                pTexture->SetDeviceTexture(dtIter->second);
                outputDeviceTextureMap.erase(dtIter);
            }
        }

        //apply input values
        for (auto iiter = graph->getInputs().begin(); iiter != graph->getInputs().end(); iiter++)
        {
            SubstanceAir::InputInstanceBase* pInput = *iiter;

            auto inputMapIter = inputValueMap.find(pInput->mDesc.mUid);
            if (inputMapIter != inputValueMap.end())
            {
                if (IGraphInput* pGraphInput = pGraphInstance->GetInputByID(pInput->mDesc.mUid))
                {
                    pGraphInput->SetValue(inputMapIter->second);
                }
            }
        }
    }

    //save material to commit new smtl/sub files
    gLibAPI->SaveProceduralMaterial(this, m_Path.c_str());

    //we may have leftover textures, release them at this point
    for (auto texture : outputDeviceTextureMap)
    {
        gAPI->ReleaseDeviceTexture(texture.second);
    }
}

//--------------------------------------------------------------------------------------------
CGraphInput::CGraphInput(SubstanceAir::InputInstanceBase* pInput)
    : m_Input(pInput)
{
}

IGraphInstance* CGraphInput::GetGraphInstance() const
{
    CProceduralMaterial* pMaterial = CProceduralMaterial::GetFromGraphInstance(&m_Input->mParentGraph);
    return pMaterial->GetIGraphInstance(&m_Input->mParentGraph);
}

GraphInputID CGraphInput::GetGraphInputID() const
{
    return m_Input->mDesc.mUid;
}

const char* CGraphInput::GetDescription() const
{
    return m_Input->mDesc.mGuiDescription.c_str();
}

const char* CGraphInput::GetLabel() const
{
    return m_Input->mDesc.mLabel.c_str();
}

const char* CGraphInput::GetName() const
{
    return m_Input->mDesc.mLabel.c_str();
}

const char* CGraphInput::GetGroupName() const
{
    return m_Input->mDesc.mGuiGroup.c_str();
}

GraphInputType CGraphInput::GetInputType() const
{
    struct InputMap
    {
        SubstanceInputType substanceType;
        GraphInputType azType;
    };

    static InputMap inputMap[] =
    {
        { Substance_IType_Float, GraphInputType::Float1 },
        { Substance_IType_Float2, GraphInputType::Float2 },
        { Substance_IType_Float3, GraphInputType::Float3 },
        { Substance_IType_Float4, GraphInputType::Float4 },
        { Substance_IType_Integer, GraphInputType::Integer1 },
        { Substance_IType_Integer2, GraphInputType::Integer2 },
        { Substance_IType_Integer3, GraphInputType::Integer3 },
        { Substance_IType_Integer4, GraphInputType::Integer4 },
        { Substance_IType_Image, GraphInputType::Image },
        { Substance_IType_String, GraphInputType::String }
    };

    for (int i = 0; i < DIMOF(inputMap); i++)
    {
        if (inputMap[i].substanceType == m_Input->mDesc.mType)
        {
            return inputMap[i].azType;
        }
    }

    return GraphInputType::None;
}

GraphInputWidgetType CGraphInput::GetInputWidgetType() const
{
    struct WidgetMap
    {
        SubstanceAir::InputWidget substanceWidget;
        GraphInputWidgetType azWidget;
    };

    static WidgetMap widgetMap[] =
    {
        { SubstanceAir::Input_Slider, GraphInputWidgetType::Slider },
        { SubstanceAir::Input_Angle, GraphInputWidgetType::Angle },
        { SubstanceAir::Input_Color, GraphInputWidgetType::Color },
        { SubstanceAir::Input_Togglebutton, GraphInputWidgetType::Boolean },
        { SubstanceAir::Input_Combobox, GraphInputWidgetType::Combobox },
    };

    for (int i = 0; i < DIMOF(widgetMap); i++)
    {
        if (widgetMap[i].substanceWidget == m_Input->mDesc.mGuiWidget)
        {
            return widgetMap[i].azWidget;
        }
    }

    return GraphInputWidgetType::None;
}

template<typename T>
GraphValueVariant ToGraphValueVariant(const T& in)
{
    return GraphValueVariant(in);
}
template<>
GraphValueVariant ToGraphValueVariant(const SubstanceAir::Vec2Int& in)
{
    return GraphValueVariant(in.x, in.y);
}
template<>
GraphValueVariant ToGraphValueVariant(const SubstanceAir::Vec3Int& in)
{
    return GraphValueVariant(in.x, in.y, in.z);
}
template<>
GraphValueVariant ToGraphValueVariant(const SubstanceAir::Vec4Int& in)
{
    return GraphValueVariant(in.x, in.y, in.z, in.w);
}
template<>
GraphValueVariant ToGraphValueVariant(const SubstanceAir::Vec2Float& in)
{
    return GraphValueVariant(in.x, in.y);
}
template<>
GraphValueVariant ToGraphValueVariant(const SubstanceAir::Vec3Float& in)
{
    return GraphValueVariant(in.x, in.y, in.z);
}
template<>
GraphValueVariant ToGraphValueVariant(const SubstanceAir::Vec4Float& in)
{
    return GraphValueVariant(in.x, in.y, in.z, in.w);
}

template<typename T>
T FromGraphValueVariant(const GraphValueVariant& gv)
{
    return (T)gv;
}
template<>
SubstanceAir::Vec2Int FromGraphValueVariant(const GraphValueVariant& gv)
{
    const int* vec = gv;
    return SubstanceAir::Vec2Int(vec[0], vec[1]);
}
template<>
SubstanceAir::Vec3Int FromGraphValueVariant(const GraphValueVariant& gv)
{
    const int* vec = gv;
    return SubstanceAir::Vec3Int(vec[0], vec[1], vec[2]);
}
template<>
SubstanceAir::Vec4Int FromGraphValueVariant(const GraphValueVariant& gv)
{
    const int* vec = gv;
    return SubstanceAir::Vec4Int(vec[0], vec[1], vec[2], vec[3]);
}
template<>
SubstanceAir::Vec2Float FromGraphValueVariant(const GraphValueVariant& gv)
{
    const float* vec = gv;
    return SubstanceAir::Vec2Float(vec[0], vec[1]);
}
template<>
SubstanceAir::Vec3Float FromGraphValueVariant(const GraphValueVariant& gv)
{
    const float* vec = gv;
    return SubstanceAir::Vec3Float(vec[0], vec[1], vec[2]);
}
template<>
SubstanceAir::Vec4Float FromGraphValueVariant(const GraphValueVariant& gv)
{
    const float* vec = gv;
    return SubstanceAir::Vec4Float(vec[0], vec[1], vec[2], vec[3]);
}

enum class GetValueKind
{
    Value,
    MinValue,
    MaxValue
};

template<typename T>
GraphValueVariant TGetValue(SubstanceAir::InputInstanceBase* pInput, GetValueKind valueKind)
{
    T* castInput = static_cast<T*>(pInput);
    return ToGraphValueVariant((valueKind == GetValueKind::Value) ? castInput->getValue() :
        (valueKind == GetValueKind::MinValue) ? castInput->getDesc().mMinValue : castInput->getDesc().mMaxValue);
}

template<typename T>
void TSetValue(SubstanceAir::InputInstanceBase* pInput, const GraphValueVariant& value)
{
    static_cast<T*>(pInput)->setValue(FromGraphValueVariant<typename T::Desc::Type>(value));
}

GraphValueVariant CGraphInput::GetValue() const
{
    switch (m_Input->mDesc.mType)
    {
    case Substance_IType_Integer:
        return TGetValue<SubstanceAir::InputInstanceInt>(m_Input, GetValueKind::Value);
    case Substance_IType_Integer2:
        return TGetValue<SubstanceAir::InputInstanceInt2>(m_Input, GetValueKind::Value);
    case Substance_IType_Integer3:
        return TGetValue<SubstanceAir::InputInstanceInt3>(m_Input, GetValueKind::Value);
    case Substance_IType_Integer4:
        return TGetValue<SubstanceAir::InputInstanceInt4>(m_Input, GetValueKind::Value);
    case Substance_IType_Float:
        return TGetValue<SubstanceAir::InputInstanceFloat>(m_Input, GetValueKind::Value);
    case Substance_IType_Float2:
        return TGetValue<SubstanceAir::InputInstanceFloat2>(m_Input, GetValueKind::Value);
    case Substance_IType_Float3:
        return TGetValue<SubstanceAir::InputInstanceFloat3>(m_Input, GetValueKind::Value);
    case Substance_IType_Float4:
        return TGetValue<SubstanceAir::InputInstanceFloat4>(m_Input, GetValueKind::Value);
    case Substance_IType_Image:
        return CProceduralMaterial::GetFromGraphInstance(&m_Input->mParentGraph)->
                   GetInputImagePath(static_cast<SubstanceAir::InputInstanceImage*>(m_Input));
    case Substance_IType_String:
        return static_cast<SubstanceAir::InputInstanceString*>(m_Input)->getString().c_str();
    default:
        break;
    }

    return 0;
}

void CGraphInput::SetValue(const GraphValueVariant& value)
{
    switch (m_Input->mDesc.mType)
    {
    case Substance_IType_Integer:
        TSetValue<SubstanceAir::InputInstanceInt>(m_Input, value);
        break;
    case Substance_IType_Integer2:
        TSetValue<SubstanceAir::InputInstanceInt2>(m_Input, value);
        break;
    case Substance_IType_Integer3:
        TSetValue<SubstanceAir::InputInstanceInt3>(m_Input, value);
        break;
    case Substance_IType_Integer4:
        TSetValue<SubstanceAir::InputInstanceInt4>(m_Input, value);
        break;
    case Substance_IType_Float:
        TSetValue<SubstanceAir::InputInstanceFloat>(m_Input, value);
        break;
    case Substance_IType_Float2:
        TSetValue<SubstanceAir::InputInstanceFloat2>(m_Input, value);
        break;
    case Substance_IType_Float3:
        TSetValue<SubstanceAir::InputInstanceFloat3>(m_Input, value);
        break;
    case Substance_IType_Float4:
        TSetValue<SubstanceAir::InputInstanceFloat4>(m_Input, value);
        break;
    case Substance_IType_Image:
    {
        CProceduralMaterial::GetFromGraphInstance(&m_Input->mParentGraph)->
            BindImageInput(static_cast<SubstanceAir::InputInstanceImage*>(m_Input), (const char*)value);
    }
        return;
    case Substance_IType_String:
        static_cast<SubstanceAir::InputInstanceString*>(m_Input)->setString((const char*)value);
        break;
    default:
        break;
    }
}

GraphValueVariant CGraphInput::GetMinValue() const
{
    switch (m_Input->mDesc.mType)
    {
    case Substance_IType_Integer:
        return TGetValue<SubstanceAir::InputInstanceInt>(m_Input, GetValueKind::MinValue);
    case Substance_IType_Integer2:
        return TGetValue<SubstanceAir::InputInstanceInt2>(m_Input, GetValueKind::MinValue);
    case Substance_IType_Integer3:
        return TGetValue<SubstanceAir::InputInstanceInt3>(m_Input, GetValueKind::MinValue);
    case Substance_IType_Integer4:
        return TGetValue<SubstanceAir::InputInstanceInt4>(m_Input, GetValueKind::MinValue);
    case Substance_IType_Float:
        return TGetValue<SubstanceAir::InputInstanceFloat>(m_Input, GetValueKind::MinValue);
    case Substance_IType_Float2:
        return TGetValue<SubstanceAir::InputInstanceFloat2>(m_Input, GetValueKind::MinValue);
    case Substance_IType_Float3:
        return TGetValue<SubstanceAir::InputInstanceFloat3>(m_Input, GetValueKind::MinValue);
    case Substance_IType_Float4:
        return TGetValue<SubstanceAir::InputInstanceFloat4>(m_Input, GetValueKind::MinValue);
    default:
        break;
    }

    return 0;
}

GraphValueVariant CGraphInput::GetMaxValue() const
{
    switch (m_Input->mDesc.mType)
    {
    case Substance_IType_Integer:
        return TGetValue<SubstanceAir::InputInstanceInt>(m_Input, GetValueKind::MaxValue);
    case Substance_IType_Integer2:
        return TGetValue<SubstanceAir::InputInstanceInt2>(m_Input, GetValueKind::MaxValue);
    case Substance_IType_Integer3:
        return TGetValue<SubstanceAir::InputInstanceInt3>(m_Input, GetValueKind::MaxValue);
    case Substance_IType_Integer4:
        return TGetValue<SubstanceAir::InputInstanceInt4>(m_Input, GetValueKind::MaxValue);
    case Substance_IType_Float:
        return TGetValue<SubstanceAir::InputInstanceFloat>(m_Input, GetValueKind::MaxValue);
    case Substance_IType_Float2:
        return TGetValue<SubstanceAir::InputInstanceFloat2>(m_Input, GetValueKind::MaxValue);
    case Substance_IType_Float3:
        return TGetValue<SubstanceAir::InputInstanceFloat3>(m_Input, GetValueKind::MaxValue);
    case Substance_IType_Float4:
        return TGetValue<SubstanceAir::InputInstanceFloat4>(m_Input, GetValueKind::MaxValue);
    default:
        break;
    }

    return 0;
}

int CGraphInput::GetEnumCount() const
{
    switch (m_Input->mDesc.mType)
    {
    case Substance_IType_Integer:
        return (int)static_cast<SubstanceAir::InputInstanceInt*>(m_Input)->getDesc().mEnumValues.size();
    case Substance_IType_Integer2:
        return (int)static_cast<SubstanceAir::InputInstanceInt2*>(m_Input)->getDesc().mEnumValues.size();
    case Substance_IType_Integer3:
        return (int)static_cast<SubstanceAir::InputInstanceInt3*>(m_Input)->getDesc().mEnumValues.size();
    case Substance_IType_Integer4:
        return (int)static_cast<SubstanceAir::InputInstanceInt4*>(m_Input)->getDesc().mEnumValues.size();
    case Substance_IType_Float:
        return (int)static_cast<SubstanceAir::InputInstanceFloat*>(m_Input)->getDesc().mEnumValues.size();
    case Substance_IType_Float2:
        return (int)static_cast<SubstanceAir::InputInstanceFloat2*>(m_Input)->getDesc().mEnumValues.size();
    case Substance_IType_Float3:
        return (int)static_cast<SubstanceAir::InputInstanceFloat3*>(m_Input)->getDesc().mEnumValues.size();
    case Substance_IType_Float4:
        return (int)static_cast<SubstanceAir::InputInstanceFloat4*>(m_Input)->getDesc().mEnumValues.size();
    default:
        return 0;
    }
}

template<class T>
GraphEnumValue TGetGraphEnumValue(SubstanceAir::InputInstanceBase* pInput, int index)
{
    GraphEnumValue enumValue;
    auto& enumPair = static_cast<T*>(pInput)->getDesc().mEnumValues[index];
    enumValue.value = ToGraphValueVariant<typename T::Desc::Type>(enumPair.first);
    enumValue.label = enumPair.second.c_str();
    return enumValue;
}

GraphEnumValue CGraphInput::GetEnumValue(int index)
{
    switch (m_Input->mDesc.mType)
    {
    case Substance_IType_Integer:
        return TGetGraphEnumValue<SubstanceAir::InputInstanceInt>(m_Input, index);
    case Substance_IType_Integer2:
        return TGetGraphEnumValue<SubstanceAir::InputInstanceInt2>(m_Input, index);
    case Substance_IType_Integer3:
        return TGetGraphEnumValue<SubstanceAir::InputInstanceInt3>(m_Input, index);
    case Substance_IType_Integer4:
        return TGetGraphEnumValue<SubstanceAir::InputInstanceInt4>(m_Input, index);
    case Substance_IType_Float:
        return TGetGraphEnumValue<SubstanceAir::InputInstanceFloat>(m_Input, index);
    case Substance_IType_Float2:
        return TGetGraphEnumValue<SubstanceAir::InputInstanceFloat2>(m_Input, index);
    case Substance_IType_Float3:
        return TGetGraphEnumValue<SubstanceAir::InputInstanceFloat3>(m_Input, index);
    case Substance_IType_Float4:
        return TGetGraphEnumValue<SubstanceAir::InputInstanceFloat4>(m_Input, index);
    default:
    {
        GraphEnumValue gev;
        gev.label = "";
        gev.value = 0;
        return gev;
    }
    }
}

//--------------------------------------------------------------------------------------------
CGraphOutput::CGraphOutput(SubstanceAir::OutputInstance* pOutput)
    : m_Output(pOutput)
{
}

string CGraphOutput::GetPath(SubstanceAir::OutputInstance* pOutput, int numGraphs, const char* smtlPath)
{
    SubstanceAir::GraphInstance& graphInstance = pOutput->mParentGraph;
    string smtlName = PathUtil::GetFileName(smtlPath);
    string subPath = PathUtil::AddSlash(PathUtil::GetPath(smtlPath)) + smtlName;

    //multiple graphs need more unique paths
    if (numGraphs > 1)
    {
        string graphLabel(graphInstance.mDesc.mLabel.c_str());

        graphLabel = CryStringUtils::ToLower(graphLabel);

        subPath = subPath + string("_") + graphLabel;
    }

    auto resolveOutputSuffix = [pOutput]()
        {
            if (pOutput->mDesc.mChannel == SubstanceAir::Channel_Diffuse ||
                pOutput->mDesc.mChannel == SubstanceAir::Channel_BaseColor)
            {
                return "_diff";
            }
            else if (pOutput->mDesc.mChannel == SubstanceAir::Channel_Specular)
            {
                return "_spec";
            }
            else if (pOutput->mDesc.mChannel == SubstanceAir::Channel_Height)
            {
                return "_displ";
            }
            else if (pOutput->mDesc.mChannel == SubstanceAir::Channel_Glossiness)
            {
                return "_gloss";
            }
            else if (pOutput->mDesc.mChannel == SubstanceAir::Channel_Normal)
            {
                for (auto& output : pOutput->mParentGraph.getOutputs())
                {
                    if (output->mDesc.mChannel == SubstanceAir::Channel_Glossiness)
                    {
                        return "_ddna";
                    }
                }

                return "_ddn";
            }
            else
            {
                return "_unknown";
            }
        };

    return subPath + resolveOutputSuffix() + string("." PROCEDURALTEXTURE_EXTENSION);
}

CGraphOutput* CGraphOutput::GetFromOutputInstance(SubstanceAir::OutputInstance* pOutput)
{
    if (CGraphInstance* pGraph = CGraphInstance::GetFromGraphInstance(&(pOutput->mParentGraph)))
    {
        for (int o = 0; o < pGraph->GetOutputCount(); o++)
        {
            CGraphOutput* pCompareOutput = static_cast<CGraphOutput*>(pGraph->GetOutput(o));
            if (pCompareOutput->m_Output == pOutput)
            {
                return pCompareOutput;
            }
        }
    }

    return nullptr;
}

IGraphInstance* CGraphOutput::GetGraphInstance() const
{
    return CGraphInstance::GetFromGraphInstance(&(m_Output->mParentGraph));
}

GraphOutputID CGraphOutput::GetGraphOutputID() const
{
    return m_Output->mDesc.mUid;
}

const char* CGraphOutput::GetLabel() const
{
    return m_Output->mDesc.mIdentifier.c_str();
}

const char* CGraphOutput::GetPath() const
{
    if (m_Path.size() == 0)
    {
        IProceduralMaterial* pMaterial = GetGraphInstance()->GetProceduralMaterial();
        m_Path = CGraphOutput::GetPath(m_Output, pMaterial->GetGraphInstanceCount(), pMaterial->GetPath());
    }

    return m_Path.c_str();
}

bool CGraphOutput::IsEnabled() const
{
    return m_Output->mEnabled;
}

void CGraphOutput::SetEnabled(bool bEnabled)
{
    m_Output->mEnabled = bEnabled;
}

bool CGraphOutput::IsDirty() const
{
    return m_Output->isDirty();
}

void CGraphOutput::SetDirty()
{
    m_Output->flagAsDirty();
}

SubstanceAir::RenderResult* CGraphOutput::GetEditorPreview() const
{
    if (CProceduralTexture* pTexture = CProceduralTexture::GetFromOutputInstance(m_Output))
    {
        return pTexture->GetEditorPreview();
    }

    return nullptr;
}

SubstanceAir::ChannelUse CGraphOutput::GetChannel() const
{
    return m_Output->mDesc.mChannel;
}

//--------------------------------------------------------------------------------------------
CGraphInstance::CGraphInstance(SubstanceAir::GraphInstance* pGraph)
    : m_Graph(pGraph)
{
    //create input handles
    m_Inputs.reserve(m_Graph->getInputs().size());
    for (auto iter = m_Graph->getInputs().begin(); iter != m_Graph->getInputs().end(); iter++)
    {
        m_Inputs.push_back(CGraphInput(*iter));
    }

    //create output handles
    m_Outputs.reserve(m_Graph->getOutputs().size());
    for (auto iter = m_Graph->getOutputs().begin(); iter != m_Graph->getOutputs().end(); iter++)
    {
        m_Outputs.push_back(CGraphOutput(*iter));
    }
}

CGraphInstance* CGraphInstance::GetFromGraphInstance(SubstanceAir::GraphInstance* pGraph)
{
    if (CProceduralMaterial* pMaterial = CProceduralMaterial::GetFromGraphInstance(pGraph))
    {
        for (int i = 0; i < pMaterial->GetGraphInstanceCount(); i++)
        {
            CGraphInstance* pGraphCompare = static_cast<CGraphInstance*>(pMaterial->GetGraphInstance(i));
            if (pGraphCompare->m_Graph->mInstanceUid == pGraph->mInstanceUid)
            {
                return pGraphCompare;
            }
        }
    }

    return nullptr;
}

IProceduralMaterial* CGraphInstance::GetProceduralMaterial() const
{
    return CProceduralMaterial::GetFromGraphInstance(m_Graph);
}

const char* CGraphInstance::GetName() const
{
    return m_Graph->mDesc.mLabel.c_str();
}

GraphInstanceID CGraphInstance::GetGraphInstanceID() const
{
    CProceduralMaterial* pMaterial = CProceduralMaterial::GetFromGraphInstance(m_Graph);

    for (int i = 0; i < pMaterial->GetGraphInstanceCount(); i++)
    {
        SubstanceAir::GraphInstance* pGraphInstance = pMaterial->GetSubstanceGraphInstance(i);

        if (pGraphInstance == m_Graph)
        {
            return pSubstanceSystem->EncodeGraphInstanceID(pMaterial, i);
        }
    }

    return INVALID_GRAPHINSTANCEID;
}

int CGraphInstance::GetInputCount() const
{
    return (int)m_Inputs.size();
}

IGraphInput* CGraphInstance::GetInput(int index)
{
    return &(m_Inputs[index]);
}

IGraphInput* CGraphInstance::GetInputByName(const char* name)
{
    for (auto iter = m_Inputs.begin(); iter != m_Inputs.end(); iter++)
    {
        IGraphInput& theGraph = *iter;

        if (!strcmp(name, theGraph.GetName()))
        {
            return &theGraph;
        }
    }

    return nullptr;
}

IGraphInput* CGraphInstance::GetInputByID(GraphInputID inputID)
{
    for (int i = 0; i < GetInputCount(); i++)
    {
        IGraphInput* pInput = GetInput(i);

        if (pInput->GetGraphInputID() == inputID)
        {
            return pInput;
        }
    }

    return nullptr;
}

int CGraphInstance::GetOutputCount() const
{
    return (int)m_Outputs.size();
}

IGraphOutput* CGraphInstance::GetOutput(int index)
{
    return &(m_Outputs[index]);
}

IGraphOutput* CGraphInstance::GetOutputByID(GraphOutputID outputID)
{
    for (int i = 0; i < GetOutputCount(); i++)
    {
        IGraphOutput* pOutput = GetOutput(i);

        if (pOutput->GetGraphOutputID() == outputID)
        {
            return pOutput;
        }
    }

    return nullptr;
}

#endif // AZ_USE_SUBSTANCE
