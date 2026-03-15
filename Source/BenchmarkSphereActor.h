#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BenchmarkSphereActor.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;

/*
 * Lightweight physical sphere actor used by the benchmark.
 * This is the Unreal equivalent of a simple Unity sphere prefab
 * with a mesh, collider and rigidbody.
 */
UCLASS()
class RESOURCEBENCHMARKUE5_API ABenchmarkSphereActor : public AActor
{
    GENERATED_BODY()

public:
    ABenchmarkSphereActor();

    /* Returns the mesh component used as the physical body */
    UStaticMeshComponent* GetSphereMesh() const { return SphereMesh; }

protected:
    virtual void OnConstruction(const FTransform& Transform) override;

    /* Optional lightweight material assigned in editor or blueprint */
    UPROPERTY(EditDefaultsOnly, Category = "Benchmark")
    UMaterialInterface* SphereMaterial = nullptr;

private:
    /* Single mesh component used for rendering and physics */
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* SphereMesh;
};