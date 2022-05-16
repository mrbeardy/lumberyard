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

#include "Oculus_precompiled.h"
#include <IGem.h>
#include <platform_impl.h>

#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>
#include <Oculus_Traits_Platform.h>

#if AZ_TRAIT_OCULUS_OCULUS_SUPPORTED
#include "OculusDevice.h"
#endif // Windows x64

namespace Oculus
{
    class OculusGem
        : public CryHooksModule
    {
    public:
        AZ_RTTI(OculusGem, "{4BEB17B4-A97D-40EE-B5C6-A296436B753C}", AZ::Module);

        OculusGem()
            : CryHooksModule()
        {
#if AZ_TRAIT_OCULUS_OCULUS_SUPPORTED
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                OculusDevice::CreateDescriptor(),
            });

#endif // Windows x64




            // This is an internal Amazon gem, so register it's components for metrics tracking, otherwise the name of the component won't get sent back.
            // IF YOU ARE A THIRDPARTY WRITING A GEM, DO NOT REGISTER YOUR COMPONENTS WITH EditorMetricsComponentRegistrationBus
            AZStd::vector<AZ::Uuid> typeIds;
            typeIds.reserve(m_descriptors.size());
            for (AZ::ComponentDescriptor* descriptor : m_descriptors)
            {
                typeIds.emplace_back(descriptor->GetUuid());
            }
            EBUS_EVENT(AzFramework::MetricsPlainTextNameRegistrationBus, RegisterForNameSending, typeIds);
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
#if AZ_TRAIT_OCULUS_OCULUS_SUPPORTED
                azrtti_typeid<OculusDevice>(),
#endif // Windows x64
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Oculus_c32178b3c4e94fbead069bd92ff9b04a, Oculus::OculusGem)
