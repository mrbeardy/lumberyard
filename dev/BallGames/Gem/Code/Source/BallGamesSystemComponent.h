
#pragma once

#include <AzCore/Component/Component.h>

#include <BallGames/BallGamesBus.h>

namespace BallGames
{
    class BallGamesSystemComponent
        : public AZ::Component
        , protected BallGamesRequestBus::Handler
    {
    public:
        AZ_COMPONENT(BallGamesSystemComponent, "{A7D62FC0-1192-4FEE-BAF4-1597B039BF36}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // BallGamesRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    };

} // namespace BallGames
