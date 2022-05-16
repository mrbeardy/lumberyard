#pragma once

#include "AzCore/Component/Component.h"

#include "InputEventBus.h"
#include "AzCore/Component/TickBus.h"
#include "AzFramework/Input/Events/InputChannelEventListener.h"

namespace MetalInMotion
{
	

	// -----------------------------------------------------------------------------

	class PlayerBallBearing
		: public AZ::Component
		, public AZ::TickBus::Handler
	{

		const char* INPUT_MOVELONG = "MoveLong";
		const char* INPUT_MOVELAT = "MoveLat";

		// TODO(mh): Refactor this away.
		class InputEventNotificationBusListener
			: protected AZ::InputEventNotificationBus::Handler
			, protected AzFramework::InputChannelEventListener
		{
		public:
			explicit InputEventNotificationBusListener(const char* actionName, PlayerBallBearing* playerBallBearing)
				: m_actionName(actionName), m_playerBallBearing(playerBallBearing) {}

			void OnActivate();
			void OnDeactivate();

			//=========================================================================
			// AZ::InputEventNotificationBus
			void OnPressed(float value) override;
			void OnReleased(float value) override;
			//=========================================================================

		protected:
			bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;


		private:

			const char* m_actionName;
			PlayerBallBearing* m_playerBallBearing;
		};

		// -----------------------------------------------------------------------------
		
		
	public:
		AZ_COMPONENT(PlayerBallBearing, "{8EE48127-4601-4E5B-9383-B46703297353}")

		//=========================================================================
		// AZ::Component
		void Activate() override;
		void Deactivate() override;
		//=========================================================================


		//=========================================================================
		// AZ::TickBus
		void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
		//=========================================================================


		// TODO(mh): Replace with interface
		void InputListener_OnPressed(const char* actionName, float value);
		void InputListener_OnReleased(const char* actionName, float value);

		float ControllerForce = 2.f;

	protected:
		static void Reflect(AZ::ReflectContext* reflection);


	private:

		std::vector<InputEventNotificationBusListener*> m_inputEventNotificationListeners;

		float m_inputLat, m_inputLong;


		// TODO(mh): Replace with interface
		friend InputEventNotificationBusListener;
	};

}
