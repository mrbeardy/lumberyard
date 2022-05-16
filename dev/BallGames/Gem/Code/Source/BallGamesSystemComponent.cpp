
#include <BallGamesSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace BallGames
{
    void BallGamesSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<BallGamesSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<BallGamesSystemComponent>("BallGames", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void BallGamesSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("BallGamesService"));
    }

    void BallGamesSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("BallGamesService"));
    }

    void BallGamesSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void BallGamesSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void BallGamesSystemComponent::Init()
    {
    }

    void BallGamesSystemComponent::Activate()
    {
        BallGamesRequestBus::Handler::BusConnect();
    }

    void BallGamesSystemComponent::Deactivate()
    {
        BallGamesRequestBus::Handler::BusDisconnect();
    }

} // namespace BallGames
