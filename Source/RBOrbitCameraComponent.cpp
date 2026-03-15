#include "RBOrbitCameraComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"

URBOrbitCameraComponent::URBOrbitCameraComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    // Update after physics for smoother motion in physics-heavy scenes
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void URBOrbitCameraComponent::BeginPlay()
{
    Super::BeginPlay();

    // Initialize orbit angle and snap owner to the orbit path
    CurrentAngleDeg = StartAngleDeg;
    ApplyOrbitTransform();

    // Automatically switch player view to the owning actor
    if (bAutoActivateView)
    {
        if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
        {
            if (AActor* Owner = GetOwner())
            {
                PC->SetViewTarget(Owner);
            }
        }
    }

    // Start orbit immediately only if waiting is disabled
    bOrbitStarted = !bWaitForSpaceToStart;
}

void URBOrbitCameraComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bOrbitEnabled)
    {
        return;
    }

    // Wait for Space before starting orbit motion
    if (!bOrbitStarted)
    {
        if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
        {
            if (PC->WasInputKeyJustPressed(EKeys::SpaceBar))
            {
                bOrbitStarted = true;
            }
        }

        return;
    }

    // Advance orbit angle
    CurrentAngleDeg += OrbitSpeedDegPerSec * DeltaTime;

    if (CurrentAngleDeg >= 360.0f)
    {
        CurrentAngleDeg -= 360.0f;
    }
    else if (CurrentAngleDeg < 0.0f)
    {
        CurrentAngleDeg += 360.0f;
    }

    ApplyOrbitTransform();
}

void URBOrbitCameraComponent::ApplyOrbitTransform()
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    const float AngleRad = FMath::DegreesToRadians(CurrentAngleDeg);

    const float X = OrbitCenter.X + FMath::Cos(AngleRad) * OrbitRadius;
    const float Y = OrbitCenter.Y + FMath::Sin(AngleRad) * OrbitRadius;
    const float Z = OrbitHeight;

    const FVector NewLocation(X, Y, Z);
    Owner->SetActorLocation(NewLocation);

    // Always look at the orbit center
    const FRotator LookAtRotation = (OrbitCenter - NewLocation).Rotation();
    Owner->SetActorRotation(LookAtRotation);
}