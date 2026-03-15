#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BenchmarkHUD.generated.h"

/*
 * Lightweight runtime HUD used by the benchmark.
 *
 * Responsibilities:
 *  - display benchmark progress (time, object count, FPS)
 *  - maintain a small rolling text buffer
 *  - render the buffer directly using Canvas
 *
 * Design goal:
 * avoid UMG/Slate overhead during performance measurement.
 */
UCLASS(Blueprintable)
class RESOURCEBENCHMARKUE5_API ABenchmarkHUD : public AHUD
{
    GENERATED_BODY()

public:

    ABenchmarkHUD();

    /* Called every frame to draw the HUD overlay */
    virtual void DrawHUD() override;

    /* Adds a line to the on-screen log buffer */
    void PushLine(const FString& InLine);

    /* Horizontal anchor for HUD placement (0..1 screen width) */
    UPROPERTY(EditAnywhere, Category = "Layout")
    float LeftAnchorX = 0.66f;

    /* Padding from screen edges */
    UPROPERTY(EditAnywhere, Category = "Layout")
    FVector2D Padding = FVector2D(16.f, 16.f);

    /* Inner padding between text and background box */
    UPROPERTY(EditAnywhere, Category = "Layout")
    FVector2D InnerPadding = FVector2D(8.f, 6.f);

    /* Maximum number of visible log lines */
    UPROPERTY(EditAnywhere, Category = "Log Buffer")
    int32 MaxLines = 18;

    /* Text scale applied to rendered log lines */
    UPROPERTY(EditAnywhere, Category = "Style")
    float TextScale = 0.85f;

    /* Vertical spacing between lines */
    UPROPERTY(EditAnywhere, Category = "Style")
    float LineHeight = 18.0f;

private:

    /* Rolling buffer storing the most recent log messages */
    TArray<FString> LogLines;

    /* Font used for HUD rendering */
    UPROPERTY()
    UFont* DebugFont;
};