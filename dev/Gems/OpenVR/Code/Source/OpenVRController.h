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

#pragma once

#include <VRControllerBus.h>
#include <CrySystemBus.h>

#include <AzFramework/Input/Buses/Requests/InputHapticFeedbackRequestBus.h>
#include <AzFramework/Input/Channels/InputChannelAnalog.h>
#include <AzFramework/Input/Channels/InputChannelAxis1D.h>
#include <AzFramework/Input/Channels/InputChannelAxis2D.h>
#include <AzFramework/Input/Channels/InputChannelDigital.h>
#include <AzFramework/Input/Devices/InputDevice.h>

#include <openvr.h>

namespace OpenVR
{

class OpenVRController
    : public AZ::VR::ControllerRequestBus::Handler
    , public AzFramework::InputDevice
    , public AzFramework::InputHapticFeedbackRequestBus::Handler
    , protected CrySystemEventBus::Handler
{
    public:

        OpenVRController(vr::IVRSystem*& system);
        ~OpenVRController();

        // AzFramework::InputDevice overrides //////////////////////////////////////
        AZ_CLASS_ALLOCATOR(OpenVRController, AZ::SystemAllocator, 0);
        const InputChannelByIdMap& GetInputChannelsById() const override;
        bool IsSupported() const override;
        bool IsConnected() const override;
        void TickInputDevice() override;
        ////////////////////////////////////////////////////////////////////////////

        // AzFramework::InputHapticFeedbackRequests overrides //////////////////////
        void SetVibration(float leftMotorSpeedNormalized, float rightMotorSpeedNormalized) override;
        ////////////////////////////////////////////////////////////////////////////
       
        // ControllerBus overrides ////////////////////////////////////////////////
        AZ::VR::TrackingState* GetTrackingState(AZ::VR::ControllerIndex controllerIndex) override;
        bool IsConnected(AZ::VR::ControllerIndex controllerIndex) override;
        ////////////////////////////////////////////////////////////////////////////

        // CrySystemEventBus ///////////////////////////////////////////////////////
        void OnCrySystemInitialized(ISystem& system, const SSystemInitParams&) override;
        ////////////////////////////////////////////////////////////////////////////

        void ConnectToControllerBus();
        void DisconnectFromControllerBus();

        void SetCurrentTrackingState(const vr::TrackedDeviceIndex_t deviceIndex, const AZ::VR::TrackingState& trackingState);

        void ConnectController(const vr::TrackedDeviceIndex_t deviceIndex);
        void DisconnectController(const vr::TrackedDeviceIndex_t deviceIndex);

    private:

        OpenVRController(const OpenVRController& other) = delete;
        OpenVRController& operator=(const OpenVRController& other) = delete;

        vr::TrackedDeviceIndex_t GetConnectedDeviceIndexForAzControllerIndex(AZ::VR::ControllerIndex controllerIndex) const;

        using ConnectedDeviceIndexSet = AZStd::unordered_set<vr::TrackedDeviceIndex_t>;
        using TrackingStateByDeviceIndexMap = AZStd::unordered_map<vr::TrackedDeviceIndex_t, AZ::VR::TrackingState>;
        using ButtonChannelByIdMap = AZStd::unordered_map<AzFramework::InputChannelId, AzFramework::InputChannelDigital*>;
        using TriggerChannelByIdMap = AZStd::unordered_map<AzFramework::InputChannelId, AzFramework::InputChannelAnalog*>;
        using TouchPadAxis1DChannelByIdMap = AZStd::unordered_map<AzFramework::InputChannelId, AzFramework::InputChannelAxis1D*>;
        using TouchPadAxis2DChannelByIdMap = AZStd::unordered_map<AzFramework::InputChannelId, AzFramework::InputChannelAxis2D*>;

        vr::IVRSystem*&                m_system; // Note: the system can be created *after* the device is created
        ConnectedDeviceIndexSet        m_connectedControllers;
        TrackingStateByDeviceIndexMap  m_trackingStatesByDeviceIndex;
        InputChannelByIdMap            m_allChannelsById;
        ButtonChannelByIdMap           m_buttonChannelsById;
        TriggerChannelByIdMap          m_triggerChannelsById;
        TouchPadAxis1DChannelByIdMap   m_touchPadAxis1DChannelsById;
        TouchPadAxis2DChannelByIdMap   m_touchPadAxis2DChannelsById;
};
        
} // namespace OpenVR

