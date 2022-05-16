#include "PlayerBallBearing.h"

#include "AzCore/Serialization/EditContext.h"
#include "AzFramework/Physics/RigidBodyBus.h"

namespace MetalInMotion
{
	void PlayerBallBearing::InputEventNotificationBusListener::OnActivate()
	{
		AZ::InputEventNotificationBus::Handler::BusConnect(AZ::InputEventNotificationId{ m_actionName });
	}

	void PlayerBallBearing::InputEventNotificationBusListener::OnDeactivate()
	{
		AZ::InputEventNotificationBus::Handler::BusDisconnect();
	}



	//=========================================================================
	// AZ::InputEventNotificationBus

	void PlayerBallBearing::InputEventNotificationBusListener::OnPressed(float value)
	{
		m_playerBallBearing->InputListener_OnPressed(m_actionName, value);
	}

	void PlayerBallBearing::InputEventNotificationBusListener::OnReleased(float value)
	{
		m_playerBallBearing->InputListener_OnReleased(m_actionName, value);
	}


	//=========================================================================

	bool PlayerBallBearing::InputEventNotificationBusListener::OnInputChannelEventFiltered(
		const AzFramework::InputChannel& inputChannel)
	{

		AZ_Printf("InputEventNotificationBusListener", "OnInputChannelEventFiltered");

		// Return false so we don't consume the event.
		return false;
	}






	// --------------------------------------------------------------------------------------------------------

	//=========================================================================
	// AZ::Component

	void PlayerBallBearing::Activate()
	{
		AZ_Printf("PlayerBallBearing::Activate", "")

		AZ::TickBus::Handler::BusConnect();

		// TODO(mh): Initialize somewhere else?
		m_inputEventNotificationListeners.emplace_back(new InputEventNotificationBusListener(INPUT_MOVELONG, this));
		m_inputEventNotificationListeners.emplace_back(new InputEventNotificationBusListener(INPUT_MOVELAT, this));

		for (const auto listener : m_inputEventNotificationListeners)
		{
			listener->OnActivate();
		}
	}

	void PlayerBallBearing::Deactivate()
	{
		AZ::TickBus::Handler::BusDisconnect();

		for (const auto listener : m_inputEventNotificationListeners)
		{
			listener->OnDeactivate();

			delete listener;
		}
	}

	//=========================================================================




	//=========================================================================
	// AZ::TickBus

	void PlayerBallBearing::OnTick(float deltaTime, AZ::ScriptTimePoint time)
	{
		float mass;

		
		Physics::RigidBodyRequestBus::EventResult(mass, GetEntityId(), &Physics::RigidBodyRequests::GetMass);

		const auto movementVector = AZ::Vector3 { m_inputLong, m_inputLat, 0.f };
		// auto linearImpulse = movementVector * ControllerForce * mass;
		auto force = movementVector * ControllerForce * mass;

		if (m_inputLong > 0.1f || m_inputLat > 0.1f)
		    Physics::RigidBodyRequestBus::Event(GetEntityId(), &Physics::RigidBodyRequests::AddForce, force);
			//Physics::RigidBodyRequestBus::Event(GetEntityId(), &Physics::RigidBodyRequests::ApplyLinearImpulse, linearImpulse);

		//AZ_Printf("PlayerBallBearing", "Tick(Long: %f, Lat: %f, ControllerForce: %f, Mass: %f)", m_inputLong, m_inputLat, ControllerForce, mass);
		//AZ_Printf("PlayerBallBearing", "LinearImpulse(%f, %f, %f)", linearImpulse.GetX(), linearImpulse.GetY(), linearImpulse.GetZ())
	}

	//=========================================================================



	void PlayerBallBearing::Reflect(AZ::ReflectContext* context)
	{
		auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

		if (serializeContext)
		{
			serializeContext
				->Class<PlayerBallBearing, AZ::Component>()
				->Field("ControllerForce", &PlayerBallBearing::ControllerForce)
				->Version(1);


			if (auto* editContext = serializeContext->GetEditContext())
			{
				editContext
					->Class<PlayerBallBearing>("Player Ball Bearing", "Player Ball Bearing")
					->ClassElement(AZ::Edit::ClassElements::EditorData, "")
						->Attribute(AZ::Edit::Attributes::Category, "Ball Games")
						->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
					->DataElement(
						AZ::Edit::UIHandlers::Default, 
						&PlayerBallBearing::ControllerForce,
						"Controller Force",
						"Controller Force"
					);
			}
		}
	}



	//=========================================================================
	// Called by InputEventNotificationBus_Listener

	void PlayerBallBearing::InputListener_OnPressed(const char* actionName, float value)
	{
		//AZ_Printf("PlayerBallBearing", "OnPressed: %s", actionName)

		if (actionName == INPUT_MOVELONG)
		{
			m_inputLong = value;
		}

		if (actionName == INPUT_MOVELAT)
		{
			m_inputLat = value;
		}
	}

	void PlayerBallBearing::InputListener_OnReleased(const char* actionName, float value)
	{
		//AZ_Printf("PlayerBallBearing", "OnReleased: %s", actionName)

		if (actionName == INPUT_MOVELONG)
		{
			m_inputLong = 0.f;
		}

		if (actionName == INPUT_MOVELAT)
		{
			m_inputLat = 0.f;
		}
	}

	//=========================================================================
}
