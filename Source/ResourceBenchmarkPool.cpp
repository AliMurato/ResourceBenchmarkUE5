#include "ResourceBenchmarkPool.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"

/*
 * Initialize
 *
 * Pre-spawns actors and puts them into an inactive pool.
 * This removes runtime allocation cost during the benchmark.
 */
void FResourceBenchmarkPool::Initialize(UWorld* World, TSubclassOf<AActor> ActorClass, int32 Count)
{
    if (!World || !ActorClass)
    {
        return;
    }

    // Reserve memory to avoid reallocations during pool creation
    Pool.Reserve(Count);

    for (int32 i = 0; i < Count; i++)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AActor* Actor = World->SpawnActor<AActor>(
            ActorClass,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            Params
        );

        if (!Actor)
        {
            continue;
        }

        // Keep actor inactive until reused
        Actor->SetActorHiddenInGame(true);
        Actor->SetActorTickEnabled(false);

        // Disable physics and collision while pooled
        if (UPrimitiveComponent* Phys = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
        {
            Phys->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Phys->SetSimulatePhysics(false);
        }

        Pool.Add(Actor);
    }
}

/*
 * Acquire
 *
 * Returns one actor from the pool.
 * The controller is responsible for reactivating it.
 */
AActor* FResourceBenchmarkPool::Acquire()
{
    if (Pool.Num() == 0)
    {
        return nullptr;
    }

    return Pool.Pop();
}