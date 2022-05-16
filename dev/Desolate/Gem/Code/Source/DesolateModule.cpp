
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <DesolateSystemComponent.h>

#include "Game/BallComponent.h"
#include "Game/BallTrackerComponent.h"

namespace Desolate
{
    class DesolateModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(DesolateModule, "{399362DD-D19A-4AC0-846F-A4063C4B49FF}", AZ::Module);
        AZ_CLASS_ALLOCATOR(DesolateModule, AZ::SystemAllocator, 0);

        DesolateModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                DesolateSystemComponent::CreateDescriptor(),
            });
            m_descriptors.insert(m_descriptors.end(), {
                BallTrackerComponent::CreateDescriptor(),
            });

            m_descriptors.insert(m_descriptors.end(), {
                BallComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<DesolateSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Desolate_eee2b77b0a814ad0ba92f666b02c5a9a, Desolate::DesolateModule)
