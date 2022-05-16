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
#include "SubstanceSystem.h"
#include "ProceduralMaterial.h"
#include "ProceduralTexture.h"
#include "Substance/ISubstanceAPI.h"


//------------------------------------------------------------------------------------------
static bool SaveGraphInstances(const SubstanceAir::GraphInstances& graphs, const char* smtlPath, const char* sbsarPath)
{
    SubstanceAir::stringstream smtlStream;
    smtlStream << "<ProceduralMaterial Source=\"";
    smtlStream << sbsarPath;
    smtlStream << "\">\n";

    SubstanceAir::map<SubstanceAir::OutputInstance*, string>    outputInstancePTexMap;

    //write output enabled id's and the sub files that map to them
    for (auto& pGraph : graphs)
    {
        for (auto& pOutput : pGraph->getOutputs())
        {
            string subPath = pOutput->mEnabled ? CGraphOutput::GetPath(pOutput, (int)graphs.size(), smtlPath) : "";
            const SubstanceAir::OutputFormat& outFormat = pOutput->getFormatOverride();
            bool bCompressed = true;
            SubstanceAir::map<size_t, SubstanceAir::OutputFormat::Component> formatComponents;

            if (pOutput->mEnabled)
            {
                outputInstancePTexMap[pOutput] = subPath;
            }

            if (CProceduralTexture* pTexture = CProceduralTexture::GetFromOutputInstance(pOutput))
            {
                bCompressed = pTexture->IsCompressed();
            }

            //see if we need to serialize format overrides
            for (size_t i = 0; i < SubstanceAir::OutputFormat::ComponentsCount; i++)
            {
                SubstanceAir::OutputFormat::Component component = outFormat.perComponent[i];

                if (!component.isIdentity(i))
                {
                    formatComponents.emplace(i, component);
                }
            }

            smtlStream << "\t<Output ID=\"";
            smtlStream << pOutput->mDesc.mUid;
            smtlStream << "\" Enabled=\"";
            smtlStream << pOutput->mEnabled;
            smtlStream << "\" Compressed=\"";
            smtlStream << bCompressed;
            smtlStream << "\" File=\"";
            smtlStream << subPath;

            //Write out format overrides if available
            if (formatComponents.size())
            {
                smtlStream << "\">\n";

                for (auto& kv : formatComponents)
                {
                    smtlStream << "\t\t<ComponentOverride ";
                    smtlStream << "Index=\"" << kv.first << "\" ";
                    smtlStream << "ShuffleIndex=\"" << kv.second.shuffleIndex << "\" ";

                    if (kv.second.outputUid != SubstanceAir::OutputFormat::UseDefault)
                    {
                        smtlStream << "OutputID=\"" << kv.second.outputUid << "\" ";
                    }

                    smtlStream << " />\n";
                }

                smtlStream << "\t</Output>\n";
            }
            else
            {
                smtlStream << "\" />\n";
            }
        }
    }

    //write parameters
    for (auto giter = graphs.begin(); giter != graphs.end(); giter++)
    {
        SubstanceAir::GraphInstance* pGraph = (*giter).get();
        for (auto iiter = pGraph->getInputs().begin(); iiter != pGraph->getInputs().end(); iiter++)
        {
            SubstanceAir::InputInstanceBase* pInput = *iiter;
            if (pInput->isNonDefault())
            {
                smtlStream << "\t<Parameter Name=\"";
                smtlStream << pInput->mDesc.mIdentifier;
                smtlStream << "\" GraphIndex=\"";
                smtlStream << std::distance(graphs.begin(), giter);
                smtlStream << "\" Value=\"";

                switch (pInput->mDesc.mType)
                {
                case Substance_IType_Integer:
                    smtlStream << static_cast<SubstanceAir::InputInstanceInt*>(pInput)->getValue();
                    break;
                case Substance_IType_Integer2:
                    smtlStream << static_cast<SubstanceAir::InputInstanceInt2*>(pInput)->getValue();
                    break;
                case Substance_IType_Integer3:
                    smtlStream << static_cast<SubstanceAir::InputInstanceInt3*>(pInput)->getValue();
                    break;
                case Substance_IType_Integer4:
                    smtlStream << static_cast<SubstanceAir::InputInstanceInt4*>(pInput)->getValue();
                    break;
                case Substance_IType_Float:
                    smtlStream << static_cast<SubstanceAir::InputInstanceFloat*>(pInput)->getValue();
                    break;
                case Substance_IType_Float2:
                    smtlStream << static_cast<SubstanceAir::InputInstanceFloat2*>(pInput)->getValue();
                    break;
                case Substance_IType_Float3:
                    smtlStream << static_cast<SubstanceAir::InputInstanceFloat3*>(pInput)->getValue();
                    break;
                case Substance_IType_Float4:
                    smtlStream << static_cast<SubstanceAir::InputInstanceFloat4*>(pInput)->getValue();
                    break;
                case Substance_IType_Image:
                {
                    if (CProceduralMaterial* pMaterial = CProceduralMaterial::GetFromGraphInstance(pGraph))
                    {
                        smtlStream << pMaterial->GetInputImagePath(static_cast<SubstanceAir::InputInstanceImage*>(pInput));
                    }
                }
                break;
                case Substance_IType_String:
                    smtlStream << static_cast<SubstanceAir::InputInstanceString*>(pInput)->getString().c_str();
                    break;
                default:
                    break;
                }

                smtlStream << "\" />\n";
            }
        }
    }

    smtlStream << "</ProceduralMaterial>";

    SubstanceAir::string smtlString = smtlStream.str();
    gAPI->WriteFile(smtlPath, smtlString.c_str(), smtlString.size());

    //write sub files
    for (auto& kv : outputInstancePTexMap)
    {
        SubstanceAir::OutputInstance* pOutput = kv.first;

        SubstanceAir::stringstream subStream;
        subStream << "<ProceduralTexture Material=\"";
        subStream << smtlPath;
        subStream << "\" OutputID=\"";
        subStream << pOutput->mDesc.mUid;
        subStream << "\" />";

        SubstanceAir::string subStreamString = subStream.str();
        gAPI->WriteFile(kv.second.c_str(), subStreamString.c_str(), subStreamString.size());
    }

    return true;
}

//------------------------------------------------------------------------------------------
class CSubstanceLibAPI
    : public ISubstanceLibAPI
{
public:
    virtual size_t CalcTextureSize(int width, int height, int mips, SubstanceTexFormat format) override
    {
        return CProceduralTexture::CalcTextureSize(width, height, mips, CProceduralTexture::ToSubstanceFormat(format));
    }

    virtual bool CreateProceduralMaterial(const char* sbsarPath, const char* smtlPath) override
    {
        IFileContents* pFileContents = nullptr;

        if (!gAPI->ReadFile(sbsarPath, &pFileContents))
        {
            return false;
        }

        SubstanceAir::unique_ptr<SubstanceAir::PackageDesc> package(AIR_NEW(SubstanceAir::PackageDesc)(pFileContents->GetData(), pFileContents->GetDataSize()));
        pFileContents->Release();

        if (package->isValid() == false)
        {
            return false;
        }

        SubstanceAir::GraphInstances graphs;
        SubstanceAir::instantiate(graphs, *package);

        //walk outputs and determine what sub's to make by default
        for (auto& pGraph : graphs)
        {
            SubstanceAir::OutputInstance* pDiffuse = nullptr;
            SubstanceAir::OutputInstance* pBaseColor = nullptr;

            for (auto& pOutput : pGraph->getOutputs())
            {
                switch (pOutput->mDesc.mChannel)
                {
                case SubstanceAir::Channel_Diffuse:
                    pDiffuse = pOutput;
                    break;
                case SubstanceAir::Channel_BaseColor:
                    pBaseColor = pOutput;
                    break;
                case SubstanceAir::Channel_Height:
                case SubstanceAir::Channel_Specular:
                case SubstanceAir::Channel_Normal:
                case SubstanceAir::Channel_Glossiness:
                    //keep these channels enabled
                    break;
                default:
                    //disable all other outputs for performance
                    pOutput->mEnabled = false;
                    break;
                }

                //if the substance has both diffuse and basecolor outputs, prefer diffuse
                if (pDiffuse && pBaseColor)
                {
                    pBaseColor->mEnabled = false;
                }
            }
        }

        return SaveGraphInstances(graphs, smtlPath, sbsarPath);
    }

    virtual GraphInstanceID EncodeGraphInstanceID(IProceduralMaterial* pMaterial, int graphIndex) override
    {
        return pSubstanceSystem->EncodeGraphInstanceID(static_cast<CProceduralMaterial*>(pMaterial), graphIndex);
    }

    virtual IGraphInstance* GetGraphInstance(GraphInstanceID graphInstanceID) const override
    {
        return pSubstanceSystem->GetGraphInstance(graphInstanceID);
    }

    virtual IProceduralMaterial* GetMaterialFromPath(const char* path, bool bForceLoad) override
    {
        if (bForceLoad)
        {
            return CProceduralMaterial::CreateFromPath(path);
        }
        else
        {
            return pSubstanceSystem->GetMaterialFromPath(path);
        }
    }

    virtual int GetMinimumOutputSize() const override
    {
        return 16;
    }

    virtual int GetMaximumOutputSize() const override
    {
#if defined(SUBSTANCE_ENGINE_SSE2)
        return 2048;
#else
        return 4096;
#endif
    }

    virtual bool HasRenderCompleted(ProceduralMaterialRenderUID uid) const override
    {
        return pSubstanceSystem->HasRenderCompleted(uid);
    }

    virtual bool LoadTextureData(const char* path, SSubstanceLoadData& loadData) override
    {
        return pSubstanceSystem->LoadTextureData(path, loadData);
    }

    virtual void OnExitSimulation() override
    {
        pSubstanceSystem->OnExitSimulation();
    }

    virtual void OnEnterSimulation() override
    {
        pSubstanceSystem->OnEnterSimulation();
    }

    virtual void OnRuntimeBudgetChanged(bool bApplyChangesImmediately) override
    {
        if (pSubstanceSystem)
        {
            pSubstanceSystem->OnRuntimeBudgetChanged(bApplyChangesImmediately);
        }
    }

    virtual bool QueueRender(IGraphInstance* pGraph) override
    {
        return pSubstanceSystem->QueueRender(static_cast<CGraphInstance*>(pGraph)->GetSubstanceGraph());
    }

    virtual void RemoveProceduralMaterial(IProceduralMaterial* pMaterial) override
    {
        std::vector<std::string> filesToDelete;

        //add smtl and sbsar to the list
        filesToDelete.push_back(pMaterial->GetPath());
        filesToDelete.push_back(pMaterial->GetSourcePath());

        //go through outputs
        for (int g = 0; g < pMaterial->GetGraphInstanceCount(); g++)
        {
            IGraphInstance* pGraph = pMaterial->GetGraphInstance(g);

            //add sub files
            for (int o = 0; o < pGraph->GetOutputCount(); o++)
            {
                filesToDelete.push_back(pGraph->GetOutput(o)->GetPath());
            }
        }

        //delete the substance files
        for (auto theString : filesToDelete)
        {
            gAPI->RemoveFile(theString.c_str());
        }

        pSubstanceSystem->RemoveMaterial(pMaterial);
    }

    virtual ProceduralMaterialRenderUID RenderASync(bool force) override
    {
        return pSubstanceSystem->RenderASync(force);
    }

    virtual void RenderSync() override
    {
        pSubstanceSystem->RenderSync();
    }

    virtual bool SaveProceduralMaterial(IProceduralMaterial* pMaterial, const char* path) override
    {
        CProceduralMaterial* pRealMaterial = static_cast<CProceduralMaterial*>(pMaterial);

        if (!path)
        {
            path = pRealMaterial->GetPath();
        }

        return SaveGraphInstances(pRealMaterial->GetGraphInstances(), path, pRealMaterial->GetSourcePath());
    }

    virtual void Update() override
    {
        pSubstanceSystem->Update();
    }

    virtual void Release() override
    {
        pSubstanceSystem->Shutdown();
        delete pSubstanceSystem;
        pSubstanceSystem = nullptr;
    }
};

//------------------------------------------------------------------------------------------
CSubstanceSystem* pSubstanceSystem = nullptr;
ISubstanceAPI* gAPI = nullptr;
ISubstanceLibAPI* gLibAPI = nullptr;

CSubstanceLibAPI gLibAPIStatic;

//------------------------------------------------------------------------------------------
void GetSubstanceAPI(ISubstanceAPI* pApi, ISubstanceLibAPI** pLibAPI)
{
    gAPI = pApi;

    if (pSubstanceSystem == nullptr)
    {
        pSubstanceSystem = new CSubstanceSystem();
        pSubstanceSystem->Init();
    }

    gLibAPI = &gLibAPIStatic;
    *pLibAPI = gLibAPI;
}

#endif // AZ_USE_SUBSTANCE
