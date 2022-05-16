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
/** @file ProceduralMaterialEditorPlugin.cpp
	@brief Library hook to register plugin features
	@author Josh Coyne - Allegorithmic (josh.coyne@allegorithmic.com)
	@date 09-14-2015
	@copyright Allegorithmic. All rights reserved.
*/
#include "ProceduralMaterialEditorPlugin_precompiled.h"
#include "ProceduralMaterialEditorPlugin.h"
#include "QtViewPane.h"
#include "QProceduralMaterialEditorMainWindow.h"
#include "IResourceSelectorHost.h"
#include "QtViewPaneManager.h"

#include "CryExtension/ICryFactoryRegistry.h"

ProceduralMaterialEditorPlugin::ProceduralMaterialEditorPlugin(IEditor* editor)
    : m_registered(false)
{
#if defined(AZ_USE_SUBSTANCE)

    QtViewOptions options;
    options.canHaveMultipleInstances = true;
    options.sendViewPaneNameBackToAmazonAnalyticsServers = true;
    m_registered = RegisterQtViewPane<QProceduralMaterialEditorMainWindow>(editor, LyViewPane::SubstanceEditor, LyViewPane::CategoryPlugIns, options);
    if (m_registered)
    {
        RegisterModuleResourceSelectors(GetIEditor()->GetResourceSelectorHost());
    }
#endif
}

void ProceduralMaterialEditorPlugin::Release()
{
#if defined(AZ_USE_SUBSTANCE)
    if (m_registered)
    {
        UnregisterQtViewPane<QProceduralMaterialEditorMainWindow>();
    }
    delete this;
#endif
}
