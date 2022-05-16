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

#include "OpenVR_precompiled.h"
#include "OpenVRController.h"
#include <sstream>

#include <IInput.h> // Temp until deprecated

namespace OpenVR
{
    using namespace AzFramework;

    // Input devices implemented in AzFramework all expose their InputDeviceId and associated
    // InputChannelIds as publically accessible static members. We could probably do the same
    // for VR controllers at some point, but once we have settled on a default common layout.
    //
    // Also, VR controllers should perhaps derive from InputDeviceGamepad::Implementation, but
    // OpenVR is somewhat of an outlier in that the input channels don't directly translate to
    // gamepad buttons/triggers/thumbsticks.
    //
    // So for now at least it is likely best to keep the visibility of these ids local to this
    // .cpp file, to minimise the impact if we decide to change or consolidate them in future.

    // The id used to identify a pair of OpenVR controller devices
    const InputDeviceId ControllerId("openvr_controllers");

    // All the input channel ids that identify OpenVR controller digital button input
    namespace Button
    {
        // Neither the A or directional buttons are supported by
        // the hardware (yet?), but they are defined by the SDK.
        const InputChannelId AL("openvr_button_a_l");               // The A button on the left controller
        const InputChannelId AR("openvr_button_a_r");               // The A button on the right controller
        const InputChannelId DUL("openvr_button_d_up_l");           // The up directional pad button on the left controller
        const InputChannelId DDL("openvr_button_d_down_l");         // The down directional pad button on the left controller
        const InputChannelId DLL("openvr_button_d_left_l");         // The left directional pad button on the left controller
        const InputChannelId DRL("openvr_button_d_right_l");        // The right directional pad button on the left controller
        const InputChannelId DUR("openvr_button_d_up_r");           // The up directional pad button on the right controller
        const InputChannelId DDR("openvr_button_d_down_r");         // The down directional pad button on the right controller
        const InputChannelId DLR("openvr_button_d_left_r");         // The left directional pad button on the right controller
        const InputChannelId DRR("openvr_button_d_right_r");        // The right directional pad button on the right controller

        const InputChannelId GripL("openvr_button_grip_l");         // The grip button on the left controller
        const InputChannelId GripR("openvr_button_grip_r");         // The grip button on the right controller
        const InputChannelId StartL("openvr_button_start_l");       // The menu button on the left controller
        const InputChannelId StartR("openvr_button_start_r");       // The menu button on the right controller
        const InputChannelId SelectL("openvr_button_select_l");     // The system button on the left controller
        const InputChannelId SelectR("openvr_button_select_r");     // The system button on the right controller
        const InputChannelId TriggerL("openvr_button_trigger_l");   // The trigger button on the left controller
        const InputChannelId TriggerR("openvr_button_trigger_r");   // The trigger button on the right controller
        const InputChannelId TouchpadL("openvr_button_touchpad_l"); // The touchpad button on the left controller
        const InputChannelId TouchpadR("openvr_button_touchpad_r"); // The touchpad button on the right controller

        // All OpenVR controller digital button ids
        const AZStd::array<InputChannelId, 20> All =
        {{
            AL,
            AR,
            DUL,
            DDL,
            DLL,
            DRL,
            DUR,
            DDR,
            DLR,
            DRR,
            GripL,
            GripR,
            StartL,
            StartR,
            SelectL,
            SelectR,
            TriggerL,
            TriggerR,
            TouchpadL,
            TouchpadR
        }};

        // Map of left hand controller digital button ids keyed by their OpenVR button bitmask
        const AZStd::array<AZStd::pair<AZ::u64, const InputChannelId*>, 10> LeftHandIdByBitMaskMap =
        {{
            { vr::ButtonMaskFromId(vr::k_EButton_A),                &AL },
            { vr::ButtonMaskFromId(vr::k_EButton_DPad_Up),          &DUL },
            { vr::ButtonMaskFromId(vr::k_EButton_DPad_Down),        &DDL },
            { vr::ButtonMaskFromId(vr::k_EButton_DPad_Left),        &DLL },
            { vr::ButtonMaskFromId(vr::k_EButton_DPad_Right),       &DRL },
            { vr::ButtonMaskFromId(vr::k_EButton_Grip),             &GripL },
            { vr::ButtonMaskFromId(vr::k_EButton_ApplicationMenu),  &StartL },
            { vr::ButtonMaskFromId(vr::k_EButton_System),           &SelectL },
            { vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger),  &TriggerL },
            { vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Touchpad), &TouchpadL }
        }};

        // Map of right hand controller digital button ids keyed by their OpenVR button bitmask
        const AZStd::array<AZStd::pair<AZ::u64, const InputChannelId*>, 10> RightHandIdByBitMaskMap =
        {{
            { vr::ButtonMaskFromId(vr::k_EButton_A),                &AR },
            { vr::ButtonMaskFromId(vr::k_EButton_DPad_Up),          &DUR },
            { vr::ButtonMaskFromId(vr::k_EButton_DPad_Down),        &DDR },
            { vr::ButtonMaskFromId(vr::k_EButton_DPad_Left),        &DLR },
            { vr::ButtonMaskFromId(vr::k_EButton_DPad_Right),       &DRR },
            { vr::ButtonMaskFromId(vr::k_EButton_Grip),             &GripR },
            { vr::ButtonMaskFromId(vr::k_EButton_ApplicationMenu),  &StartR },
            { vr::ButtonMaskFromId(vr::k_EButton_System),           &SelectR },
            { vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger),  &TriggerR },
            { vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Touchpad), &TouchpadR }
        } };
    }

    // All the input channel ids that identify OpenVR controller analog trigger input
    namespace Trigger
    {
        const InputChannelId L1("openvr_trigger_l1"); // The left controller index trigger
        const InputChannelId R1("openvr_trigger_r1"); // The right controller index trigger

        // All OpenVR controller analog trigger ids
        const AZStd::array<InputChannelId, 2> All =
        {{
            L1,
            R1
        }};
    }

    // All the input channel ids that identify OpenVR controller touch-pad axis 1D input
    namespace TouchPadAxis1D
    {
        const AzFramework::InputChannelId LX("openvr_touchpad_l_x"); // X-axis of the left-hand touch-pad
        const AzFramework::InputChannelId LY("openvr_touchpad_l_y"); // Y-axis of the left-hand touch-pad
        const AzFramework::InputChannelId RX("openvr_touchpad_r_x"); // X-axis of the right-hand touch-pad
        const AzFramework::InputChannelId RY("openvr_touchpad_r_y"); // Y-axis of the right-hand touch-pad

        // All OpenVR controller touch-pad axis 1D ids
        const AZStd::array<AzFramework::InputChannelId, 4> All =
        {{
            LX,
            LY,
            RX,
            RY
        }};
    }

    // All the input channel ids that identify OpenVR controller touch-pad axis 2D input
    namespace TouchPadAxis2D
    {
        const AzFramework::InputChannelId L("openvr_touchpad_l"); // The left-hand touch-pad
        const AzFramework::InputChannelId R("openvr_touchpad_r"); // The right-hand touch-pad

        // All OpenVR controller touch-pad axis 2D ids
        const AZStd::array<AzFramework::InputChannelId, 2> All =
        {{
            L,
            R
        }};
    }

    OpenVRController::OpenVRController(vr::IVRSystem*& system)
        : InputDevice(ControllerId)
        , m_system(system)
        , m_connectedControllers()
        , m_trackingStatesByDeviceIndex()
        , m_allChannelsById()
        , m_buttonChannelsById()
        , m_triggerChannelsById()
        , m_touchPadAxis1DChannelsById()
        , m_touchPadAxis2DChannelsById()
    {
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

        // Create all touch-pad axis 1D input channels
        for (const InputChannelId& channelId : TouchPadAxis1D::All)
        {
            InputChannelAxis1D* channel = aznew InputChannelAxis1D(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_touchPadAxis1DChannelsById[channelId] = channel;
        }

        // Create all touch-pad axis 2D input channels
        for (const InputChannelId& channelId : TouchPadAxis2D::All)
        {
            InputChannelAxis2D* channel = aznew InputChannelAxis2D(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_touchPadAxis2DChannelsById[channelId] = channel;
        }

        CrySystemEventBus::Handler::BusConnect();
        InputHapticFeedbackRequestBus::Handler::BusConnect(GetInputDeviceId());
    }

    OpenVRController::~OpenVRController()
    {
        InputHapticFeedbackRequestBus::Handler::BusDisconnect(GetInputDeviceId());
        CrySystemEventBus::Handler::BusDisconnect();

        // Destroy all touch-pad axis 2D input channels
        for (const auto& channelById : m_touchPadAxis2DChannelsById)
        {
            delete channelById.second;
        }

        // Destroy all touch-pad axis 1D input channels
        for (const auto& channelById : m_touchPadAxis1DChannelsById)
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

    const AzFramework::InputDevice::InputChannelByIdMap& OpenVRController::GetInputChannelsById() const
    {
        return m_allChannelsById;
    }

    bool OpenVRController::IsSupported() const
    {
        // If this class exists the OpenVR gem is enabled, so touch controllers are supported
        return true;
    }

    bool OpenVRController::IsConnected() const
    {
        return GetConnectedDeviceIndexForAzControllerIndex(AZ::VR::ControllerIndex::LeftHand) != vr::k_unTrackedDeviceIndexInvalid &&
               GetConnectedDeviceIndexForAzControllerIndex(AZ::VR::ControllerIndex::RightHand) != vr::k_unTrackedDeviceIndexInvalid;
    }

    void OpenVRController::TickInputDevice()
    {
        if (m_system == nullptr)
        {
            return;
        }

        // The OpenVR SDK keeps track of which device is currently considered left vs right, and this can change from one frame to the next.
        // We rely on IVRSystem::GetTrackedDeviceIndexForControllerRole to obtain this information, which is provided by Valve so that each
        // developer doesn't have to figure this out for themselves: https://steamcommunity.com/app/358720/discussions/0/451848855009182860/

        // Get the current state of the left hand controller
        vr::VRControllerState_t leftHandControllerState;
        const vr::TrackedDeviceIndex_t leftHandDeviceIndex = GetConnectedDeviceIndexForAzControllerIndex(AZ::VR::ControllerIndex::LeftHand);
        if (!m_system->GetControllerState(leftHandDeviceIndex, &leftHandControllerState))
        {
            // Instead of returning, clear the input state so all potentially active channels get reset below.
            // The meset may not be necessary, but the docs don't explicitly state what state the struct will
            // be left in if GetControllerState fails, so better to be safe than sorry.
            memset(&leftHandControllerState, 0, sizeof(leftHandControllerState));
        }

        // Get the current state of the right hand controller
        vr::VRControllerState_t rightHandControllerState;
        const vr::TrackedDeviceIndex_t rightHandDeviceIndex = GetConnectedDeviceIndexForAzControllerIndex(AZ::VR::ControllerIndex::RightHand);
        if (!m_system->GetControllerState(rightHandDeviceIndex, &rightHandControllerState))
        {
            // Instead of returning, clear the input state so all potentially active channels get reset below.
            // The meset may not be necessary, but the docs don't explicitly state what state the struct will
            // be left in if GetControllerState fails, so better to be safe than sorry.
            memset(&rightHandControllerState, 0, sizeof(rightHandControllerState));
        }

        // Update digital button states for the left hand controller
        for (const auto& buttonIdByBitMaskPair : Button::LeftHandIdByBitMaskMap)
        {
            const AZ::u64 buttonState = leftHandControllerState.ulButtonPressed & buttonIdByBitMaskPair.first;
            const InputChannelId& inputChannelId = *(buttonIdByBitMaskPair.second);
            m_buttonChannelsById[inputChannelId]->ProcessRawInputEvent(buttonState != 0);
        }

        // Update digital button states for the right hand controller
        for (const auto& buttonIdByBitMaskPair : Button::RightHandIdByBitMaskMap)
        {
            const AZ::u64 buttonState = rightHandControllerState.ulButtonPressed & buttonIdByBitMaskPair.first;
            const InputChannelId& inputChannelId = *(buttonIdByBitMaskPair.second);
            m_buttonChannelsById[inputChannelId]->ProcessRawInputEvent(buttonState != 0);
        }

        // Update analog trigger states
        static const int triggerAxisIndex = vr::k_EButton_SteamVR_Trigger - vr::k_EButton_Axis0;
        m_triggerChannelsById[Trigger::L1]->ProcessRawInputEvent(leftHandControllerState.rAxis[triggerAxisIndex].x);
        m_triggerChannelsById[Trigger::R1]->ProcessRawInputEvent(rightHandControllerState.rAxis[triggerAxisIndex].x);

        // Update the left hand touch-pad states
        static const int touchpadAxisIndex = vr::k_EButton_SteamVR_Touchpad - vr::k_EButton_Axis0;
        const AZ::Vector2 leftTouchpadValues(leftHandControllerState.rAxis[touchpadAxisIndex].x, leftHandControllerState.rAxis[touchpadAxisIndex].y);
        m_touchPadAxis2DChannelsById[TouchPadAxis2D::L]->ProcessRawInputEvent(leftTouchpadValues);
        m_touchPadAxis1DChannelsById[TouchPadAxis1D::LX]->ProcessRawInputEvent(leftTouchpadValues.GetX());
        m_touchPadAxis1DChannelsById[TouchPadAxis1D::LY]->ProcessRawInputEvent(leftTouchpadValues.GetY());

        // Update the right hand touch-pad states
        const AZ::Vector2 rightTouchpadValues(rightHandControllerState.rAxis[touchpadAxisIndex].x, rightHandControllerState.rAxis[touchpadAxisIndex].y);
        m_touchPadAxis2DChannelsById[TouchPadAxis2D::R]->ProcessRawInputEvent(rightTouchpadValues);
        m_touchPadAxis1DChannelsById[TouchPadAxis1D::RX]->ProcessRawInputEvent(rightTouchpadValues.GetX());
        m_touchPadAxis1DChannelsById[TouchPadAxis1D::RY]->ProcessRawInputEvent(rightTouchpadValues.GetY());
    }

    void OpenVRController::SetVibration(float leftMotorSpeedNormalized, float rightMotorSpeedNormalized)
    {
        if (m_system == nullptr)
        {
            return;
        }

        static const unsigned short kMaxPulseDurationMicroSec = 3999; // 5ms - minimum vibrate time. 3999 max value

        const vr::TrackedDeviceIndex_t leftHandDeviceIndex = GetConnectedDeviceIndexForAzControllerIndex(AZ::VR::ControllerIndex::LeftHand);
        if (leftHandDeviceIndex != vr::k_unTrackedDeviceIndexInvalid)
        {
            m_system->TriggerHapticPulse(leftHandDeviceIndex, 0, static_cast<unsigned short>(leftMotorSpeedNormalized*kMaxPulseDurationMicroSec));
        }

        const vr::TrackedDeviceIndex_t rightHandDeviceIndex = GetConnectedDeviceIndexForAzControllerIndex(AZ::VR::ControllerIndex::RightHand);
        if (rightHandDeviceIndex != vr::k_unTrackedDeviceIndexInvalid)
        {
            m_system->TriggerHapticPulse(rightHandDeviceIndex, 0, static_cast<unsigned short>(rightMotorSpeedNormalized*kMaxPulseDurationMicroSec));
        }
    }

    AZ::VR::TrackingState* OpenVRController::GetTrackingState(AZ::VR::ControllerIndex controllerIndex)
    {
        const vr::TrackedDeviceIndex_t connectedDeviceIndex = GetConnectedDeviceIndexForAzControllerIndex(controllerIndex);
        auto trackingStateByDeviceIndex = m_trackingStatesByDeviceIndex.find(connectedDeviceIndex);
        return trackingStateByDeviceIndex != m_trackingStatesByDeviceIndex.end() ?
               &trackingStateByDeviceIndex->second :
               nullptr;
    }

    bool OpenVRController::IsConnected(AZ::VR::ControllerIndex controllerIndex)
    {
        return GetConnectedDeviceIndexForAzControllerIndex(controllerIndex) != vr::k_unTrackedDeviceIndexInvalid;
    }

    void OpenVRController::OnCrySystemInitialized(ISystem& system, const SSystemInitParams&)
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
            SInputSymbol(Button::AL.GetNameCrc32(), motionBaseKeyId, "OpenVR_A_0", SInputSymbol::Button),
            SInputSymbol(Button::AR.GetNameCrc32(), motionBaseKeyId, "OpenVR_A_1", SInputSymbol::Button),
            SInputSymbol(Button::DUL.GetNameCrc32(), motionBaseKeyId, "OpenVR_DPadUp_0", SInputSymbol::Button),
            SInputSymbol(Button::DDL.GetNameCrc32(), motionBaseKeyId, "OpenVR_DPadDown_0", SInputSymbol::Button),
            SInputSymbol(Button::DLL.GetNameCrc32(), motionBaseKeyId, "OpenVR_DPadLeft_0", SInputSymbol::Button),
            SInputSymbol(Button::DRL.GetNameCrc32(), motionBaseKeyId, "OpenVR_DPadRight_0", SInputSymbol::Button),
            SInputSymbol(Button::DUR.GetNameCrc32(), motionBaseKeyId, "OpenVR_DPadUp_1", SInputSymbol::Button),
            SInputSymbol(Button::DDR.GetNameCrc32(), motionBaseKeyId, "OpenVR_DPadDown_1", SInputSymbol::Button),
            SInputSymbol(Button::DLR.GetNameCrc32(), motionBaseKeyId, "OpenVR_DPadLeft_1", SInputSymbol::Button),
            SInputSymbol(Button::DRR.GetNameCrc32(), motionBaseKeyId, "OpenVR_DPadRight_1", SInputSymbol::Button),

            SInputSymbol(Button::GripL.GetNameCrc32(), motionBaseKeyId, "OpenVR_Grip_0", SInputSymbol::Button),
            SInputSymbol(Button::GripR.GetNameCrc32(), motionBaseKeyId, "OpenVR_Grip_1", SInputSymbol::Button),
            SInputSymbol(Button::StartL.GetNameCrc32(), motionBaseKeyId, "OpenVR_Application_0", SInputSymbol::Button),
            SInputSymbol(Button::StartR.GetNameCrc32(), motionBaseKeyId, "OpenVR_Application_1", SInputSymbol::Button),
            SInputSymbol(Button::SelectL.GetNameCrc32(), motionBaseKeyId, "OpenVR_System_0", SInputSymbol::Button),
            SInputSymbol(Button::SelectR.GetNameCrc32(), motionBaseKeyId, "OpenVR_System_1", SInputSymbol::Button),
            SInputSymbol(Button::TriggerL.GetNameCrc32(), motionBaseKeyId, "OpenVR_TriggerButton_0", SInputSymbol::Button),
            SInputSymbol(Button::TriggerR.GetNameCrc32(), motionBaseKeyId, "OpenVR_TriggerButton_1", SInputSymbol::Button),
            SInputSymbol(Button::TouchpadL.GetNameCrc32(), motionBaseKeyId, "OpenVR_TouchpadButton_0", SInputSymbol::Button),
            SInputSymbol(Button::TouchpadR.GetNameCrc32(), motionBaseKeyId, "OpenVR_TouchpadButton_1", SInputSymbol::Button),

            SInputSymbol(Trigger::L1.GetNameCrc32(), motionBaseKeyId, "OpenVR_Trigger_0", SInputSymbol::Trigger),
            SInputSymbol(Trigger::R1.GetNameCrc32(), motionBaseKeyId, "OpenVR_Trigger_1", SInputSymbol::Trigger),

            SInputSymbol(TouchPadAxis1D::LX.GetNameCrc32(), motionBaseKeyId, "OpenVR_TouchpadX_0", SInputSymbol::Axis),
            SInputSymbol(TouchPadAxis1D::LY.GetNameCrc32(), motionBaseKeyId, "OpenVR_TouchpadY_0", SInputSymbol::Axis),
            SInputSymbol(TouchPadAxis1D::RX.GetNameCrc32(), motionBaseKeyId, "OpenVR_TouchpadX_1", SInputSymbol::Axis),
            SInputSymbol(TouchPadAxis1D::RY.GetNameCrc32(), motionBaseKeyId, "OpenVR_TouchpadY_1", SInputSymbol::Axis)
        };

        system.GetIInput()->AddAzToLyInputDevice(eIDT_MotionController,
                                                 "OpenVR Controller",
                                                 azToCryInputSymbols,
                                                 GetInputDeviceId());
    }

    void OpenVRController::ConnectToControllerBus()
    {
        AZ::VR::ControllerRequestBus::Handler::BusConnect();
    }

    void OpenVRController::DisconnectFromControllerBus()
    {
        AZ::VR::ControllerRequestBus::Handler::BusDisconnect();
    }

    void OpenVRController::SetCurrentTrackingState(const vr::TrackedDeviceIndex_t deviceIndex, const AZ::VR::TrackingState& trackingState)
    {
        if (deviceIndex != vr::k_unTrackedDeviceIndexInvalid)
        {
            m_trackingStatesByDeviceIndex[deviceIndex] = trackingState;
        }
    }

    void OpenVRController::ConnectController(const vr::TrackedDeviceIndex_t deviceIndex)
    {
        if (deviceIndex == vr::k_unTrackedDeviceIndexInvalid)
        {
            return;
        }

        const bool wasConnected = IsConnected();
        m_connectedControllers.insert(deviceIndex);
        if (!wasConnected && IsConnected())
        {
            // Both the left and right controllers are now connected
            BroadcastInputDeviceConnectedEvent();
        }
    }

    void OpenVRController::DisconnectController(const vr::TrackedDeviceIndex_t deviceIndex)
    {
        const bool wasConnected = IsConnected();
        m_connectedControllers.erase(deviceIndex);
        if (wasConnected && !IsConnected())
        {
            // Either the the left or right controller just disconnected
            BroadcastInputDeviceDisconnectedEvent();
        }
    }

    vr::ETrackedControllerRole GetTrackedControllerRoleForAzControllerIndex(AZ::VR::ControllerIndex controllerIndex)
    {
        switch (controllerIndex)
        {
            case AZ::VR::ControllerIndex::LeftHand: return vr::TrackedControllerRole_LeftHand;
            case AZ::VR::ControllerIndex::RightHand: return vr::TrackedControllerRole_RightHand;
            default: return vr::TrackedControllerRole_Invalid;
        }
    }

    vr::TrackedDeviceIndex_t OpenVRController::GetConnectedDeviceIndexForAzControllerIndex(AZ::VR::ControllerIndex controllerIndex) const
    {
        if (!m_system)
        {
            return vr::k_unTrackedDeviceIndexInvalid;
        }

        const vr::ETrackedControllerRole trackedControllerRole = GetTrackedControllerRoleForAzControllerIndex(controllerIndex);
        if (trackedControllerRole == vr::TrackedControllerRole_Invalid)
        {
            return vr::k_unTrackedDeviceIndexInvalid;
        }

        const vr::TrackedDeviceIndex_t trackedDeviceIndex = m_system->GetTrackedDeviceIndexForControllerRole(trackedControllerRole);
        if (m_connectedControllers.find(trackedDeviceIndex) == m_connectedControllers.end())
        {
            return vr::k_unTrackedDeviceIndexInvalid;
        }

        return trackedDeviceIndex;
    }
} // namespace OpenVR
