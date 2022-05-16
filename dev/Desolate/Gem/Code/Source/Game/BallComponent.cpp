#include "BallComponent.h"
#include "AzCore/Serialization/EditContext.h"

namespace Desolate
{

	class BehaviorBallComponentNotificationBusHandler
		: public BallComponentNotificationBus::Handler,
		  public AZ::BehaviorEBusHandler
	{
	public:
		AZ_EBUS_BEHAVIOR_BINDER(
			BehaviorBallComponentNotificationBusHandler, 
			"", 
			AZ::SystemAllocator,
			OnHit);
#
		void OnHit() override
		{
			Call(FN_OnHit);
		}
	};
	

	void BallComponent::Activate()
	{
		Physics::CollisionNotificationBus::Handler::BusConnect(GetEntityId());
	}

	void BallComponent::Deactivate()
	{
		Physics::CollisionNotificationBus::Handler::BusDisconnect();
	}

	void BallComponent::Reflect(AZ::ReflectContext* context)
	{
		if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
		{
			serializeContext->Class<BallComponent, AZ::Component>()
			                ->Version(1);

			// TODO(mh): Needed? LookAtComponent doesn't have it, yet still appears in the Add Component menu.
			if (auto editContext = serializeContext->GetEditContext())
			{
				editContext->Class<BallComponent>("Ball Component", "Ball Component")
					->ClassElement(AZ::Edit::ClassElements::EditorData, "")
					->Attribute(AZ::Edit::Attributes::Category, "Desolate")
					->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"));
			}
		}

		if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
		{
			behaviorContext->EBus<BallComponentNotificationBus>("BallNotification", "BallComponentNotificationBus", "BallNotification")
				->Attribute(AZ::Script::Attributes::Category, "Gameplay")
				->Handler<BehaviorBallComponentNotificationBusHandler>();
		}
	}

	void BallComponent::OnCollisionBegin(const Physics::CollisionEvent& collision_event)
	{
		BallComponentNotificationBus::Broadcast(&BallComponentNotifications::OnHit);
	}
}
