#include "BenchmarkHUD.h"
#include "Engine/Canvas.h"
#include "UObject/ConstructorHelpers.h"
#include "CanvasItem.h"
#include "Engine/Font.h"

/*
 * Constructor
 *
 * Loads a lightweight built-in engine font.
 * Using a fixed engine font avoids asset dependencies
 * and guarantees availability in packaged builds.
 */
ABenchmarkHUD::ABenchmarkHUD()
{
    static ConstructorHelpers::FObjectFinder<UFont> FontObj(TEXT("/Engine/EngineFonts/Roboto"));

    if (FontObj.Succeeded())
    {
        DebugFont = FontObj.Object;
    }
}

/*
 * PushLine
 *
 * Adds a message to the HUD log buffer.
 * The buffer behaves as a rolling queue:
 * when the limit is exceeded, the oldest line is removed.
 */
void ABenchmarkHUD::PushLine(const FString& InLine)
{
    if (InLine.IsEmpty())
    {
        return;
    }

    LogLines.Add(InLine);

    // Keep only the most recent lines without shrinking the array
    if (LogLines.Num() > MaxLines)
    {
        LogLines.RemoveAt(0, 1, EAllowShrinking::No);
    }
}

/*
 * DrawHUD
 *
 * Main rendering routine executed every frame.
 *
 * Rendering steps:
 *  1. Determine widest text line
 *  2. Compute background box dimensions
 *  3. Clamp box inside the screen
 *  4. Draw semi-transparent background
 *  5. Render each buffered log line
 */
void ABenchmarkHUD::DrawHUD()
{
    Super::DrawHUD();

    // Skip drawing if prerequisites are missing
    if (!Canvas || !DebugFont)
    {
        return;
    }

    const int32 LineCount = LogLines.Num();
    if (LineCount == 0)
    {
        return;
    }

    // Preferred HUD position on the right side of the screen
    const float PreferredStartX = Canvas->SizeX * LeftAnchorX + Padding.X;
    const float StartY = Padding.Y;

    /*
     * Measure the widest line to size the background box
     */
    float MaxTextWidth = 0.0f;

    for (const FString& Line : LogLines)
    {
        float RawWidth = 0.0f;
        float RawHeight = 0.0f;

        Canvas->StrLen(DebugFont, Line, RawWidth, RawHeight);

        const float ScaledWidth = RawWidth * TextScale;
        MaxTextWidth = FMath::Max(MaxTextWidth, ScaledWidth);
    }

    /*
     * Compute background box size
     */
    float BoxWidth = MaxTextWidth + InnerPadding.X * 2.0f;
    const float BoxHeight = LineCount * LineHeight + InnerPadding.Y * 2.0f;

    // Prevent HUD from exceeding screen width
    const float MaxAllowedWidth = Canvas->SizeX - Padding.X * 2.0f;
    BoxWidth = FMath::Min(BoxWidth, MaxAllowedWidth);

    /*
     * Adjust horizontal position if the box would go off-screen
     */
    float StartX = PreferredStartX;

    if (StartX + BoxWidth > Canvas->SizeX - Padding.X)
    {
        StartX = Canvas->SizeX - Padding.X - BoxWidth;
    }

    // Safety clamp
    StartX = FMath::Max(Padding.X, StartX);

    /*
     * Draw background rectangle behind the log text
     */
    const FLinearColor BgColor(0.0f, 0.0f, 0.0f, 0.5f);
    DrawRect(BgColor, StartX, StartY, BoxWidth, BoxHeight);

    /*
     * Draw each buffered log line
     */
    float CurrentY = StartY + InnerPadding.Y;
    const float TextX = StartX + InnerPadding.X;

    for (const FString& Line : LogLines)
    {
        FCanvasTextItem TextItem(
            FVector2D(TextX, CurrentY),
            FText::FromString(Line),
            DebugFont,
            FLinearColor::White
        );

        TextItem.Scale = FVector2D(TextScale, TextScale);
        TextItem.BlendMode = SE_BLEND_Translucent;

        Canvas->DrawItem(TextItem);
        CurrentY += LineHeight;
    }
}