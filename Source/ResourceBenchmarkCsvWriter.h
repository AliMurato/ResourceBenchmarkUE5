#pragma once

#include "CoreMinimal.h"

class IFileHandle;

/*
 * CSV writer used by the benchmark.
 *
 * Responsibilities:
 * - create output files
 * - write static run information
 * - append marker rows during the run
 * - keep the marker file open to reduce I/O overhead
 */
class FResourceBenchmarkCsvWriter
{
public:

    /* Creates marker and runinfo files for a new run */
    void Initialize();

    /* Writes one static metadata row describing the completed run */
    void WriteRunInfo(
        int32 RandomSeed,
        const FString& StopReason,
        double StopTimeS,
        int32 FinalSpawnCount,
        float AvgFps,
        float FinalFpsSmooth,
        float FinalFpsInstant,
        float FinalFrameTimeMs
    );

    /* Writes one time-series marker row */
    void WriteMarker(
        double TimeS,
        int32 Count,
        float FrameMs,
        float FpsSmooth,
        float FpsInstant
    );

    /* Flushes and closes the marker file */
    void Close();

private:

    /* Path to marker CSV */
    FString MarkersFilePath;

    /* Path to run info CSV */
    FString RunInfoFilePath;

    /* Open file handle for buffered marker writes */
    TUniquePtr<IFileHandle> MarkerFile;

    /* Finds the next free numeric index for file naming */
    int32 GetNextRunIndex(const FString& Directory, const FString& BaseName);
};