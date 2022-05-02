
#pragma once

#include <AzCore/Component/Component.h>

#include <Desolate/DesolateBus.h>

namespace Desolate
{
    class DesolateSystemComponent
        : public AZ::Component
        , protected DesolateRequestBus::Handler
    {
    public:
        AZ_COMPONENT(DesolateSystemComponent, "{A42CD799-8D9A-46B2-BC15-835C25D59091}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // DesolateRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    };

} // namespace Desolate
