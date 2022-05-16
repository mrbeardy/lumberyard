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

#include "Substance_precompiled.h"

#include "ProceduralMaterialHandle.h"

#include <Substance/SubstanceBus.h>
#include <Substance/ISubstanceAPI.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector2.h>



// Provides a set of reflected functions for operating on ProceduralMaterial graphs through a ProceduralMaterialHandle.
// We keep these cpp-private instead of putting them in the header to align with the fact that
// ProceduralMaterialHandle is not useful on the code side, and only exists to support reflection.
namespace ProceduralMaterialHandleFunctions
{
    void ReportError(const char* functionName, const char* inputName, const char* message)
    {
        AZ_Error("Substance", false, "%s(InputName='%s') - %s", functionName, inputName, message);
    }

    void ReportWarning(const char* functionName, const char* inputName, const char* message)
    {
        AZ_Warning("Substance", false, "%s(InputName='%s') - %s", functionName, inputName, message);
    }

#if defined(AZ_USE_SUBSTANCE)
    //! Helper function for finding an IGraphInstance
    //! \param thisPtr             Procedural Material that the script is operating on
    //! \param inputParameterName  Name of the Input Parameter that the script is operating on
    //! \param functionName        Name of the script function that was called, for error messages
    IGraphInstance* GetGraph(ProceduralMaterialHandle* thisPtr, AZStd::string_view inputParameterName, const char* functionName)
    {
        IGraphInstance* graphInstance = nullptr;


        if (thisPtr)
        {
            SubstanceRequestBus::BroadcastResult(graphInstance, &SubstanceRequests::GetGraphInstance, thisPtr->m_graphInstance);
        }

        if (!graphInstance)
        {
            // This is only a warning because it could be a temporary situation that will correct itself
            ReportWarning(functionName, inputParameterName.data(), "ProceduralMaterial is Invalid");
        }

        return graphInstance;
    }
#endif // AZ_USE_SUBSTANCE

#if defined(AZ_USE_SUBSTANCE)
    //! Helper function for finding an IGraphInput
    //! \param graphInstance       Substance Graph
    //! \param inputParameterName  Name of the Input Parameter that the script is operating on
    //! \param functionName        Name of the script function that was called, for error messages
    IGraphInput* GetGraphInput(IGraphInstance* graphInstance, AZStd::string_view inputParameterName, const char* functionName)
    {
        if (graphInstance)
        {
            if (IGraphInput* graphInput = graphInstance->GetInputByName(inputParameterName.data()))
            {
                return graphInput;
            }
            else
            {
                ReportError(functionName, inputParameterName.data(), "There is no Input Parameter with this name");
            }
        }

        return nullptr;
    }
#endif // AZ_USE_SUBSTANCE

#if defined(AZ_USE_SUBSTANCE)
    //! Helper function for finding an IGraphInput (for cases where the IGraphInstance isn't needed)
    //! \param thisPtr             Procedural Material that the script is operating on
    //! \param inputParameterName  Name of the Input Parameter that the script is operating on
    //! \param functionName        Name of the script function that was called, for error messages
    IGraphInput* GetGraphInput(ProceduralMaterialHandle* thisPtr, AZStd::string_view inputParameterName, const char* functionName)
    {
        IGraphInstance* graphInstance = GetGraph(thisPtr, inputParameterName, functionName);
        return GetGraphInput(graphInstance, inputParameterName, functionName);
    }
#endif // AZ_USE_SUBSTANCE

    //! Sets a Procedural Material's Input Parameter value, presuming that it is a numeric type. Can support 1-,2-,3-, and 4-dimensional parameters.
    //! Since all numbers in Script Canvas are floats, the float values will be cast to ints when the Input Parameter is an Integer type.
    //! If successful, this will queue a Render Update with the Substance system. Otherwise, it will report error messages.
    //! \param Dimension           The number of numbers that will be set
    //! \param thisPtr             Procedural Material that the script is operating on
    //! \param inputParameterName  Name of the Input Parameter to set
    //! \param values              Array of 'Dimension' floats
    //! \param functionName        Name of the script function that was called, for error messages
    // (It's called "InputNumber" instead of "InputFloat" because in Script Canvas all primitives are just "numbers")
    template<int Dimension>
    void SetInputNumbers(ProceduralMaterialHandle* thisPtr, AZStd::string_view inputParameterName, const float values[Dimension], const char* functionName)
    {
#if defined(AZ_USE_SUBSTANCE)
        IGraphInstance* graph = GetGraph(thisPtr, inputParameterName, functionName);
        IGraphInput* graphInput = GetGraphInput(graph, inputParameterName, functionName);

        if (graph && graphInput)
        {
            const GraphInputType type = graphInput->GetInputType();

            bool didSet = false;

            float floatInput[4] = { 0.0f,0.0f,0.0f,0.0f };
            int     intInput[4] = { 0,0,0,0 };

            switch (type)
            {
            case GraphInputType::Integer1:
            case GraphInputType::Integer2:
            case GraphInputType::Integer3:
            case GraphInputType::Integer4:
                for (int i = 0; i < Dimension; i++)
                {
                    intInput[i] = static_cast<int>(values[i]);
                }
                graphInput->SetValue(intInput);
                didSet = true;
                break;
            case GraphInputType::Float1:
            case GraphInputType::Float2:
            case GraphInputType::Float3:
            case GraphInputType::Float4:
                memcpy(floatInput, values, Dimension * sizeof(float));
                graphInput->SetValue(floatInput);
                didSet = true;
                break;
            case GraphInputType::Image:
                ReportError(functionName, inputParameterName.data(), "This function cannot be used with Image type");
                break;
            case GraphInputType::String:
                ReportError(functionName, inputParameterName.data(), "This function cannot be used with String type");
                break;
            default:
                AZ_Assert(false, "Unhandled Substance Input Type %d", type);
            }

            if (didSet)
            {
                SubstanceRequestBus::Broadcast(&SubstanceRequests::QueueRender, graph);
            }
        }
#endif // AZ_USE_SUBSTANCE
    }

    //! Gets a Procedural Material's Input Parameter value, presuming that it is a numeric type. Can support 1-,2-,3-, and 4-dimensional parameters.
    //! Since all numbers in Script Canvas are floats, the int values will be cast to floats when the Input Parameter is a Float type.
    //! Will report error messages if the Input Parameter could not be found or is not a numeric type.
    //! \param Dimension           The number of numbers that will be set
    //! \param thisPtr             Procedural Material that the script is operating on
    //! \param inputParameterName  Name of the Input Parameter to set
    //! \param valuesOut           Array of 'Dimension' floats that will receive the output
    //! \param functionName        Name of the script function that was called, for error messages
    // (It's called "InputNumber" instead of "InputFloat" because in Script Canvas all primitives are just "numbers")
    template<int Dimension>
    void GetInputNumbers(ProceduralMaterialHandle* thisPtr, AZStd::string_view inputParameterName, float valuesOut[Dimension], const char* functionName)
    {
        memset(valuesOut, 0, Dimension * sizeof(float));

#if defined(AZ_USE_SUBSTANCE)

        IGraphInput* graphInput = GetGraphInput(thisPtr, inputParameterName, functionName);

        if (graphInput)
        {
            const GraphValueVariant value = graphInput->GetValue();
            const GraphInputType type = graphInput->GetInputType();

            const int* intValues = nullptr;

            switch (type)
            {
            case GraphInputType::Integer1:
            case GraphInputType::Integer2:
            case GraphInputType::Integer3:
            case GraphInputType::Integer4:
                intValues = static_cast<const int*>(value);
                for (int i = 0; i < Dimension; i++)
                {
                    valuesOut[i] = static_cast<float>(intValues[i]);
                }
                break;
            case GraphInputType::Float1:
            case GraphInputType::Float2:
            case GraphInputType::Float3:
            case GraphInputType::Float4:
                memcpy(valuesOut, static_cast<const float*>(value), Dimension * sizeof(float));
                break;
            case GraphInputType::Image:
                ReportError(functionName, inputParameterName.data(), "This function cannot be used with Image type");
                break;
            case GraphInputType::String:
                ReportError(functionName, inputParameterName.data(), "This function cannot be used with String type");
                break;
            default:
                AZ_Assert(false, "Unhandled Substance Input Type %d", type);
            }
        }
#endif // AZ_USE_SUBSTANCE
    }

    void SetInputColor(ProceduralMaterialHandle* thisPtr, AZStd::string_view inputParameterName, const AZ::Color& value)
    {
        float values[4];
        value.StoreToFloat4(values);
        SetInputNumbers<4>(thisPtr, inputParameterName, values, "SetInputColor");
    }

    void SetInputVector4(ProceduralMaterialHandle* thisPtr, AZStd::string_view inputParameterName, const AZ::Vector4& value)
    {
        float values[4];
        value.StoreToFloat4(values);
        SetInputNumbers<4>(thisPtr, inputParameterName, values, "SetInputVector4");
    }

    void SetInputVector3(ProceduralMaterialHandle* thisPtr, AZStd::string_view inputParameterName, const AZ::Vector3& value)
    {
        float values[3];
        value.StoreToFloat3(values);
        SetInputNumbers<3>(thisPtr, inputParameterName, values, "SetInputVector3");
    }

    void SetInputVector2(ProceduralMaterialHandle* thisPtr, AZStd::string_view inputParameterName, const AZ::Vector2& value)
    {
        float values[2];
        value.StoreToFloat2(values);
        SetInputNumbers<2>(thisPtr, inputParameterName, values, "SetInputVector2");
    }

    // (It's called "InputNumber" instead of "InputFloat" because in Script Canvas all primitives are just "numbers")
    void SetInputNumber(ProceduralMaterialHandle* thisPtr, AZStd::string_view inputParameterName, float value)
    {
        float values[1] = { value };
        SetInputNumbers<1>(thisPtr, inputParameterName, values, "SetInputNumber"); // SetInputNumber instead of SetInputNumber to match how it's reflected in BehaviorContext
    }

    void SetInputString(ProceduralMaterialHandle* thisPtr, AZStd::string_view inputParameterName, AZStd::string_view value)
    {
#if defined(AZ_USE_SUBSTANCE)
        IGraphInstance* graph = GetGraph(thisPtr, inputParameterName, "SetInputString");
        IGraphInput* graphInput = GetGraphInput(graph, inputParameterName, "SetInputString");

        if (graph && graphInput)
        {
            const GraphInputType type = graphInput->GetInputType();

            if (type == GraphInputType::String || type == GraphInputType::Image)
            {
                graphInput->SetValue(value.data());
                SubstanceRequestBus::Broadcast(&SubstanceRequests::QueueRender, graph);
            }
            else
            {
                ReportError("SetInputString", inputParameterName.data(), "This function can only be used with String and Image Input types");
            }
        }
#endif // AZ_USE_SUBSTANCE
    }

    AZ::Color GetInputColor(ProceduralMaterialHandle* thisPtr, AZStd::string_view inputParameterName)
    {
        float values[4];
        GetInputNumbers<4>(thisPtr, inputParameterName, values, "GetInputColor");
        return AZ::Color::CreateFromFloat4(values);
    }

    AZ::Vector4 GetInputVector4(ProceduralMaterialHandle* thisPtr, AZStd::string_view inputParameterName)
    {
        float values[4];
        GetInputNumbers<4>(thisPtr, inputParameterName, values, "GetInputVector4");
        return AZ::Vector4::CreateFromFloat4(values);
    }

    AZ::Vector3 GetInputVector3(ProceduralMaterialHandle* thisPtr, AZStd::string_view inputParameterName)
    {
        float values[3];
        GetInputNumbers<3>(thisPtr, inputParameterName, values, "GetInputVector3");
        return AZ::Vector3::CreateFromFloat3(values);
    }

    AZ::Vector2 GetInputVector2(ProceduralMaterialHandle* thisPtr, AZStd::string_view inputParameterName)
    {
        float values[2];
        GetInputNumbers<2>(thisPtr, inputParameterName, values, "GetInputVector2");
        return AZ::Vector2::CreateFromFloat2(values);
    }

    // (It's called "InputNumber" instead of "InputFloat" because in Script Canvas all primitives are just "numbers")
    float GetInputNumber(ProceduralMaterialHandle* thisPtr, AZStd::string_view inputParameterName)
    {
        float values[1];
        GetInputNumbers<1>(thisPtr, inputParameterName, values, "GetInputNumber");
        return values[0];
    }


    AZStd::string GetInputString(ProceduralMaterialHandle* thisPtr, AZStd::string_view inputParameterName)
    {
#if defined(AZ_USE_SUBSTANCE)
        IGraphInput* graphInput = GetGraphInput(thisPtr, inputParameterName, "GetInputString");

        if (graphInput)
        {
            const GraphInputType type = graphInput->GetInputType();

            if (type == GraphInputType::String || type == GraphInputType::Image)
            {
                return static_cast<const char*>(graphInput->GetValue());
            }
            else
            {
                ReportError("GetInputString", inputParameterName.data(), "This function can only be used with String and Image Input types");
            }
        }
#endif // AZ_USE_SUBSTANCE
            
        return "";
    }

    //! May change the Substance Name provided by a script, to support multiple extensions that all refer to the same actual material.
    AZStd::string TouchupSubstanceName(AZStd::string_view substanceName, const char* functionName)
    {
        AZStd::string newName = substanceName;

        // Support both sbsar and smtl extensions. Includes smtl because that's the built-in behavior of the Substance API. 
        // Includes sbsar because that's the path that gets displayed in the Substance Editor, and is easy for the user to copy/paste.
        const AZStd::string mainExtension = ".smtl";
        const AZStd::string altExtension = ".sbsar";

        if (AZStd::string::npos != substanceName.find(altExtension, substanceName.size() - altExtension.size()))
        {
            newName = newName.substr(0, substanceName.size() - altExtension.size()) + mainExtension;
        }
        else if (AZStd::string::npos == substanceName.find(mainExtension, substanceName.size() - mainExtension.size()))
        {
            AZ_Warning("Substance", false, "%s() - Substance Name '%s' does not have a '%s' or '%s' extension. The material probably won't be found.", functionName, substanceName.data(), mainExtension.c_str(), altExtension.c_str());
        }

        return newName;
    }

    //! Static script function for finding a Procedural Material
    //! \param substanceName  The name of a Substance Material. May be either the ".smtl" file or ".sbsar" file.
    //! \param graphName      The name of a graph within the Substance Material
    //! \param forceLoad      Whether to force the Procedural Material to be loaded. If false, the function will return a Invalid material if it isn't already loaded.
    ProceduralMaterialHandle FindByName(AZStd::string_view substanceName, AZStd::string_view graphName, bool forceLoad)
    {
        ProceduralMaterialHandle found;

#if defined(AZ_USE_SUBSTANCE)
        
        const AZStd::string newSubstanceName = TouchupSubstanceName(substanceName, "FindByName");

        IProceduralMaterial* material = nullptr;
        SubstanceRequestBus::BroadcastResult(material, &SubstanceRequests::GetMaterialFromPath, newSubstanceName.c_str(), forceLoad);

        ISubstanceLibAPI* substanceLibAPI = nullptr;
        SubstanceRequestBus::BroadcastResult(substanceLibAPI, &SubstanceRequests::GetSubstanceLibAPI);

        if (material && substanceLibAPI)
        {
            const int numGraphs = material->GetGraphInstanceCount();
            for (int i = 0; i < numGraphs; ++i)
            {
                if (material->GetGraphInstance(i)->GetName() == graphName)
                {
                    found.m_graphInstance = substanceLibAPI->EncodeGraphInstanceID(material, i);
                    break;
                }
            }
        }
#endif // AZ_USE_SUBSTANCE

        return found;
    }

    //! Static script function for finding a Procedural Material
    //! \param substanceName  The name of a Substance Material. May be either the ".smtl" file or ".sbsar" file.
    //! \param graphIndex     The index of a graph within the Substance Material
    //! \param forceLoad      Whether to force the Procedural Material to be loaded. If false, the function will return an Invalid material if it isn't already loaded.
    ProceduralMaterialHandle FindByNameAndIndex(AZStd::string_view substanceName, int graphIndex, bool forceLoad)
    {
        ProceduralMaterialHandle found;

#if defined(AZ_USE_SUBSTANCE)

        const AZStd::string newSubstanceName = TouchupSubstanceName(substanceName, "FindByNameAndIndex");

        IProceduralMaterial* material = nullptr;
        SubstanceRequestBus::BroadcastResult(material, &SubstanceRequests::GetMaterialFromPath, newSubstanceName.c_str(), forceLoad);

        ISubstanceLibAPI* substanceLibAPI = nullptr;
        SubstanceRequestBus::BroadcastResult(substanceLibAPI, &SubstanceRequests::GetSubstanceLibAPI);

        if (material && substanceLibAPI)
        {
            found.m_graphInstance = substanceLibAPI->EncodeGraphInstanceID(material, graphIndex);
        }
        
#endif // AZ_USE_SUBSTANCE

        return found;
    }

    //! Static script function for asynchronous rendering of queued Procedural Material Input Parameter updates
    //! \param force         If true, all queued Graphs will be sent to the renderer. This is better for occasional updates, where you need to guarantee that the job is submitted.
    //!                      If false, each queued Graph will be sent to the renderer only if all prior renders of that Graph have finished. This is better for high frequency 
    //!                      updates, to prevent backing up the internal render queue.
    //! \return              A RenderID for the scheduled render job. This can be used to confirm when the render job has completed. Will return 0 if no render job was scheduled (which
    //!                      can happen if there weren't any updates queued, or if force=false and the render was blocked by a pending render).
    ProceduralMaterialRenderUID RenderAsync(bool force)
    {
        ProceduralMaterialRenderUID renderUID = INVALID_PROCEDURALMATERIALRENDERUID;

#if defined(AZ_USE_SUBSTANCE)
        SubstanceRequestBus::BroadcastResult(renderUID, &SubstanceRequests::RenderASync, force);
#endif // AZ_USE_SUBSTANCE

        return renderUID;
    }

    //! Static script function for rendering all queued Procedural Material Input Parameter updates. The render is synchronous, so the render is guaranteed to complete before returning.
    void RenderSync()
    {
#if defined(AZ_USE_SUBSTANCE)
        SubstanceRequestBus::Broadcast(&SubstanceRequests::RenderSync);
#endif // AZ_USE_SUBSTANCE
    }

}

//////////////////////////////////////////////////////////////////////////

//! Handler/binding code that is required for Behavior Context reflection of EBus Notifications.
class SubstanceNotificationBusBehaviorHandler : public SubstanceNotificationBus::Handler, public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(SubstanceNotificationBusBehaviorHandler, "{68D0FC8A-FCC7-4016-81E6-1A522EAF6A12}", AZ::SystemAllocator
        , OnRenderFinished);

    void OnRenderFinished(ProceduralMaterialRenderUID renderUID) override
    {
        Call(FN_OnRenderFinished, renderUID);
    }
};

void ProceduralMaterialHandle::Reflect(AZ::ReflectContext* reflectContext)
{
    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
    {
        // This is required in order to create a ProceduralMaterialHandle variable in script canvas.
        serializeContext->Class<ProceduralMaterialHandle>()->Version(0);
    }

    if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
    {
        const char* setMaterialInputTooltip = "Sets a Procedural Material's Input Parameter value";
        const char* getMaterialInputTooltip = "Gets a Procedural Material's Input Parameter value";
        AZ::BehaviorParameterOverrides substanceNameDetails = { "SubstanceName", "Path name of the Substance '.sbsar' or '.smtl' file" };
        AZ::BehaviorParameterOverrides setMaterialDetails = { "ProceduralMaterial", "The Procedural Material to modify" };
        AZ::BehaviorParameterOverrides getMaterialDetails = { "ProceduralMaterial", "The Procedural Material to query" };
        AZ::BehaviorParameterOverrides renderMaterialDetails = { "ProceduralMaterial", "The Procedural Material to render" };
        AZ::BehaviorParameterOverrides setInputNameDetails = { "InputName", "The name of the Input Parameter to set" };
        AZ::BehaviorParameterOverrides getInputNameDetails = { "InputName", "The name of the Input Parameter to return" };
        const AZStd::array<AZ::BehaviorParameterOverrides, 2> getMaterialInputArgs = { { getMaterialDetails,getInputNameDetails } };
        const char* newValueTooltip = "The new value to apply";

        behaviorContext->Class<ProceduralMaterialHandle>("ProceduralMaterial")
            ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::RuntimeOwn)
            ->Attribute(AZ::Script::Attributes::Category, "Rendering")
            ->Method("RenderAsync", &ProceduralMaterialHandleFunctions::RenderAsync, { { { "Force", "Forces the render to be scheduled (false gives better performance on high frequency updates)", behaviorContext->MakeDefaultValue(true) } } })
            ->Attribute(AZ::Script::Attributes::ToolTip, "Schedules asynchronous regeneration of Procedural Material textures based on the latest Input Parameter settings")
            ->Method("RenderSync", &ProceduralMaterialHandleFunctions::RenderSync)
            ->Attribute(AZ::Script::Attributes::ToolTip, "Regenerates the Procedural Material textures based on the latest Input Parameter settings")
            ->Method("FindByName", &ProceduralMaterialHandleFunctions::FindByName, { { substanceNameDetails,{ "GraphName", "Name of a specific graph in the Substance file" },{ "ForceLoad", "Forces the Procedural Material to be loaded if it isn't already", behaviorContext->MakeDefaultValue(true) } } })
            ->Attribute(AZ::Script::Attributes::ToolTip, "Finds a ProceduralMaterial by name. Returns Invalid if the material could not be found or loaded.")
            ->Method("FindByNameAndIndex", &ProceduralMaterialHandleFunctions::FindByNameAndIndex, { { substanceNameDetails,{ "GraphIndex", "Index of a specific graph in the Substance file" },{ "ForceLoad", "Forces the Procedural Material to be loaded if it isn't already", behaviorContext->MakeDefaultValue(true) } } })
            ->Attribute(AZ::Script::Attributes::ToolTip, "Finds a ProceduralMaterial by name and graph index. Returns Invalid if the material could not be found or loaded.")
            ->Method("SetInputVector4", &ProceduralMaterialHandleFunctions::SetInputVector4,
            { { setMaterialDetails,setInputNameDetails,{ "Vector4", newValueTooltip } } })
            ->Attribute(AZ::Script::Attributes::ToolTip, setMaterialInputTooltip)
            ->Method("SetInputVector3", &ProceduralMaterialHandleFunctions::SetInputVector3,
            { { setMaterialDetails,setInputNameDetails,{ "Vector3", newValueTooltip } } })
            ->Attribute(AZ::Script::Attributes::ToolTip, setMaterialInputTooltip)
            ->Method("SetInputVector2", &ProceduralMaterialHandleFunctions::SetInputVector2,
            { { setMaterialDetails,setInputNameDetails,{ "Vector2", newValueTooltip } } })
            ->Attribute(AZ::Script::Attributes::ToolTip, setMaterialInputTooltip)
            ->Method("SetInputNumber", &ProceduralMaterialHandleFunctions::SetInputNumber, // It's called "InputNumber" instead of "InputFloat" because in Script Canvas all primitives are just "numbers"
            { { setMaterialDetails,setInputNameDetails,{ "Number", newValueTooltip } } })
            ->Attribute(AZ::Script::Attributes::ToolTip, setMaterialInputTooltip)
            ->Method("SetInputColor", &ProceduralMaterialHandleFunctions::SetInputColor,
            { { setMaterialDetails,setInputNameDetails,{ "Color", newValueTooltip } } })
            ->Attribute(AZ::Script::Attributes::ToolTip, setMaterialInputTooltip)
            ->Method("SetInputString", &ProceduralMaterialHandleFunctions::SetInputString,
            { { setMaterialDetails,setInputNameDetails,{ "String", newValueTooltip } } })
            ->Attribute(AZ::Script::Attributes::ToolTip, setMaterialInputTooltip)
            ->Method("GetInputVector4", &ProceduralMaterialHandleFunctions::GetInputVector4, getMaterialInputArgs)
            ->Attribute(AZ::Script::Attributes::ToolTip, getMaterialInputTooltip)
            ->Method("GetInputVector3", &ProceduralMaterialHandleFunctions::GetInputVector3, getMaterialInputArgs)
            ->Attribute(AZ::Script::Attributes::ToolTip, getMaterialInputTooltip)
            ->Method("GetInputVector2", &ProceduralMaterialHandleFunctions::GetInputVector2, getMaterialInputArgs)
            ->Attribute(AZ::Script::Attributes::ToolTip, getMaterialInputTooltip)
            ->Method("GetInputNumber", &ProceduralMaterialHandleFunctions::GetInputNumber, getMaterialInputArgs) // It's called "InputNumber" instead of "InputFloat" because in Script Canvas all primitives are just "numbers"
            ->Attribute(AZ::Script::Attributes::ToolTip, getMaterialInputTooltip)
            ->Method("GetInputColor", &ProceduralMaterialHandleFunctions::GetInputColor, getMaterialInputArgs)
            ->Attribute(AZ::Script::Attributes::ToolTip, getMaterialInputTooltip)
            ->Method("GetInputString", &ProceduralMaterialHandleFunctions::GetInputString, getMaterialInputArgs)
            ->Attribute(AZ::Script::Attributes::ToolTip, getMaterialInputTooltip)
            ;

        behaviorContext->EBus<SubstanceNotificationBus>("SubstanceNotificationBus", nullptr, "Provides notifications from the Substance system")
            ->Attribute(AZ::Script::Attributes::Category, "Rendering")
            ->Handler<SubstanceNotificationBusBehaviorHandler>()
            ;
    }
}
