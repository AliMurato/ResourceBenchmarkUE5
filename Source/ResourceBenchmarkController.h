#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "ResourceBenchmarkPool.h"
#include "ResourceBenchmarkCsvWriter.h"

#include "ResourceBenchmarkController.generated.h"

/*
 * Main benchmark controller.
 * Handles timing, spawning, logging and automatic stop.
 */
UCLASS()
class RESOURCEBENCHMARKUE5_API AResourceBenchmarkController : public AActor
{
    GENERATED_BODY()

public:
    AResourceBenchmarkController();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:

    /* Spawned actor class */
    UPROPERTY(EditAnywhere, Category = "Spawn")
    TSubclassOf<AActor> SphereClass;

    /* Total number of active spheres for the run */
    UPROPERTY(EditAnywhere, Category = "Spawn")
    int32 MaxSpawnCount = 15000;

    /* Time between spawn steps */
    UPROPERTY(EditAnywhere, Category = "Spawn")
    float SpawnInterval = 0.005f;

    /* Half-size of the square spawn area in cm */
    UPROPERTY(EditAnywhere, Category = "Spawn")
    float SpawnAreaHalfExtent = 1150.0f;

    /* Vertical offset of the spawn plane in cm */
    UPROPERTY(EditAnywhere, Category = "Spawn")
    float SpawnHeightOffset = 1150.0f;

    /* Initial linear speed in cm/s */
    UPROPERTY(EditAnywhere, Category = "Initial Motion")
    float InitialSpeed = 600.0f;

    /* Initial angular speed in rad/s */
    UPROPERTY(EditAnywhere, Category = "Initial Motion")
    float AngularSpeed = 2.0f;

    /* Number of prewarmed pooled actors */
    UPROPERTY(EditAnywhere, Category = "Pooling")
    int32 PrewarmPoolCount = 15000;

    /* Seed for deterministic random stream */
    UPROPERTY(EditAnywhere, Category = "Reproducibility")
    int32 RandomSeed = 12345;

    /* Marker logging starts only after this time */
    UPROPERTY(EditAnywhere, Category = "Warm-up & Logging")
    float WarmupSeconds = 5.0f;

    /* Enable CSV output */
    UPROPERTY(EditAnywhere, Category = "Warm-up & Logging")
    bool bWriteCsv = true;

    /* Write one marker every N spawns */
    UPROPERTY(EditAnywhere, Category = "Warm-up & Logging")
    int32 LogEveryNSpawns = 100;

    /* Smoothing factor for frame time */
    UPROPERTY(EditAnywhere, Category = "FPS smoothing")
    float FpsSmoothing = 0.05f;

    /* Stop automatically after max count is reached */
    UPROPERTY(EditAnywhere, Category = "Auto stop when complete")
    bool bAutoStopWhenComplete = true;

    /* Delay before quitting */
    UPROPERTY(EditAnywhere, Category = "Auto stop when complete")
    float StopDelaySeconds = 1.0f;

    /* Stop automatically when smoothed FPS drops below this threshold */
    UPROPERTY(EditAnywhere, Category = "Auto stop when complete")
    float StopFpsThreshold = 10.0f;

    /* Allow ESC to stop the run */
    UPROPERTY(EditAnywhere, Category = "Manual stop")
    bool bAllowManualStopWithEsc = true;

    /* Wait for Space before starting the benchmark */
    UPROPERTY(EditAnywhere, Category = "Start Control")
    bool bWaitForSpaceToStart = true;

private:
    /* Pooled actor manager */
    FResourceBenchmarkPool Pool;

    /* CSV writer */
    FResourceBenchmarkCsvWriter Csv;

    /* Deterministic random stream */
    FRandomStream RandomStream;

    /* Spawn timer accumulator */
    float SpawnTimer = 0.0f;

    /* Number of successfully activated spheres */
    int32 CurrentSpawnCount = 0;

    /* Benchmark start time */
    double StartTime = 0.0;

    /* Smoothed delta time for FPS estimate */
    float SmoothedDeltaTime = 0.0f;

    /* Number of benchmark frames after actual start */
    int32 FrameCount = 0;

    /* Benchmark time origin for average FPS calculation */
    double BenchmarkStartTimeS = 0.0;

    /* True after warm-up logging becomes active */
    bool bWarmupCompleted = false;

    /* Prevent duplicate marker rows */
    int32 LastLoggedSpawnCount = 0;

    /* Pending marker state */
    bool bHasPendingMarker = false;
    int32 PendingMarkerCount = 0;

    /* Stop state */
    bool bStopRequested = false;
    double StopAtTime = 0.0;
    FString StopReason = TEXT("unknown");

    /* Final run snapshot */
    float AverageFps = 0.0f;
    float FinalFpsSmooth = 0.0f;
    float FinalFpsInstant = 0.0f;
    float FinalFrameTimeMs = 0.0f;
    double FinalStopTimeS = 0.0;

    /* True after the benchmark is actually started */
    bool bBenchmarkStarted = false;

    /* Ensures the start prompt is shown only once */
    bool bStartPromptShown = false;

private:
    /* Activates one pooled sphere */
    bool SpawnSphere(float FpsSmooth, float FpsInstant, float FrameMs);

    /* Checks whether a marker should be queued */
    bool ShouldQueueMarker() const;

    /* Writes one marker row and mirrors it to HUD */
    void WriteMarkerAtCount(int32 SpawnedCount, float FrameMs, float FpsSmooth, float FpsInstant);

    /* Schedules benchmark stop */
    void RequestStop(const FString& InStopReason, float FpsSmooth, float FpsInstant, float FrameMs);

    /* Closes files and exits */
    void StopRun();
};