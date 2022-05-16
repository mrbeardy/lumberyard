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

#include <OVR_CAPI.h>
#include <OVR_Version.h>

namespace Oculus 
{

class OculusTouchController
    : public AZ::VR::ControllerRequestBus::Handler
    , public AzFramework::InputDevice
    , public AzFramework::InputHapticFeedbackRequestBus::Handler
    , protected CrySystemEventBus::Handler
{
    public:

        OculusTouchController(ovrSession& session);
        ~OculusTouchController();

        // AzFramework::InputDevice overrides //////////////////////////////////////
        AZ_CLASS_ALLOCATOR(OculusTouchController, AZ::SystemAllocator, 0);
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

        void SetCurrentTrackingState(const AZ::VR::TrackingState trackingState[static_cast<uint32_t>(AZ::VR::ControllerIndex::MaxNumControllers)]);

    private:

        OculusTouchController(const OculusTouchController& other) = delete;
        OculusTouchController& operator=(const OculusTouchController& other) = delete;

        using ButtonChannelByIdMap = AZStd::unordered_map<AzFramework::InputChannelId, AzFramework::InputChannelDigital*>;
        using TriggerChannelByIdMap = AZStd::unordered_map<AzFramework::InputChannelId, AzFramework::InputChannelAnalog*>;
        using ThumbStickAxis1DChannelByIdMap = AZStd::unordered_map<AzFramework::InputChannelId, AzFramework::InputChannelAxis1D*>;
        using ThumbStickAxis2DChannelByIdMap = AZStd::unordered_map<AzFramework::InputChannelId, AzFramework::InputChannelAxis2D*>;

        ovrSession&                    m_session;
        ovrControllerType              m_connectionState;
        AZ::VR::TrackingState          m_trackingState[static_cast<uint32_t>(AZ::VR::ControllerIndex::MaxNumControllers)];
        InputChannelByIdMap            m_allChannelsById;
        ButtonChannelByIdMap           m_buttonChannelsById;
        TriggerChannelByIdMap          m_triggerChannelsById;
        ThumbStickAxis1DChannelByIdMap m_thumbStickAxis1DChannelsById;
        ThumbStickAxis2DChannelByIdMap m_thumbStickAxis2DChannelsById;
};

} // namespace Oculus


