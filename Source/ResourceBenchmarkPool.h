#pragma once

#include "CoreMinimal.h"

class AActor;
class UPrimitiveComponent;

/*
 * Lightweight actor pool used by the benchmark.
 *
 * Responsibilities:
 *  - pre-spawn pooled actors at startup
 *  - keep them inactive until needed
 *  - return actors on demand without new allocations
 */
class FResourceBenchmarkPool
{
public:

    /* Prewarms the pool with Count actors */
    void Initialize(UWorld* World, TSubclassOf<AActor> ActorClass, int32 Count);

    /* Returns one pooled actor or nullptr if the pool is empty */
    AActor* Acquire();

private:

    /* Storage for inactive pooled actors */
    TArray<AActor*> Pool;
};