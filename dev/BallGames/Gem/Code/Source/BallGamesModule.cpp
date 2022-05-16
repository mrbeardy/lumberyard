
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <BallGamesSystemComponent.h>

#include "PlayerBallBearing.h"

namespace BallGames
{
    class BallGamesModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(BallGamesModule, "{23BA19CE-0016-4E66-92D0-B13548B3C896}", AZ::Module);
        AZ_CLASS_ALLOCATOR(BallGamesModule, AZ::SystemAllocator, 0);

        BallGamesModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                BallGamesSystemComponent::CreateDescriptor(),
            });

            m_descriptors.insert(m_descriptors.end(), {
                MetalInMotion::PlayerBallBearing::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<BallGamesSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(BallGames_45e28c3c2a2149cf870833557b8b71ed, BallGames::BallGamesModule)
