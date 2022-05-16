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
/** @file SubstanceBus.h
    @brief Header for the Substance Gem.
    @author Josh Coyne - Allegorithmic (josh.coyne@allegorithmic.com)
    @date 09-14-2015
    @copyright Allegorithmic. All rights reserved.
*/
#ifndef GEM_46AED0DF_955D_4582_9583_0B4D2422A727_CODE_INCLUDE_SUBSTANCEBUS_H
#define GEM_46AED0DF_955D_4582_9583_0B4D2422A727_CODE_INCLUDE_SUBSTANCEBUS_H
#include <AzCore/EBus/EBus.h>

#include <Substance/Types.h>
#include <Substance/IProceduralMaterial.h>

#if defined(AZ_USE_SUBSTANCE)
struct ISubstanceLibAPI;
#endif // AZ_USE_SUBSTANCE


//----------------------------------------------------------------------------------------------------
class SubstanceRequests
    : public AZ::EBusTraits
{
public:
    ////////////////////////////////////////////////////////////////////////////
    // EBusTraits
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    ////////////////////////////////////////////////////////////////////////////

#if defined(AZ_USE_SUBSTANCE)
    /// Retrieve minimum output size of a texture.
    virtual int GetMinimumOutputSize() const = 0;

    /// Retrieve maximum output size of a texture.
    virtual int GetMaximumOutputSize() const = 0;

    /** Get a ProceduralMaterial object given a file path.
      * If bForceLoad is true, we will load the material from disk
      */
    virtual IProceduralMaterial* GetMaterialFromPath(const char* path, bool bForceLoad) const = 0;

    /// Retrieve a graph instance given a GraphInstanceID.
    virtual IGraphInstance* GetGraphInstance(GraphInstanceID graphInstanceID) const = 0;

    /// Queue a GraphInstance for rendering. Returns false if the IGraphInstance is already queued.
    virtual bool QueueRender(IGraphInstance* pGraphInstance) = 0;
    
    /// Triggers asynchronous rendering of queued Graphs.
    /// \param  force  If true, all queued Graphs will be sent to the renderer. This is better for accasional updates, where you need to guarantee that the job is submitted.
    ///                If false, each queued Graph will be sent to the renderer only if all prior renders of that Graph have finished. This is better for high frequency 
    ///                updates, to prevent backing up the internal render queue.
    /// \return        A handle so you can query for completion (or INVALID_PROCEDURALMATERIALRENDERUID if no Graphs were submitted for rendering)
    virtual ProceduralMaterialRenderUID RenderASync(bool force) = 0;

    /// Renders all queued graphs synchronously.
    virtual void RenderSync() = 0;

    /// Used to query if an async render command has completed.
    virtual bool HasRenderCompleted(ProceduralMaterialRenderUID uid) const = 0;

    /// Given a substance archive and destination path, create the appropriate smtl/sub files.
    virtual bool CreateProceduralMaterial(const char* sbsarPath, const char* smtlPath) = 0;

    /// Save an existing procedural material to disk, serializing all property changes.
    virtual bool SaveProceduralMaterial(IProceduralMaterial* pMaterial, const char* path = nullptr) = 0;

    /// Remove a procedural material from disk
    virtual void RemoveProceduralMaterial(IProceduralMaterial* pMaterial) = 0;

    // Retrieve the Substance Library API. Intended for internal usage.
    virtual ISubstanceLibAPI* GetSubstanceLibAPI() const = 0;

    // Set the Substance Library API. Intended for internal usage.
    virtual void SetSubstanceLibAPI(ISubstanceLibAPI*) = 0;
#endif // AZ_USE_SUBSTANCE
};
using SubstanceRequestBus = AZ::EBus<SubstanceRequests>;

//----------------------------------------------------------------------------------------------------

/*!
* Notificagtions from the Substance system
*/
class SubstanceNotifications
    : public AZ::EBusTraits
{
public:
    //! Sent when a render batch has finished. If the render was scheduled using RenderAsync(), the ProceduralMaterialRenderUID corresponds to the ID returned by RenderAsync()
    virtual void OnRenderFinished(ProceduralMaterialRenderUID) = 0;
};

using SubstanceNotificationBus = AZ::EBus<SubstanceNotifications>;

#endif// GEM_46AED0DF_955D_4582_9583_0B4D2422A727_CODE_INCLUDE_SUBSTANCEBUS_H
