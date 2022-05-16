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

#ifndef CRYINCLUDE_LYSUBSTANCE_PROCEDURALMATERIAL_H
#define CRYINCLUDE_LYSUBSTANCE_PROCEDURALMATERIAL_H
#pragma once

#if defined(AZ_USE_SUBSTANCE)
#include <Substance/IProceduralMaterial.h>

class CGraphInstance;
class CProceduralTexture;

/**/
class CProceduralMaterial
    : public IProceduralMaterial
{
    friend class CSubstanceSystem;
public:
    CProceduralMaterial(const char* path, const char* sourcePath, SubstanceAir::unique_ptr<SubstanceAir::PackageDesc> package);
    virtual ~CProceduralMaterial();

    void BindImageInput(SubstanceAir::InputInstanceImage* pImage, const char* path);

    inline const SubstanceAir::GraphInstances& GetGraphInstances() const
    {
        return m_Graphs;
    }

    SubstanceAir::InputInstanceBase* GetInputByName(int graphIndex, const char* name) const;
    static SubstanceAir::InputInstanceBase* GetInputByName(SubstanceAir::GraphInstance* pGraphInstance, const char* name);

    inline ProceduralMaterialID GetID() const
    {
        return m_ID;
    }
    inline void SetID(ProceduralMaterialID id)
    {
        m_ID = id;
    }

    inline size_t GetNumTextures() const
    {
        return m_Textures.size();
    }
    inline CProceduralTexture* GetTextureByIndex(int index) const
    {
        return m_Textures[index];
    }
    CProceduralTexture* GetTextureByUID(SubstanceUID uid) const;

    const char* GetInputImagePath(SubstanceAir::InputInstanceImage* pImage) const;

    //add bound image inputs to map (if pGraphInstance is null, apply all inputs for this material)
    void AddImageInputs(SubstanceAir::GraphInstance* pGraphInstance, InputImageMap& mapOut) const;

    void Reset(int nResetID);

    IGraphInstance* GetIGraphInstance(SubstanceAir::GraphInstance* pGraph);
    SubstanceAir::GraphInstance* GetSubstanceGraphInstance(int index) const;

    static CProceduralMaterial* CreateFromPath(const char* path);
    static CProceduralMaterial* GetFromGraphInstance(SubstanceAir::GraphInstance* pGraphInstance);

    //IProceduralMaterial
    virtual const char* GetPath() const override
    {
        return m_Path.c_str();
    }
    virtual const char* GetSourcePath() const override
    {
        return m_SubstancePath.c_str();
    }

    virtual int GetGraphInstanceCount() const override;
    virtual IGraphInstance* GetGraphInstance(int index) override;

    virtual void ReimportSubstance() override;

private:
    void Init();
    void Dispose();

    static SubstanceAir::OutputFormat CreateOutputFormat(SubstanceAir::OutputInstance* output, const SubstanceAir::OutputFormat& baseFormat, bool bCompressed);

private:
    string                                                  m_Path;
    string                                                  m_SubstancePath;
    ProceduralMaterialID                                    m_ID;
    SubstanceAir::unique_ptr<SubstanceAir::PackageDesc>     m_Package;
    SubstanceAir::GraphInstances                            m_Graphs;
    std::vector<CGraphInstance>                             m_GraphHandles;
    int                                                     m_LastResetID;

    typedef std::vector<CProceduralTexture*>                TextureVector;
    TextureVector                                           m_Textures;

    InputImageMap                                           m_ImageInputs;
};

/**/
class CGraphInput
    : public IGraphInput
{
public:
    CGraphInput(SubstanceAir::InputInstanceBase* pInput);

    //IGraphInput
    virtual IGraphInstance* GetGraphInstance() const;
    virtual GraphInputID GetGraphInputID() const;

    virtual const char* GetDescription() const;
    virtual const char* GetLabel() const;
    virtual const char* GetName() const;
    virtual const char* GetGroupName() const;

    virtual GraphInputType GetInputType() const;
    virtual GraphInputWidgetType GetInputWidgetType() const;

    virtual GraphValueVariant GetValue() const;
    virtual void SetValue(const GraphValueVariant& value);

    virtual GraphValueVariant GetMinValue() const;
    virtual GraphValueVariant GetMaxValue() const;

    virtual int GetEnumCount() const;
    virtual GraphEnumValue GetEnumValue(int index);

private:
    SubstanceAir::InputInstanceBase*    m_Input;
};

/**/
class CGraphOutput
    : public IGraphOutput
{
public:
    CGraphOutput(SubstanceAir::OutputInstance* pOutput);

    static string GetPath(SubstanceAir::OutputInstance* pOutput, int numGraphs, const char* smtlPath);
    static CGraphOutput* GetFromOutputInstance(SubstanceAir::OutputInstance* pOutput);

    inline SubstanceAir::OutputInstance* GetOutputInstance() const
    {
        return m_Output;
    }

    //IGraphOutput
    virtual IGraphInstance* GetGraphInstance() const override;

    virtual GraphOutputID GetGraphOutputID() const override;

    virtual const char* GetLabel() const override;
    virtual const char* GetPath() const override;

    virtual bool IsEnabled() const override;
    virtual void SetEnabled(bool bEnabled) override;

    virtual bool IsDirty() const override;
    virtual void SetDirty() override;

    virtual SubstanceAir::RenderResult* GetEditorPreview() const override;

    virtual SubstanceAir::ChannelUse GetChannel() const override;

private:
    SubstanceAir::OutputInstance* m_Output;
    mutable string m_Path;  //mutable because it's just a memory container for GetPath
};

/**/
class CGraphInstance
    : public IGraphInstance
{
public:
    CGraphInstance(SubstanceAir::GraphInstance* pGraph);

    inline SubstanceAir::GraphInstance* GetSubstanceGraph() const
    {
        return m_Graph;
    }

    static CGraphInstance* GetFromGraphInstance(SubstanceAir::GraphInstance* pGraph);

    //IGraphInstance
    virtual IProceduralMaterial* GetProceduralMaterial() const;

    virtual const char* GetName() const;
    virtual GraphInstanceID GetGraphInstanceID() const;

    virtual int GetInputCount() const;
    virtual IGraphInput* GetInput(int index);
    virtual IGraphInput* GetInputByName(const char* name);
    virtual IGraphInput* GetInputByID(GraphInputID inputID);

    virtual int GetOutputCount() const;
    virtual IGraphOutput* GetOutput(int index);
    virtual IGraphOutput* GetOutputByID(GraphOutputID outputID);

private:
    SubstanceAir::GraphInstance*        m_Graph;
    std::vector<CGraphInput>            m_Inputs;
    std::vector<CGraphOutput>           m_Outputs;
};

#endif // AZ_USE_SUBSTANCE
#endif //CRYINCLUDE_LYSUBSTANCE_PROCEDURALMATERIAL_H
