#include "ResourceBenchmarkController.h"

#include "Components/PrimitiveComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Misc/App.h"
#include "InputCoreTypes.h"

#include "BenchmarkHUD.h"

namespace
{
    const TCHAR* LogPrefix = TEXT("Unreal Engine 5.7.3");
}

AResourceBenchmarkController::AResourceBenchmarkController()
{
    // The controller updates every frame
    PrimaryActorTick.bCanEverTick = true;
}

void AResourceBenchmarkController::BeginPlay()
{
    Super::BeginPlay();

    // Seed deterministic random generator
    RandomStream.Initialize(RandomSeed);

    // Prewarm actor pool
    Pool.Initialize(GetWorld(), SphereClass, PrewarmPoolCount);

    // Prepare CSV output
    if (bWriteCsv)
    {
        Csv.Initialize();
    }

    // Initialize smoothed delta time
    SmoothedDeltaTime = FMath::Max(0.000001f, FApp::GetDeltaTime());

    // Start immediately only if waiting is disabled
    if (!bWaitForSpaceToStart)
    {
        StartTime = FPlatformTime::Seconds();
        BenchmarkStartTimeS = 0.0;
        bBenchmarkStarted = true;
    }
}

void AResourceBenchmarkController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    const float SafeDeltaTime = FMath::Max(0.000001f, DeltaTime);

    // Allow manual stop at any time
    if (bAllowManualStopWithEsc)
    {
        if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
        {
            if (PC->WasInputKeyJustPressed(EKeys::Escape))
            {
                const float FpsInstantEsc = 1.0f / SafeDeltaTime;
                const float FpsSmoothEsc = 1.0f / FMath::Max(0.000001f, SmoothedDeltaTime);
                const float FrameMsEsc = SmoothedDeltaTime * 1000.0f;

                RequestStop(TEXT("manual_stop"), FpsSmoothEsc, FpsInstantEsc, FrameMsEsc);
                return;
            }
        }
    }

    // Wait for Space before starting the benchmark
    if (!bBenchmarkStarted)
    {
        if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
        {
            // Show the start prompt only once
            if (!bStartPromptShown)
            {
                if (ABenchmarkHUD* HUD = Cast<ABenchmarkHUD>(PC->GetHUD()))
                {
                    HUD->PushLine(FString::Printf(TEXT("[%s] Press SPACE to start benchmark"), LogPrefix));
                }

                bStartPromptShown = true;
            }

            // Start the benchmark when Space is pressed
            if (PC->WasInputKeyJustPressed(EKeys::SpaceBar))
            {
                StartTime = FPlatformTime::Seconds();
                BenchmarkStartTimeS = 0.0;
                bBenchmarkStarted = true;

                if (ABenchmarkHUD* HUD = Cast<ABenchmarkHUD>(PC->GetHUD()))
                {
                    HUD->PushLine(FString::Printf(TEXT("[%s] Benchmark started"), LogPrefix));
                }
            }
        }

        return;
    }

    // Avoid backlog when window is unfocused
    if (!FApp::HasFocus())
    {
        SpawnTimer = 0.0f;
        return;
    }

    FrameCount++;

    // Smooth frame time for marker-only FPS
    SmoothedDeltaTime = FMath::Lerp(
        SmoothedDeltaTime,
        SafeDeltaTime,
        FpsSmoothing
    );

    const float FpsSmooth = 1.0f / SmoothedDeltaTime;
    const float FrameMs = SmoothedDeltaTime * 1000.0f;
    const float FpsInstant = 1.0f / SafeDeltaTime;
    const double TimeS = FPlatformTime::Seconds() - StartTime;

    // Stop benchmark if smoothed FPS drops below the threshold
    if (!bStopRequested && FpsSmooth <= StopFpsThreshold)
    {
        if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
        {
            if (ABenchmarkHUD* HUD = Cast<ABenchmarkHUD>(PC->GetHUD()))
            {
                const FString Msg = FString::Printf(
                    TEXT("[%s] FPS threshold reached (%.1f FPS). Stopping benchmark."),
                    LogPrefix,
                    FpsSmooth
                );

                HUD->PushLine(Msg);
            }
        }

        RequestStop(TEXT("fps_threshold"), FpsSmooth, FpsInstant, FrameMs);
        return;
    }

    // Enable logging after warm-up period
    if (!bWarmupCompleted && TimeS >= WarmupSeconds)
    {
        bWarmupCompleted = true;

        if (LogEveryNSpawns > 0)
        {
            const int32 Snapped = (CurrentSpawnCount / LogEveryNSpawns) * LogEveryNSpawns;

            if (Snapped > 0)
            {
                LastLoggedSpawnCount = Snapped;
                PendingMarkerCount = Snapped;
                bHasPendingMarker = true;
            }
        }
    }

    // Spawn catch-up loop
    SpawnTimer += SafeDeltaTime;

    while (SpawnTimer >= SpawnInterval && CurrentSpawnCount < MaxSpawnCount)
    {
        SpawnTimer -= SpawnInterval;

        const bool bSpawnSucceeded = SpawnSphere(FpsSmooth, FpsInstant, FrameMs);
        if (bSpawnSucceeded && ShouldQueueMarker())
        {
            LastLoggedSpawnCount = CurrentSpawnCount;
            PendingMarkerCount = CurrentSpawnCount;
            bHasPendingMarker = true;
        }

        if (bStopRequested)
        {
            break;
        }
    }

    // Write at most one marker per frame
    if (bHasPendingMarker && !bStopRequested)
    {
        WriteMarkerAtCount(PendingMarkerCount, FrameMs, FpsSmooth, FpsInstant);
        bHasPendingMarker = false;
    }

    // Auto-stop when max count is reached
    if (CurrentSpawnCount >= MaxSpawnCount && !bStopRequested)
    {
        RequestStop(TEXT("max_spawn_count"), FpsSmooth, FpsInstant, FrameMs);
    }

    // Handle delayed stop
    if (bStopRequested)
    {
        if (bAutoStopWhenComplete && StopAtTime > 0.0 && FPlatformTime::Seconds() >= StopAtTime)
        {
            StopAtTime = 0.0;
            StopRun();
        }
    }
}

bool AResourceBenchmarkController::SpawnSphere(float FpsSmooth, float FpsInstant, float FrameMs)
{
    // Acquire one actor from the pool
    AActor* Actor = Pool.Acquire();
    if (!Actor)
    {
        RequestStop(TEXT("pool_exhausted"), FpsSmooth, FpsInstant, FrameMs);
        return false;
    }

    // Pick a random point inside a square spawn area above the controller
    const float SpawnX = RandomStream.FRandRange(-SpawnAreaHalfExtent, SpawnAreaHalfExtent);
    const float SpawnY = RandomStream.FRandRange(-SpawnAreaHalfExtent, SpawnAreaHalfExtent);

    const FVector SpawnPos =
        GetActorLocation() +
        FVector(SpawnX, SpawnY, SpawnHeightOffset);

    Actor->SetActorLocation(SpawnPos);
    Actor->SetActorHiddenInGame(false);
    Actor->SetActorTickEnabled(true);

    UPrimitiveComponent* Phys = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
    if (!Phys)
    {
        RequestStop(TEXT("spawn_failed"), FpsSmooth, FpsInstant, FrameMs);
        return false;
    }

    // Reactivate physics state
    Phys->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Phys->SetSimulatePhysics(true);
    Phys->SetPhysicsLinearVelocity(FVector::ZeroVector);
    Phys->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

    // Apply deterministic initial motion
    Phys->SetPhysicsLinearVelocity(RandomStream.GetUnitVector() * InitialSpeed);

    const float AngularDeg = AngularSpeed * (180.0f / PI);
    Phys->SetPhysicsAngularVelocityInDegrees(RandomStream.GetUnitVector() * AngularDeg);

    CurrentSpawnCount++;
    return true;
}

void AResourceBenchmarkController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Csv.Close();
    Super::EndPlay(EndPlayReason);
}

bool AResourceBenchmarkController::ShouldQueueMarker() const
{
    // Logging starts only after warm-up
    if (!bWarmupCompleted)
    {
        return false;
    }

    if (!bWriteCsv)
    {
        return false;
    }

    if (CurrentSpawnCount <= 0)
    {
        return false;
    }

    if (LogEveryNSpawns <= 0)
    {
        return false;
    }

    if ((CurrentSpawnCount % LogEveryNSpawns) != 0)
    {
        return false;
    }

    // Avoid duplicate row after warm-up snap
    return CurrentSpawnCount != LastLoggedSpawnCount;
}

void AResourceBenchmarkController::WriteMarkerAtCount(
    int32 SpawnedCount,
    float FrameMs,
    float FpsSmooth,
    float FpsInstant)
{
    const double TimeS = FPlatformTime::Seconds() - StartTime;

    if (bWriteCsv)
    {
        Csv.WriteMarker(TimeS, SpawnedCount, FrameMs, FpsSmooth, FpsInstant);
    }

    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        if (ABenchmarkHUD* HUD = Cast<ABenchmarkHUD>(PC->GetHUD()))
        {
            const FString Msg = FString::Printf(
                TEXT("[%s] t=%.1fs count=%d fps~%.0f frame~%.1fms"),
                LogPrefix,
                TimeS,
                SpawnedCount,
                FpsSmooth,
                FrameMs
            );

            HUD->PushLine(Msg);
        }
    }
}

void AResourceBenchmarkController::RequestStop(
    const FString& InStopReason,
    float FpsSmooth,
    float FpsInstant,
    float FrameMs)
{
    if (bStopRequested)
    {
        return;
    }

    bStopRequested = true;
    StopReason = InStopReason;

    // Capture final benchmark state at stop request time
    FinalStopTimeS = FPlatformTime::Seconds() - StartTime;

    const double TotalTime = FinalStopTimeS - BenchmarkStartTimeS;
    AverageFps = TotalTime > 0.0 ? static_cast<float>(FrameCount / TotalTime) : 0.0f;

    FinalFpsSmooth = FpsSmooth;
    FinalFpsInstant = FpsInstant;
    FinalFrameTimeMs = FrameMs;

    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        if (ABenchmarkHUD* HUD = Cast<ABenchmarkHUD>(PC->GetHUD()))
        {
            const FString Msg = FString::Printf(
                TEXT("[%s] Stop requested: %s (t=%.2fs, count=%d)."),
                LogPrefix,
                *StopReason,
                FinalStopTimeS,
                CurrentSpawnCount
            );

            HUD->PushLine(Msg);
        }
    }

    if (bAutoStopWhenComplete)
    {
        StopAtTime = FPlatformTime::Seconds() + StopDelaySeconds;
    }
    else
    {
        StopRun();
    }
}

void AResourceBenchmarkController::StopRun()
{
    // Write final run information before stopping
    if (bWriteCsv)
    {
        Csv.WriteRunInfo(
            RandomSeed,
            StopReason,
            FinalStopTimeS,
            CurrentSpawnCount,
            AverageFps,
            FinalFpsSmooth,
            FinalFpsInstant,
            FinalFrameTimeMs
        );

        Csv.Close();
    }

    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
    }
}