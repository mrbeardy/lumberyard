#pragma once

#include <AzCore/Component/Component.h>

#include "BallComponent.h"

namespace Desolate
{
    class BallTrackerComponent 
        : public AZ::Component
		, private BallComponentNotificationBus::Handler
    {
public:
        AZ_COMPONENT(BallTrackerComponent, "{E638C5F2-BDEB-4562-91C3-DF8FFDCBA892}")

        //=====================================================================
		// AZ::Component
        void Activate() override;
        void DisconnectBallComponent();
        void Deactivate() override;
        //=====================================================================

        //=====================================================================
        // BallComponent
        void OnHit() override;
        //=====================================================================

protected:
    	static void Reflect(AZ::ReflectContext* context);

        AZ::EntityId m_BallEntityId;

        int m_BallHits = 0;
    };
}
