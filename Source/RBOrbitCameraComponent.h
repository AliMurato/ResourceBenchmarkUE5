#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RBOrbitCameraComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RESOURCEBENCHMARKUE5_API URBOrbitCameraComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URBOrbitCameraComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

public:
    /* Enables orbit motion */
    UPROPERTY(EditAnywhere, Category = "Orbit")
    bool bOrbitEnabled = true;

    /* Automatically switch player view to the owning camera actor */
    UPROPERTY(EditAnywhere, Category = "Orbit")
    bool bAutoActivateView = true;

    /* Center point of the orbit */
    UPROPERTY(EditAnywhere, Category = "Orbit")
    FVector OrbitCenter = FVector::ZeroVector;

    /* Orbit radius in centimeters */
    UPROPERTY(EditAnywhere, Category = "Orbit")
    float OrbitRadius = 4000.0f;

    /* Angular speed in degrees per second */
    UPROPERTY(EditAnywhere, Category = "Orbit")
    float OrbitSpeedDegPerSec = -6.0f;

    /* Fixed camera height in centimeters */
    UPROPERTY(EditAnywhere, Category = "Orbit")
    float OrbitHeight = 0.0f;

    /* Initial orbit angle in degrees */
    UPROPERTY(EditAnywhere, Category = "Orbit")
    float StartAngleDeg = -90.0f;

    /* Wait for Space before starting orbit motion */
    UPROPERTY(EditAnywhere, Category = "Start Control")
    bool bWaitForSpaceToStart = true;

private:
    float CurrentAngleDeg = 0.0f;
    bool bOrbitStarted = false;

    /* Updates owner location and look-at rotation */
    void ApplyOrbitTransform();
};