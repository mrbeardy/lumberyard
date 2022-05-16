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
#include "OculusTouchController.h"

#include <AzFramework/Input/Utils/AdjustAnalogInputForDeadZone.h>

#include <IInput.h> // Temp until deprecated

namespace Oculus
{
    using namespace AzFramework;

    // Input devices implemented in AzFramework all expose their InputDeviceId and associated
    // InputChannelIds as publically accessible static members. We could probably do the same
    // for VR controllers at some point, but once we have settled on a default common layout.
    //
    // Also, oculus controllers should perhaps derive from InputDeviceGamepad::Implementation,
    // because the oculus SDK actually processes input from XBox gamepads anyway. See comment
    // at the top of OculusTouchController::TickInputDevice for some details related to this.
    //
    // So for now at least it is likely best to keep the visibility of these ids local to this
    // .cpp file, to minimise the impact if we decide to change or consolidate them in future.

    // The id used to identify a pair of Oculus touch controller devices
    const InputDeviceId ControllerId("oculus_controllers");

    // All the input channel ids that identify oculus controller digital button input
    namespace Button
    {
        const InputChannelId A("oculus_button_a");   // The A button on the right controller
        const InputChannelId B("oculus_button_b");   // The B button on the right controller
        const InputChannelId X("oculus_button_x");   // The X button on the left controller
        const InputChannelId Y("oculus_button_y");   // The Y button on the left controller
        const InputChannelId L3("oculus_button_l3"); // The left thumb-stick click button
        const InputChannelId R3("oculus_button_r3"); // The right thumb-stick click button

        // All oculus controller digital button ids
        const AZStd::array<InputChannelId, 6> All =
        {{
            A,
            B,
            X,
            Y,
            L3,
            R3
        }};

        // Map of digital button ids keyed by their oculus vr button bitmask
        const AZStd::array<AZStd::pair<AZ::u32, const InputChannelId*>, 6> IdByBitMaskMap =
        {{
            { ovrButton_A,      &A },
            { ovrButton_B,      &B },
            { ovrButton_X,      &X },
            { ovrButton_Y,      &Y },
            { ovrButton_LThumb, &L3 },
            { ovrButton_RThumb, &R3 }
        }};
    }

    // All the input channel ids that identify oculus controller analog trigger input
    namespace Trigger
    {
        const InputChannelId L1("oculus_trigger_l1"); // The left controller index trigger
        const InputChannelId R1("oculus_trigger_r1"); // The right controller index trigger
        const InputChannelId L2("oculus_trigger_l2"); // The left controller hand trigger
        const InputChannelId R2("oculus_trigger_r2"); // The right controller hand trigger

        // All oculus controller analog trigger ids
        const AZStd::array<InputChannelId, 4> All =
        {{
            L1,
            R1,
            L2,
            R2
        }};
    }

    // All the input channel ids that identify oculus controller thumb-stick axis 1D input
    namespace ThumbStickAxis1D
    {
        const AzFramework::InputChannelId LX("oculus_thumbstick_l_x"); // X-axis of the left-hand thumb-stick
        const AzFramework::InputChannelId LY("oculus_thumbstick_l_y"); // Y-axis of the left-hand thumb-stick
        const AzFramework::InputChannelId RX("oculus_thumbstick_r_x"); // X-axis of the right-hand thumb-stick
        const AzFramework::InputChannelId RY("oculus_thumbstick_r_y"); // Y-axis of the right-hand thumb-stick

        // All oculus controller thumb-stick axis 1D ids
        const AZStd::array<AzFramework::InputChannelId, 4> All =
        {{
            LX,
            LY,
            RX,
            RY
        }};
    }

    // All the input channel ids that identify oculus controller thumb-stick axis 2D input
    namespace ThumbStickAxis2D
    {
        const AzFramework::InputChannelId L("oculus_thumbstick_l"); // The left-hand thumb-stick
        const AzFramework::InputChannelId R("oculus_thumbstick_r"); // The right-hand thumb-stick

        // All oculus controller thumb-stick axis 2D ids
        const AZStd::array<AzFramework::InputChannelId, 2> All =
        {{
            L,
            R
        }};
    }

    OculusTouchController::OculusTouchController(ovrSession& session)
        : InputDevice(ControllerId)
        , m_session(session)
        , m_connectionState(ovrControllerType_None)
        , m_allChannelsById()
        , m_buttonChannelsById()
        , m_triggerChannelsById()
        , m_thumbStickAxis1DChannelsById()
        , m_thumbStickAxis2DChannelsById()
    {
        memset(&m_trackingState, 0, sizeof(m_trackingState));

        // Create all digital button input channels
        for (const InputChannelId& channelId : Button::All)
        {
            InputChannelDigital* channel = aznew InputChannelDigital(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_buttonChannelsById[channelId] = channel;
        }

        // Create all analog trigger input channels
        for (const InputChannelId& channelId : Trigger::All)
        {
            InputChannelAnalog* channel = aznew InputChannelAnalog(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_triggerChannelsById[channelId] = channel;
        }

        // Create all thumb-stick axis 1D input channels
        for (const InputChannelId& channelId : ThumbStickAxis1D::All)
        {
            InputChannelAxis1D* channel = aznew InputChannelAxis1D(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_thumbStickAxis1DChannelsById[channelId] = channel;
        }

        // Create all thumb-stick axis 2D input channels
        for (const InputChannelId& channelId : ThumbStickAxis2D::All)
        {
            InputChannelAxis2D* channel = aznew InputChannelAxis2D(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_thumbStickAxis2DChannelsById[channelId] = channel;
        }

        CrySystemEventBus::Handler::BusConnect();
        InputHapticFeedbackRequestBus::Handler::BusConnect(GetInputDeviceId());
    }

    OculusTouchController::~OculusTouchController()
    {
        InputHapticFeedbackRequestBus::Handler::BusDisconnect(GetInputDeviceId());
        CrySystemEventBus::Handler::BusDisconnect();

        // Destroy all thumb-stick axis 2D input channels
        for (const auto& channelById : m_thumbStickAxis2DChannelsById)
        {
            delete channelById.second;
        }

        // Destroy all thumb-stick axis 1D input channels
        for (const auto& channelById : m_thumbStickAxis1DChannelsById)
        {
            delete channelById.second;
        }

        // Destroy all analog trigger input channels
        for (const auto& channelById : m_triggerChannelsById)
        {
            delete channelById.second;
        }

        // Destroy all digital button input channels
        for (const auto& channelById : m_buttonChannelsById)
        {
            delete channelById.second;
        }
    }

    const AzFramework::InputDevice::InputChannelByIdMap& OculusTouchController::GetInputChannelsById() const
    {
        return m_allChannelsById;
    }

    bool OculusTouchController::IsSupported() const
    {
        // If this class exists the Oculus gem is enabled, so oculus touch controllers are supported
        return true;
    }

    bool OculusTouchController::IsConnected() const
    {
        // Returns true if both the left and right controllers are connected, false otherwise
        return (m_connectionState & ovrControllerType_Touch) != 0;
    }

    float AdjustForDeadZoneAndNormalize(float value)
    {
        static const float DEAD_ZONE = 0.1f;
        static const float MAX_VALUE = 1.0f;
        return AdjustForDeadZoneAndNormalizeAnalogInput(value, DEAD_ZONE, MAX_VALUE);
    }

    void OculusTouchController::TickInputDevice()
    {
        if (!m_session)
        {
            return;
        }

        // According to the oculus SDK (https://developer3.oculus.com/doc/0.7.0.0-libovr/structovr_input_state.html):
        // "ovrInputState describes the complete controller input state, including Oculus Touch, and XBox gamepad.
        //  If multiple inputs are connected and used at the same time, their inputs are combined."
        //
        // This raises an interesting scenario, where the oculus SDK might be processing regular gamepad input in
        // addition to AzFramework::InputDeviceGamepadWin, which would result in the same 'raw' input event being
        // processed once by InputDeviceGamepadWin and once by OculusTouchController. So if a developer has setup
        // an input mapping that responds to events from both these devices, this will result in duplicate input.
        //
        // Fow now we are avoiding this situation by passing in ovrControllerType_Touch to ovr_GetInputState, but
        // this perhaps strengthens the case for implemeting oculus touch controllers as gamepad implementations.

        ovrInputState currentInputState;
        ovrResult result = ovr_GetInputState(m_session, ovrControllerType_Touch, &currentInputState);
        if (!OVR_SUCCESS(result))
        {
            ovrErrorInfo errorInfo;
            ovr_GetLastErrorInfo(&errorInfo);
            AZ_Warning("OculusTouchController", false,
                       "Unable to get input state from Oculus device! [%s]",
                       *errorInfo.ErrorString ? errorInfo.ErrorString : "unspecified error");

            // Instead of returning, clear the input state so all potentially active channels get reset below.
            // The meset may not be necessary, but the docs don't explicitly state what state the struct will
            // be left in if ovr_GetInputState fails, so better to be safe than sorry.
            memset(&currentInputState, 0, sizeof(currentInputState));
        }

        // Update the connection state and broadcast connection events if appropriate
        const bool wasConnected = IsConnected();
        m_connectionState = currentInputState.ControllerType;
        const bool isConnected = IsConnected();
        if (!wasConnected && isConnected)
        {
            // Both the left and right controllers are now connected
            BroadcastInputDeviceConnectedEvent();
        }
        else if (wasConnected && !isConnected)
        {
            // Either the the left or right controller just disconnected
            BroadcastInputDeviceDisconnectedEvent();
        }

        // Update digital button states
        for (const auto& buttonIdByBitMaskPair : Button::IdByBitMaskMap)
        {
            const AZ::u32 buttonState = currentInputState.Buttons & buttonIdByBitMaskPair.first;
            const InputChannelId& inputChannelId = *(buttonIdByBitMaskPair.second);
            m_buttonChannelsById[inputChannelId]->ProcessRawInputEvent(buttonState != 0);
        }

        // Update analog trigger states. The oculus SDK says it already applies a dead-zone,
        // but based on the values being returned this is clearly not the case.
        m_triggerChannelsById[Trigger::L1]->ProcessRawInputEvent(AdjustForDeadZoneAndNormalize(currentInputState.IndexTrigger[ovrHand_Left]));
        m_triggerChannelsById[Trigger::R1]->ProcessRawInputEvent(AdjustForDeadZoneAndNormalize(currentInputState.IndexTrigger[ovrHand_Right]));
        m_triggerChannelsById[Trigger::L2]->ProcessRawInputEvent(AdjustForDeadZoneAndNormalize(currentInputState.HandTrigger[ovrHand_Left]));
        m_triggerChannelsById[Trigger::R2]->ProcessRawInputEvent(AdjustForDeadZoneAndNormalize(currentInputState.HandTrigger[ovrHand_Right]));

        // Update the left thumb-stick states (the oculus SDK already applies a dead-zone)
        ovrVector2f valuesLeftThumb = currentInputState.Thumbstick[ovrHand_Left];
        m_thumbStickAxis2DChannelsById[ThumbStickAxis2D::L]->ProcessRawInputEvent(AZ::Vector2(valuesLeftThumb.x, valuesLeftThumb.y));
        m_thumbStickAxis1DChannelsById[ThumbStickAxis1D::LX]->ProcessRawInputEvent(valuesLeftThumb.x);
        m_thumbStickAxis1DChannelsById[ThumbStickAxis1D::LY]->ProcessRawInputEvent(valuesLeftThumb.y);

        // Update the right thumb-stick states (the oculus SDK already applies a dead-zone)
        ovrVector2f valuesRightThumb = currentInputState.Thumbstick[ovrHand_Right];
        m_thumbStickAxis2DChannelsById[ThumbStickAxis2D::R]->ProcessRawInputEvent(AZ::Vector2(valuesRightThumb.x, valuesRightThumb.y));
        m_thumbStickAxis1DChannelsById[ThumbStickAxis1D::RX]->ProcessRawInputEvent(valuesRightThumb.x);
        m_thumbStickAxis1DChannelsById[ThumbStickAxis1D::RY]->ProcessRawInputEvent(valuesRightThumb.y);
    }

    void OculusTouchController::SetVibration(float leftMotorSpeedNormalized, float rightMotorSpeedNormalized)
    {
        ovr_SetControllerVibration(m_session, ovrControllerType::ovrControllerType_LTouch, 0, leftMotorSpeedNormalized);
        ovr_SetControllerVibration(m_session, ovrControllerType::ovrControllerType_RTouch, 0, rightMotorSpeedNormalized);
    }

    AZ::VR::TrackingState* OculusTouchController::GetTrackingState(AZ::VR::ControllerIndex controllerIndex)
    {
        return &m_trackingState[static_cast<uint32_t>(controllerIndex)];
    }

    bool OculusTouchController::IsConnected(AZ::VR::ControllerIndex controllerIndex)
    {
        switch (controllerIndex)
        {
            case AZ::VR::ControllerIndex::LeftHand: return (m_connectionState & ovrControllerType_LTouch) != 0;
            case AZ::VR::ControllerIndex::RightHand: return (m_connectionState & ovrControllerType_RTouch) != 0;
            default: return false;
        }
        return false;
    }

    void OculusTouchController::OnCrySystemInitialized(ISystem& system, const SSystemInitParams&)
    {
        if (!system.GetIInput())
        {
            return;
        }

        // I think this is wrong, and we should be using a different EKeyId for each symbol,
        // but it matches the current implementation of the MapSymbol function below.
        static const EKeyId motionBaseKeyId = static_cast<EKeyId>(KI_MOTION_BASE);

        static const std::vector<SInputSymbol> azToCryInputSymbols =
        {
            SInputSymbol(Button::A.GetNameCrc32(), motionBaseKeyId, "OculusTouch_A", SInputSymbol::Button),
            SInputSymbol(Button::B.GetNameCrc32(), motionBaseKeyId, "OculusTouch_B", SInputSymbol::Button),
            SInputSymbol(Button::X.GetNameCrc32(), motionBaseKeyId, "OculusTouch_X", SInputSymbol::Button),
            SInputSymbol(Button::Y.GetNameCrc32(), motionBaseKeyId, "OculusTouch_Y", SInputSymbol::Button),
            SInputSymbol(Button::L3.GetNameCrc32(), motionBaseKeyId, "OculusTouch_LeftThumbstickButton", SInputSymbol::Button),
            SInputSymbol(Button::R3.GetNameCrc32(), motionBaseKeyId, "OculusTouch_RightThumbstickButton", SInputSymbol::Button),

            SInputSymbol(Trigger::L1.GetNameCrc32(), motionBaseKeyId, "OculusTouch_LeftTrigger", SInputSymbol::Trigger),
            SInputSymbol(Trigger::R1.GetNameCrc32(), motionBaseKeyId, "OculusTouch_RightTrigger", SInputSymbol::Trigger),
            SInputSymbol(Trigger::L2.GetNameCrc32(), motionBaseKeyId, "OculusTouch_LeftHandTrigger", SInputSymbol::Trigger),
            SInputSymbol(Trigger::R2.GetNameCrc32(), motionBaseKeyId, "OculusTouch_RightHandTrigger", SInputSymbol::Trigger),

            SInputSymbol(ThumbStickAxis1D::LX.GetNameCrc32(), motionBaseKeyId, "OculusTouch_LeftThumbstickX", SInputSymbol::Axis),
            SInputSymbol(ThumbStickAxis1D::LY.GetNameCrc32(), motionBaseKeyId, "OculusTouch_LeftThumbstickY", SInputSymbol::Axis),
            SInputSymbol(ThumbStickAxis1D::RX.GetNameCrc32(), motionBaseKeyId, "OculusTouch_RightThumbstickX", SInputSymbol::Axis),
            SInputSymbol(ThumbStickAxis1D::RY.GetNameCrc32(), motionBaseKeyId, "OculusTouch_RightThumbstickY", SInputSymbol::Axis)
        };

        system.GetIInput()->AddAzToLyInputDevice(eIDT_MotionController,
                                                 "Oculus Touch Controller",
                                                 azToCryInputSymbols,
                                                 GetInputDeviceId());
    }

    void OculusTouchController::ConnectToControllerBus() 
    {
        AZ::VR::ControllerRequestBus::Handler::BusConnect();
    }

    void OculusTouchController::DisconnectFromControllerBus()
    {
        AZ::VR::ControllerRequestBus::Handler::BusDisconnect();
    }

    void OculusTouchController::SetCurrentTrackingState(const AZ::VR::TrackingState trackingState[static_cast<uint32_t>(AZ::VR::ControllerIndex::MaxNumControllers)])
    {
        for (uint32 index = 0; index < static_cast<uint32>(AZ::VR::ControllerIndex::MaxNumControllers); ++index)
        {
            m_trackingState[index] = trackingState[index];
        }
    }
} // namespace Oculus

