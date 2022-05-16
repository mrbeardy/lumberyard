/** @file ISubstanceAPI.h
    @brief Header for Substance API Interface
    @author Josh Coyne - Allegorithmic (josh.coyne@allegorithmic.com)
    @date 09-14-2015
    @copyright Allegorithmic. All rights reserved.
*/
#ifndef _API_SUBSTANCE_INTERFACE_H_
#define _API_SUBSTANCE_INTERFACE_H_

#if defined(AZ_USE_SUBSTANCE)
struct IDeviceTexture;

//----------------------------------------------------------------------------------------------------
typedef enum _SubstanceTexFormat
{
    SubstanceTex_BC1,
    SubstanceTex_BC2,
    SubstanceTex_BC3,
    SubstanceTex_BC4,
    SubstanceTex_BC5,
    SubstanceTex_L8,
    SubstanceTex_PVRTC2,
    SubstanceTex_PVRTC4,
    SubstanceTex_R8G8B8A8,
    SubstanceTex_R16G16B16A16,

    SubstanceTex_Unknown,
} SubstanceTexFormat;

//----------------------------------------------------------------------------------------------------
struct SSubstanceLoadData
{
    byte*                   m_pData;
    size_t                  m_DataSize;
    int                     m_Width;
    int                     m_Height;
    SubstanceTexFormat      m_Format;
    int                     m_NumMips;
    bool                    m_bNormalMap;
    IDeviceTexture*         m_pTexture;
    void*                   (*m_dataAllocFunc)(size_t);

    unsigned int            m_nFlags;
};

//----------------------------------------------------------------------------------------------------
struct ISubstanceParameterXML
{
    virtual ~ISubstanceParameterXML() {}

    virtual const char* GetName() const = 0;
    virtual const char* GetValue() const = 0;
    virtual int GetGraphIndex() const = 0;
};

struct SSubstanceOutputInfoXML
{
    SubstanceAir::OutputFormat OverrideFormat;
    bool bEnabled;
    bool bCompressed;
};

struct ISubstanceMaterialXMLData
{
    virtual ~ISubstanceMaterialXMLData() {}

    virtual void Release() = 0;
    virtual const char* GetSource() const = 0;
    virtual const char* GetSourceData() const = 0;
    virtual size_t GetSourceDataSize() const = 0;

    virtual const ISubstanceParameterXML* GetParameter(int index) const = 0;
    virtual int GetParameterCount() const = 0;

    virtual const SSubstanceOutputInfoXML* GetOutputInfo(unsigned int uid) const = 0;
};

//----------------------------------------------------------------------------------------------------
struct IFileContents
{
    virtual ~IFileContents() {}

    virtual void Release() = 0;
    virtual void* GetData() = 0;
    virtual size_t GetDataSize() const = 0;
};

//----------------------------------------------------------------------------------------------------
struct ISubstanceAPI
{
    virtual ~ISubstanceAPI() {}

    virtual void AddRefDeviceTexture(IDeviceTexture* pTexture) = 0;
    virtual void* GetSubstanceLibrary() = 0;
    virtual int GetCoreCount() = 0;
    virtual int GetMemoryBudget() = 0;
    virtual bool IsEditor() const = 0;
    virtual ISubstanceMaterialXMLData* LoadMaterialXML(const char* path, bool bParametersOnly = false) = 0;
    virtual bool LoadTextureXML(const char* path, unsigned int& outputID, const char** material) = 0;
    virtual bool ReadFile(const char* path, IFileContents** pContents) = 0;
    virtual bool ReadTexture(const char* path, SSubstanceLoadData& loadData) = 0;
    virtual void ReleaseDeviceTexture(IDeviceTexture* pTexture) = 0;
    virtual void ReleaseSubstanceLoadData(SSubstanceLoadData& loadData) = 0;
    virtual void ReloadDeviceTexture(IDeviceTexture* pTexture) = 0;
    virtual bool WriteFile(const char* path, const char* data, size_t bytes) = 0;
    virtual bool RemoveFile(const char* path) = 0;
};

//----------------------------------------------------------------------------------------------------
struct ISubstanceLibAPI
{
    virtual ~ISubstanceLibAPI() {}

    virtual void Release() = 0;

    virtual size_t CalcTextureSize(int width, int height, int mips, SubstanceTexFormat format) = 0;
    virtual bool CreateProceduralMaterial(const char* sbsarPath, const char* smtlPath) = 0;
    virtual GraphInstanceID EncodeGraphInstanceID(IProceduralMaterial* pMaterial, int graphIndex) = 0;
    virtual IGraphInstance* GetGraphInstance(GraphInstanceID graphInstanceID) const = 0;
    virtual IProceduralMaterial* GetMaterialFromPath(const char* path, bool bForceLoad) = 0;
    virtual int GetMinimumOutputSize() const = 0;
    virtual int GetMaximumOutputSize() const = 0;
    virtual bool HasRenderCompleted(ProceduralMaterialRenderUID uid) const = 0;
    virtual bool LoadTextureData(const char* path, SSubstanceLoadData& loadData) = 0;
    virtual void OnExitSimulation() = 0;
    virtual void OnEnterSimulation() = 0;
    virtual void OnRuntimeBudgetChanged(bool bApplyChangesImmediately) = 0;
    virtual bool QueueRender(IGraphInstance* pGraph) = 0;
    virtual void RemoveProceduralMaterial(IProceduralMaterial* pMaterial) = 0;
    virtual ProceduralMaterialRenderUID RenderASync(bool force) = 0;
    virtual void RenderSync() = 0;
    virtual bool SaveProceduralMaterial(IProceduralMaterial* pMaterial, const char* path) = 0;
    virtual void Update() = 0;
};

void GetSubstanceAPI(ISubstanceAPI* pApi, ISubstanceLibAPI** pLibAPI);

#endif // AZ_USE_SUBSTANCE
#endif //_API_SUBSTANCE_INTERFACE_H_
