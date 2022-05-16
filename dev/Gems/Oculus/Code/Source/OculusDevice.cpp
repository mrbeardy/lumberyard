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
#include "OculusDevice.h"
#include "OculusTouchController.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/MathUtils.h>
#include <MathConversion.h>
#include <IRenderAuxGeom.h>
#include <Oculus_Traits_Platform.h>

#define OVR_D3D_VERSION 11
#include <d3d11.h>
#include <OVR_CAPI_D3D.h>

#define LogMessage(...) CryLogAlways("[HMD][Oculus] - " __VA_ARGS__);

#if AZ_TRAIT_OCULUS_OCULUS_SUPPORTED
#undef min
#undef max
#endif

namespace Oculus
{

AZ::Quaternion OculusQuatToAZQuat(const ovrQuatf& ovrQuat)
{
    AZ::Quaternion quat(ovrQuat.x, ovrQuat.y, ovrQuat.z, ovrQuat.w);
    AZ::Matrix3x3 m33 = AZ::Matrix3x3::CreateFromQuaternion(quat);

    AZ::Vector3 column1 = -m33.GetColumn(2);
    m33.SetColumn(2, m33.GetColumn(1));
    m33.SetColumn(1, column1);
    AZ::Quaternion rotation = AZ::Quaternion::CreateFromMatrix3x3(m33);

    AZ::Quaternion result = AZ::Quaternion::CreateRotationX(AZ::Constants::HalfPi) * rotation;
    return result;
}

AZ::Vector3 OculusVec3ToAZVec3(const ovrVector3f& vec)
{
    AZ::Vector3 result;
    result.Set(vec.x, -vec.z, vec.y);
    return result;
}

void CopyPose(const ovrPoseStatef& src, AZ::VR::PoseState& poseDest, AZ::VR::DynamicsState& dynamicsDest, AZ::VR::HMDTrackingLevel trackingLevel)
{
    const ovrPosef* pose = &(src.ThePose);

    poseDest.orientation = OculusQuatToAZQuat(pose->Orientation);

    // fixed tracking level does not contain position information
    if (trackingLevel == AZ::VR::HMDTrackingLevel::kFixed)
    {
        poseDest.position = AZ::Vector3::CreateZero();
    }
    else
    {
        poseDest.position = OculusVec3ToAZVec3(pose->Position);
    }

    // Oculus devices return angular velocity and acceleration in world space. The engine promises these
    // in local space, which is the more intuitive way to use them. Multiplying by the orientation gets
    // these values in local space.
    dynamicsDest.angularVelocity = poseDest.orientation * OculusVec3ToAZVec3(src.AngularVelocity);
    dynamicsDest.angularAcceleration = poseDest.orientation * OculusVec3ToAZVec3(src.AngularAcceleration);

    // With regard to the note about angular velocity and acceleration, linear velocity and acceleration are fine in world space
    dynamicsDest.linearVelocity = OculusVec3ToAZVec3(src.LinearVelocity);
    dynamicsDest.linearAcceleration = OculusVec3ToAZVec3(src.LinearAcceleration);
}

void OculusDevice::Reflect(AZ::ReflectContext* context)
{
    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->Class<OculusDevice, AZ::Component>()
            ->Version(1)
            ;

        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
        {
            editContext->Class<OculusDevice>(
                "Oculus Device Manager", "")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "VR")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                ;
        }
    }
}

void OculusDevice::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
{
    provided.push_back(AZ_CRC("HMDDevice"));
    provided.push_back(AZ_CRC("OculusDevice"));
}

void OculusDevice::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
{
    incompatible.push_back(AZ_CRC("OculusDevice"));
}

void OculusDevice::Init()
{
    m_controller = new OculusTouchController(m_session); // Note that this will be deleted by the input system and should not be deleted here.
}

void OculusDevice::Activate()
{
    AZ::VR::HMDInitRequestBus::Handler::BusConnect();
}

void OculusDevice::Deactivate()
{
    AZ::VR::HMDInitRequestBus::Handler::BusDisconnect();
}

bool OculusDevice::AttemptInit()
{
    LogMessage("Attempting to initialize Oculus SDK " OVR_VERSION_STRING);

    //Init OVR SDK with no params; we have nothing special to do
    ovrResult result = ovr_Initialize(nullptr);
    if (!OVR_SUCCESS(result))
    {
        ovrErrorInfo error;
        ovr_GetLastErrorInfo(&error);
        LogMessage("ovr_Initialize failed with result: %s", error.ErrorString);

        Shutdown();
        return false;
    }

    //Create OVR session. Storing graphics LUID in case we need to check for headset disconnection
    result = ovr_Create(&m_session, &m_currentLUID);
    if (!OVR_SUCCESS(result))
    {
        ovrErrorInfo error;
        ovr_GetLastErrorInfo(&error);
        LogMessage("ovr_Create failed with result: %s", error.ErrorString);

        Shutdown();
        return false;
    }

    m_hmdDesc = ovr_GetHmdDesc(m_session);

    //Calculate device info
    ovrSizei leftEyeSize = ovr_GetFovTextureSize(m_session, ovrEye_Left, m_hmdDesc.DefaultEyeFov[ovrEye_Left], 1.0f);
    ovrSizei rightEyeSize = ovr_GetFovTextureSize(m_session, ovrEye_Right, m_hmdDesc.DefaultEyeFov[ovrEye_Right], 1.0f);

    ovrMatrix4f projectionMatrix = ovrMatrix4f_Projection(m_hmdDesc.DefaultEyeFov[ovrEye_Left], 0.01f, 1000.0f, ovrProjection_None);

    float denom = 1.0f / projectionMatrix.M[1][1];
    float fovv = 2.0f * atanf(denom);
    float aspectRatio = projectionMatrix.M[1][1] / projectionMatrix.M[0][0];
    float fovh = 2.0f * atanf(denom * aspectRatio);

    m_deviceInfo.fovH = fovh;
    m_deviceInfo.fovV = fovv;
    m_deviceInfo.productName = m_hmdDesc.ProductName;
    m_deviceInfo.manufacturer = m_hmdDesc.Manufacturer;
    m_deviceInfo.renderWidth = std::max(leftEyeSize.w, rightEyeSize.w);
    m_deviceInfo.renderHeight = std::max(leftEyeSize.h, rightEyeSize.h);

    // Default the tracking origin to the head.
    result = ovr_SetTrackingOriginType(m_session, ovrTrackingOrigin_EyeLevel);
    if (!OVR_SUCCESS(result))
    {
        ovrErrorInfo errorInfo;
        ovr_GetLastErrorInfo(&errorInfo);
        LogMessage("%s [%s]", "Unable to set tracking origin type for Oculus device!", errorInfo.ErrorString);

        Shutdown();
        return false;
    }

    // Ensure that queue-ahead is enabled.
    if (!ovr_SetBool(m_session, "QueueAheadEnabled", true))
    {
        LogMessage("unable to set bool QueueAheadEnabled on Oculus device!");

        Shutdown();
        return false;
    }

    // Connect to the HMDDeviceBus in order to get HMD messages from the rest of the VR system.
    AZ::VR::HMDDeviceRequestBus::Handler::BusConnect();
    m_controller->ConnectToControllerBus();
    OculusRequestBus::Handler::BusConnect();

    using namespace AZ::VR;
    VREventBus::Broadcast(&VREvents::OnHMDInitialized);

    return true;
}

AZ::VR::HMDInitBus::HMDInitPriority OculusDevice::GetInitPriority() const
{
    return HMDInitPriority::kHighest;
}

void OculusDevice::Shutdown()
{
    using namespace AZ::VR;
    VREventBus::Broadcast(&VREvents::OnHMDShutdown);

    m_controller->DisconnectFromControllerBus();

    AZ::VR::HMDDeviceRequestBus::Handler::BusDisconnect();
    OculusRequestBus::Handler::BusDisconnect();
    if (m_session)
    {
        ovr_Destroy(m_session);
        m_session = nullptr;
    }

    ovr_Shutdown();
}

void OculusDevice::GetPerEyeCameraInfo(const EStereoEye eye, const float nearPlane, const float farPlane, AZ::VR::PerEyeCameraInfo& cameraInfo)
{
    // Grab the frustum planes from the per-eye projection matrix.
    {
        ovrMatrix4f projectionMatrix = ovrMatrix4f_Projection(m_hmdDesc.DefaultEyeFov[eye], nearPlane, farPlane, ovrProjection_None);

        float denom = 1.0f / projectionMatrix.M[1][1];
        cameraInfo.fov = 2.0f * atanf(denom);
        cameraInfo.aspectRatio = projectionMatrix.M[1][1] / projectionMatrix.M[0][0];

        cameraInfo.frustumPlane.horizontalDistance = projectionMatrix.M[0][2] * denom * cameraInfo.aspectRatio;
        cameraInfo.frustumPlane.verticalDistance = projectionMatrix.M[1][2] * denom;
    }

    // Get the world-space offset from the head position for this eye.
    {
        FrameParameters& frameParams = GetFrameParameters();
        ovrVector3f eyeOffset = frameParams.viewScaleDesc.HmdToEyeOffset[eye];
        cameraInfo.eyeOffset = OculusVec3ToAZVec3(eyeOffset);
    }
}

void OculusDevice::UpdateInternalState()
{

}

bool OculusDevice::CreateRenderTargets(void* renderDevice, const TextureDesc& desc, size_t eyeCount, AZ::VR::HMDRenderTarget* renderTargets[])
{
    bool success = false;

    for (size_t i = 0; i < eyeCount; i++)
    {
        ID3D11Device* d3dDevice = static_cast<ID3D11Device*>(renderDevice);

        ovrTextureSwapChainDesc textureDesc = {};
        textureDesc.Type                    = ovrTexture_2D;
        textureDesc.ArraySize               = 1;
        textureDesc.Format                  = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
        textureDesc.Width                   = desc.width;
        textureDesc.Height                  = desc.height;
        textureDesc.MipLevels               = 1;
        textureDesc.SampleCount             = 1;
        textureDesc.MiscFlags               = ovrTextureMisc_DX_Typeless;
        textureDesc.BindFlags               = ovrTextureBind_DX_RenderTarget | ovrTextureBind_DX_UnorderedAccess;
        textureDesc.StaticImage             = ovrFalse;

        ovrTextureSwapChain& textureSwapchain = m_textureSwapchains[i];
        textureSwapchain = nullptr;
        ovrResult result = ovr_CreateTextureSwapChainDX(m_session, d3dDevice, &textureDesc, &textureSwapchain);

        success = OVR_SUCCESS(result) && (textureSwapchain != nullptr);
        if (success)
        {
            m_textureSwapchains[i] = textureSwapchain;

            int numTextures = 0;
            result = ovr_GetTextureSwapChainLength(m_session, textureSwapchain, &(numTextures));
            if (!OVR_SUCCESS(result))
            {
                ovrErrorInfo errorInfo;
                ovr_GetLastErrorInfo(&errorInfo);
                LogMessage("%s [%s]", "Unable to get texture swap chain length for Oculus device!", *errorInfo.ErrorString ? errorInfo.ErrorString : "unspecified error");
            }
            renderTargets[i]->numTextures = numTextures;

            renderTargets[i]->deviceSwapTextureSet = textureSwapchain;
            renderTargets[i]->textures = new void*[numTextures];
            for (int t = 0; t < numTextures; ++t)
            {
                ID3D11Texture2D* texture = nullptr;
                result = ovr_GetTextureSwapChainBufferDX(m_session, textureSwapchain, t, IID_PPV_ARGS(&texture));
                renderTargets[i]->textures[t] = texture;

                if (!OVR_SUCCESS(result))
                {
                    success = false;
                    break;
                }
            }
        }

        if (!success)
        {
            ovrErrorInfo errorInfo;
            ovr_GetLastErrorInfo(&errorInfo);
            LogMessage("%s [%s]", "Unable to create D3D11 texture swap chain!", *errorInfo.ErrorString ? errorInfo.ErrorString : "unspecified error");
        }
    }

    return success;
}

void OculusDevice::DestroyRenderTarget(AZ::VR::HMDRenderTarget& renderTarget)
{
    ovr_DestroyTextureSwapChain(m_session, static_cast<ovrTextureSwapChain>(renderTarget.deviceSwapTextureSet));
    SAFE_DELETE_ARRAY(renderTarget.textures);
    renderTarget.textures = nullptr;
}

AZ::VR::TrackingState* OculusDevice::GetTrackingState()
{
    return &m_trackingState;
}

void OculusDevice::SubmitFrame(const EyeTarget& left, const EyeTarget& right)
{
    FRAME_PROFILER("OculusDevice::SubmitFrame", gEnv->pSystem, PROFILE_SYSTEM);

    const EyeTarget* eyes[] = { &left, &right };
    FrameParameters& frameParams = GetFrameParameters();

    ovrLayerEyeFov layer;
    layer.Header.Type = ovrLayerType_EyeFov;
    layer.Header.Flags = 0;
    for (uint32 eye = 0; eye < 2; ++eye)
    {
        ovrTextureSwapChain eyeSwapchain = static_cast<ovrTextureSwapChain>(eyes[eye]->renderTarget);

        ovrResult result = ovr_CommitTextureSwapChain(m_session, eyeSwapchain);
        if (!OVR_SUCCESS(result))
        {
            ovrErrorInfo errorInfo;
            ovr_GetLastErrorInfo(&errorInfo);
            LogMessage("%s [%s]", "Unable to commit texture swap chain on Oculus device!", *errorInfo.ErrorString ? errorInfo.ErrorString : "unspecified error");
        }

        layer.ColorTexture[eye]     = eyeSwapchain;
        layer.Viewport[eye].Pos.x   = eyes[eye]->viewportPosition.x;
        layer.Viewport[eye].Pos.y   = eyes[eye]->viewportPosition.y;
        layer.Viewport[eye].Size.w  = eyes[eye]->viewportSize.x;
        layer.Viewport[eye].Size.h  = eyes[eye]->viewportSize.y;
        layer.Fov[eye]              = frameParams.eyeFovs[eye];
        layer.RenderPose[eye]       = frameParams.eyePoses[eye];
        layer.SensorSampleTime      = frameParams.sensorTimeSample;
    }

    ovrLayerHeader* layers = &layer.Header;
    ovrResult result = ovr_SubmitFrame(m_session, frameParams.frameID, &frameParams.viewScaleDesc, &layers, 1);
    if (!OVR_SUCCESS(result))
    {
        ovrErrorInfo errorInfo;
        ovr_GetLastErrorInfo(&errorInfo);
        LogMessage("%s [%s]", "Unable to submit frame to the Oculus device!", *errorInfo.ErrorString ? errorInfo.ErrorString : "unspecified error");
    }
}

void OculusDevice::RecenterPose()
{
    if (m_session)
    {
        ovrResult result = ovr_RecenterTrackingOrigin(m_session);
        if (!OVR_SUCCESS(result))
        {
            ovrErrorInfo errorInfo;
            ovr_GetLastErrorInfo(&errorInfo);
            LogMessage("%s [%s]", "Unable to recenter tracking origin on Oculus device!", *errorInfo.ErrorString ? errorInfo.ErrorString : "unspecified error");
        }
    }
}

void OculusDevice::SetTrackingLevel(const AZ::VR::HMDTrackingLevel level)
{
    ovrResult result;

    switch (level)
    {
        case AZ::VR::HMDTrackingLevel::kHead:
            result = ovr_SetTrackingOriginType(m_session, ovrTrackingOrigin_EyeLevel);
            break;

        case AZ::VR::HMDTrackingLevel::kFloor:
        case AZ::VR::HMDTrackingLevel::kFixed: // Note: m_trackingOrigin can be any value for fixed, but it has to be a valid value
            result = ovr_SetTrackingOriginType(m_session, ovrTrackingOrigin_FloorLevel);
            break;

        default:
            AZ_Assert(false, "Unknown tracking level %d requested for the Oculus", static_cast<int>(level));
            break;
    }

    if (!OVR_SUCCESS(result))
    {
        ovrErrorInfo errorInfo;
        ovr_GetLastErrorInfo(&errorInfo);
        LogMessage("%s [%s]", "Unable to set tracking origin type for Oculus device!", *errorInfo.ErrorString ? errorInfo.ErrorString : "unspecified error");
    }
    m_trackingLevel = level;
}

OculusDevice::FrameParameters& OculusDevice::GetFrameParameters()
{
    static IRenderer* renderer = gEnv->pRenderer;
    const int frameID = renderer->GetFrameID(false);

    return m_frameParams[frameID & 1]; // these parameters are double-buffered.
}

void OculusDevice::OutputHMDInfo()
{
    LogMessage("Device: %s", m_hmdDesc.ProductName[0] ? m_hmdDesc.ProductName : "unknown");
    LogMessage("- Manufacturer: %s", m_hmdDesc.Manufacturer[0] ? m_hmdDesc.Manufacturer : "unknown");
    LogMessage("- VendorId: %d", m_hmdDesc.VendorId);
    LogMessage("- ProductId: %d", m_hmdDesc.ProductId);
    LogMessage("- Firmware: %d.%d", m_hmdDesc.FirmwareMajor, m_hmdDesc.FirmwareMinor);
    LogMessage("- SerialNumber: %s", m_hmdDesc.SerialNumber);
    LogMessage("- Render Resolution: %dx%d", m_deviceInfo.renderWidth * 2, m_deviceInfo.renderHeight);
    LogMessage("- Horizontal FOV: %.2f degrees", RAD2DEG(m_deviceInfo.fovH));
    LogMessage("- Vertical FOV: %.2f degrees", RAD2DEG(m_deviceInfo.fovV));

    LogMessage("- Sensor orientation tracking: %s", (m_hmdDesc.AvailableTrackingCaps & ovrTrackingCap_Orientation) ? "supported" : "unsupported");
    LogMessage("- Sensor mag yaw correction: %s", (m_hmdDesc.AvailableTrackingCaps & ovrTrackingCap_MagYawCorrection) ? "supported" : "unsupported");
    LogMessage("- Sensor position tracking: %s", (m_hmdDesc.AvailableTrackingCaps & ovrTrackingCap_Position) ? "supported" : "unsupported");
}

void OculusDevice::EnableDebugging(bool enable)
{
    if (enable)
    {
        if (!ovr_SetInt(m_session, OVR_PERF_HUD_MODE, ovrPerfHud_PerfSummary))
        {
            LogMessage("unable to set int ovrPerfHud_PerfSummary on Oculus device!");
        }
    }
    else
    {
        if (!ovr_SetInt(m_session, OVR_PERF_HUD_MODE, ovrPerfHud_Off))
        {
            LogMessage("unable to set int ovrPerfHud_Off on Oculus device!");
        }

        if (!ovr_SetInt(m_session, OVR_LAYER_HUD_MODE, ovrLayerHud_Off))
        {
            LogMessage("unable to set int ovrLayerHud_Off on Oculus device!");
        }
    }
}

void OculusDevice::DrawDebugInfo(const AZ::Transform& transform, IRenderAuxGeom* auxGeom)
{
    // Draw the playspace.
    {
        const AZ::VR::Playspace* space = GetPlayspace();

        if (space)
        {
            ColorB white[] =
            {
                ColorB(255, 255, 255, 127),
                ColorB(255, 255, 255, 127),
                ColorB(255, 255, 255, 127),
                ColorB(255, 255, 255, 127),
                ColorB(255, 255, 255, 127),
                ColorB(255, 255, 255, 127),
            };

            // Offset to apply to avoid z-fighting.
            AZ::Vector3 offset(0.0f, 0.0f, 0.01f);
            const Vec3 planeTriangles[] =
            {
                // Triangle 1
                AZVec3ToLYVec3(transform * (space->corners[0] + offset)),
                AZVec3ToLYVec3(transform * (space->corners[1] + offset)),
                AZVec3ToLYVec3(transform * (space->corners[2] + offset)),

                // Triangle 2
                AZVec3ToLYVec3(transform * (space->corners[3] + offset)),
                AZVec3ToLYVec3(transform * (space->corners[0] + offset)),
                AZVec3ToLYVec3(transform * (space->corners[2] + offset))
            };

            SAuxGeomRenderFlags flags = auxGeom->GetRenderFlags();
            flags.SetAlphaBlendMode(e_AlphaBlended);
            auxGeom->SetRenderFlags(flags);
            auxGeom->DrawTriangles(&planeTriangles[0], 6, white);
        }
    }
}

AZ::VR::HMDDeviceInfo* OculusDevice::GetDeviceInfo()
{
    return &m_deviceInfo;
}

bool OculusDevice::IsInitialized()
{
    return m_session != nullptr;
}

const AZ::VR::Playspace* OculusDevice::GetPlayspace()
{
    const int NUM_FLOOR_POINTS = 4;
    ovrVector3f outDimensions[NUM_FLOOR_POINTS];
    int pointCount;
    m_playspace.isValid = false;

    // Default bounday for Oculus is the play area.
    ovrResult result = ovr_GetBoundaryGeometry(m_session, ovrBoundary_PlayArea, outDimensions, &pointCount);
    if (!OVR_SUCCESS(result))
    {
        if (result == ovrSuccess_BoundaryInvalid)
        {
            LogMessage("The boundary for the oculus device was not set up.");
        }
        else
        {
            ovrErrorInfo errorInfo;
            ovr_GetLastErrorInfo(&errorInfo);
            LogMessage("%s [%s]", "Unable to get playspace from Oculus device!", *errorInfo.ErrorString ? errorInfo.ErrorString : "unspecified error");
        }
    }
    else
    {
        AZ_Assert(pointCount == NUM_FLOOR_POINTS, "inconsistent number of floor points recieved from Oculus boundary. Expected %d, got %d", NUM_FLOOR_POINTS, pointCount);

        if (pointCount == NUM_FLOOR_POINTS)
        {
            m_playspace.isValid = true;

            // Need to go from clockwise starting at top left, to anti-clockwise starting at bottom right to keep consistent with OpenVR
            // Oculus Boundary
            // [0] - [1]
            //        |
            // [3] - [2]
            //
            // OpenVR Boundary
            // [2] - [1]
            //  |     |
            // [3]   [0]
            m_playspace.corners[0] = OculusVec3ToAZVec3(outDimensions[2]);
            m_playspace.corners[1] = OculusVec3ToAZVec3(outDimensions[1]);
            m_playspace.corners[2] = OculusVec3ToAZVec3(outDimensions[0]);
            m_playspace.corners[3] = OculusVec3ToAZVec3(outDimensions[3]);
        }
    }

    return &m_playspace;
}

void OculusDevice::UpdateTrackingStates()
{
    const int frameId = gEnv->pRenderer->GetFrameID(false);

    // Get the eye poses, with IPD (interpupillary distance) already included.
    ovrEyeRenderDesc eyeRenderDesc[2];
    eyeRenderDesc[0] = ovr_GetRenderDesc(m_session, ovrEye_Left, m_hmdDesc.DefaultEyeFov[0]);
    eyeRenderDesc[1] = ovr_GetRenderDesc(m_session, ovrEye_Right, m_hmdDesc.DefaultEyeFov[1]);

    ovrPosef eyePose[2];
    ovrVector3f hmdToEyeOffset[2] =
    {
        eyeRenderDesc[0].HmdToEyeOffset,
        eyeRenderDesc[1].HmdToEyeOffset
    };

    // Calculate the current tracking state.
    double predictedTime = ovr_GetPredictedDisplayTime(m_session, frameId);
    ovrTrackingState currentTrackingState = ovr_GetTrackingState(m_session, predictedTime, ovrTrue);
    ovr_CalcEyePoses(currentTrackingState.HeadPose.ThePose, hmdToEyeOffset, eyePose);

    CopyPose(currentTrackingState.HeadPose, m_trackingState.pose, m_trackingState.dynamics, m_trackingLevel);

    // Specify what is currently being tracked.
    int statusFlags = 0;
    ovrSessionStatus sessionStatus;
    ovrResult result = 0;

    result = ovr_GetSessionStatus(m_session, &sessionStatus);
    if (!OVR_SUCCESS(result))
    {
        ovrErrorInfo errorInfo;
        ovr_GetLastErrorInfo(&errorInfo);
        LogMessage("%s [%s]", "Unable to get session status for Oculus device!", *errorInfo.ErrorString ? errorInfo.ErrorString : "unspecified error");
        return;
    }

    statusFlags = AZ::VR::HMDStatus_CameraPoseTracked |
        ((!sessionStatus.DisplayLost) ? AZ::VR::HMDStatus_HmdConnected | AZ::VR::HMDStatus_PositionConnected : 0) |
        ((currentTrackingState.StatusFlags & ovrStatus_OrientationTracked) ? AZ::VR::HMDStatus_OrientationTracked : 0) |
        ((currentTrackingState.StatusFlags & ovrStatus_PositionTracked) ? AZ::VR::HMDStatus_PositionTracked : 0);

    m_trackingState.statusFlags = statusFlags;


    // Cache the current tracking state for later submission after this frame has finished rendering.
    FrameParameters& frameParams = GetFrameParameters();
    for (uint32 eye = 0; eye < 2; ++eye)
    {
        frameParams.eyeFovs[eye] = m_hmdDesc.DefaultEyeFov[eye];
        frameParams.eyePoses[eye] = eyePose[eye];
        frameParams.viewScaleDesc.HmdToEyeOffset[eye] = hmdToEyeOffset[eye];
    }

    frameParams.frameID = frameId;
    frameParams.sensorTimeSample = 0;   //No need to provide this since we called ovr_GetTrackingState with ovrTrue


    // Update the current controller state.
    AZ::VR::TrackingState controllerTrackingStates[static_cast<uint32_t>(AZ::VR::ControllerIndex::MaxNumControllers)];
    CopyPose(currentTrackingState.HandPoses[ovrHand_Left], controllerTrackingStates[static_cast<uint32_t>(AZ::VR::ControllerIndex::LeftHand)].pose, controllerTrackingStates[static_cast<uint32_t>(AZ::VR::ControllerIndex::LeftHand)].dynamics, m_trackingLevel);
    CopyPose(currentTrackingState.HandPoses[ovrHand_Right], controllerTrackingStates[static_cast<uint32_t>(AZ::VR::ControllerIndex::RightHand)].pose, controllerTrackingStates[static_cast<uint32_t>(AZ::VR::ControllerIndex::RightHand)].dynamics, m_trackingLevel);

    for (size_t controllerIndex = 0; controllerIndex < static_cast<uint32_t>(AZ::VR::ControllerIndex::MaxNumControllers); ++controllerIndex)
    {
        controllerTrackingStates[controllerIndex].statusFlags = statusFlags;
    }

    m_controller->SetCurrentTrackingState(controllerTrackingStates);
}

AZ::u32 OculusDevice::GetSwapchainIndex(const EStereoEye& eye)
{
    int index = 0;
    ovr_GetTextureSwapChainCurrentIndex(m_session, m_textureSwapchains[static_cast<int>(eye)], &index);
    return index;
}

} // namespace Oculus
