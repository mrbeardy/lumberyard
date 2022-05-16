
#include "BallTrackerComponent.h"

#include "AzCore/Component/TransformBus.h"
#include "AzCore/Serialization/EditContext.h"
#include "Code/Source/Scripting/LookAtComponent.h"

namespace Desolate
{
    void BallTrackerComponent::Activate()
    {
        m_BallHits = 0;

        BallComponentNotificationBus::Handler::BusConnect(m_BallEntityId);
    }

    void BallTrackerComponent::Deactivate()
    {
        DisconnectBallComponent();
    }

    void BallTrackerComponent::DisconnectBallComponent()
    {
        BallComponentNotificationBus::Handler::BusDisconnect();
    }

    void BallTrackerComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<BallTrackerComponent, AZ::Component>()
                ->Version(1)
                ->Field("BallEntityId", &BallTrackerComponent::m_BallEntityId);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if(editContext)
            {
                editContext->Class<BallTrackerComponent>("Ball Tracker Component", "Ball Tracker Component")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Desolate")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->DataElement(AZ::Edit::UIHandlers::EntityId, &BallTrackerComponent::m_BallEntityId, "Ball", "Ball");
            }
        }
    }
    
    void BallTrackerComponent::OnHit()
    {
        if (!m_BallEntityId.IsValid()) return;

        m_BallHits += 1;

        if (m_BallHits > 1)
        {
            // DisconnectBallComponent();

            AZ::Vector3 BallWorldTranslation;

            AZ::TransformBus::EventResult(
                BallWorldTranslation,
                m_BallEntityId,
                &AZ::TransformBus::Events::GetWorldTranslation
            );

    //        LmbrCentral::LookAtComponentRequestBus::Event(
                //GetEntityId(),
    //            &LmbrCentral::LookAtComponentRequestBus::Events::SetTarget,
    //            AZ::EntityId()
    //        );

            LmbrCentral::LookAtComponentRequestBus::Event(
                GetEntityId(),
                &LmbrCentral::LookAtComponentRequestBus::Events::SetTargetPosition,
                BallWorldTranslation
            );

            DisconnectBallComponent();
        } else
        {
            LmbrCentral::LookAtComponentRequestBus::Event(
                GetEntityId(),
                &LmbrCentral::LookAtComponentRequestBus::Events::SetTarget,
                m_BallEntityId
            );
        }
    }
}
