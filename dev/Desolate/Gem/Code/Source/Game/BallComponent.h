#pragma once
#include "AzCore/Component/Component.h"
#include "AzFramework/Physics/CollisionNotificationBus.h"
#include <AzFramework/Physics/WorldEventhandler.h>

namespace Desolate
{


	class BallComponentNotifications
		: public AZ::ComponentBus
	{
	public:
		virtual void OnHit() { }
	};

	using BallComponentNotificationBus = AZ::EBus<BallComponentNotifications>;

	class BallComponent
		: public AZ::Component
		, private Physics::CollisionNotificationBus::Handler
	{
public:
		AZ_COMPONENT(BallComponent, "{AB430E63-319D-4CB4-8950-04B4256539B5}")
		
		//=====================================================================
		// AZ::Component
		void Activate() override;
		void Deactivate() override;
		//=====================================================================

		//=====================================================================
		// Physics::CollisionNotificationBus
		void OnCollisionBegin(const Physics::CollisionEvent&) override;
		//=====================================================================

protected:
		static void Reflect(AZ::ReflectContext* context);
	};
}
