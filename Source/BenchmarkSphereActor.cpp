#include "BenchmarkSphereActor.h"

#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

ABenchmarkSphereActor::ABenchmarkSphereActor()
{
    PrimaryActorTick.bCanEverTick = false;

    // Create one mesh component and use it as the root
    SphereMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SphereMesh"));
    RootComponent = SphereMesh;

    // Load the built-in engine sphere mesh
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));

    if (SphereMeshAsset.Succeeded())
    {
        SphereMesh->SetStaticMesh(SphereMeshAsset.Object);
    }

    // Make the sphere lightweight for the benchmark
    SphereMesh->SetMobility(EComponentMobility::Movable);

    // Disable extra gameplay overhead
    SphereMesh->SetGenerateOverlapEvents(false);
    SphereMesh->SetCanEverAffectNavigation(false);
    SphereMesh->SetNotifyRigidBodyCollision(false);
    SphereMesh->SetReceivesDecals(false);

    // Disable rendering features that are not needed
    SphereMesh->CastShadow = false;
    SphereMesh->bCastDynamicShadow = false;
    SphereMesh->bCastStaticShadow = false;
    SphereMesh->bCastContactShadow = false;
    SphereMesh->bAffectDistanceFieldLighting = false;
    SphereMesh->bAffectDynamicIndirectLighting = false;
    SphereMesh->bRenderCustomDepth = false;
    SphereMesh->SetRenderCustomDepth(false);

    // Disable tick on the component itself
    SphereMesh->PrimaryComponentTick.bCanEverTick = false;

    // Enable simple physics and collision
    SphereMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    SphereMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
    SphereMesh->SetSimulatePhysics(true);

    // Physics damping (similar to Unity Rigidbody defaults)
    SphereMesh->SetLinearDamping(0.05f);
    SphereMesh->SetAngularDamping(0.05f);

    // Disable Continuous Collision Detection (expensive)
    SphereMesh->BodyInstance.bUseCCD = false;

    // Allow bodies to go to sleep faster to reduce solver load
    SphereMesh->BodyInstance.SleepFamily = ESleepFamily::Custom;
    SphereMesh->BodyInstance.CustomSleepThresholdMultiplier = 2.0f;

    // Disable physics notifications
    SphereMesh->BodyInstance.bNotifyRigidBodyCollision = false;

    // Start hidden because pooled actors are activated later
    SetActorHiddenInGame(true);
    SetActorTickEnabled(false);
}

void ABenchmarkSphereActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    // Apply benchmark material if assigned
    if (SphereMaterial)
    {
        SphereMesh->SetMaterial(0, SphereMaterial);
    }
}