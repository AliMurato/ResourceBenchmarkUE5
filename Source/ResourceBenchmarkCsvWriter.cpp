#include "ResourceBenchmarkCsvWriter.h"

#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformFile.h"
#include "HAL/PlatformProperties.h"
#include "HAL/PlatformProcess.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "GenericPlatform/GenericPlatformMisc.h"
#include "GenericPlatform/GenericPlatformMemory.h"

namespace
{
    const TCHAR* CsvEngineVersion = TEXT("UE_5.7.3");
}

/*
 * Initialize
 *
 * Creates benchmark output directory and opens the marker CSV file.
 * In Editor, files are written into the project Saved folder.
 * In packaged build, files are written next to the build executable.
 * A numeric suffix is added to avoid overwriting previous runs.
 */
void FResourceBenchmarkCsvWriter::Initialize()
{
#if WITH_EDITOR
    const FString Dir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("BenchmarkResults"));
#else
    const FString BaseDir = FPlatformProcess::BaseDir();
    const FString Dir = FPaths::Combine(BaseDir, TEXT("BenchmarkResults"));
#endif

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    PlatformFile.CreateDirectoryTree(*Dir);

    const int32 Index = GetNextRunIndex(Dir, TEXT("resource_benchmark_markers"));
    const FString Id = FString::Printf(TEXT("%03d"), Index);

    MarkersFilePath = Dir / FString::Printf(TEXT("resource_benchmark_markers_%s.csv"), *Id);
    RunInfoFilePath = Dir / FString::Printf(TEXT("resource_benchmark_runinfo_%s.csv"), *Id);

    MarkerFile.Reset(PlatformFile.OpenWrite(*MarkersFilePath));

    if (MarkerFile)
    {
        const FString Header =
            TEXT("time_s,spawned_objects,frame_time_ms,fps_smooth,fps_instant\n");

        FTCHARToUTF8 Convert(*Header);
        MarkerFile->Write((uint8*)Convert.Get(), Convert.Length());
    }
}

/*
 * WriteRunInfo
 *
 * Writes one static metadata row describing the completed run.
 */
void FResourceBenchmarkCsvWriter::WriteRunInfo(
    int32 RandomSeed,
    const FString& StopReason,
    double StopTimeS,
    int32 FinalSpawnCount,
    float AvgFps,
    float FinalFpsSmooth,
    float FinalFpsInstant,
    float FinalFrameTimeMs
)
{
    const FString Header =
        TEXT("engine_version,platform,gpu,cpu,sys_ram_mb,spawn_mode,random_seed,stop_reason,stop_time_s,final_spawn_count,avg_fps,final_fps_smooth,final_fps_instant,final_frame_time_ms\n");

    const FString GPU = FPlatformMisc::GetPrimaryGPUBrand();
    const FString CPU = FPlatformMisc::GetCPUBrand();

    const uint32 RAM =
        (uint32)(FPlatformMemory::GetConstants().TotalPhysical / (1024ull * 1024ull));

    const FString PlatformName =
        ANSI_TO_TCHAR(FPlatformProperties::PlatformName());

    const FString Data = FString::Printf(
        TEXT("%s,%s,\"%s\",\"%s\",%u,Pool,%d,%s,%.3f,%d,%.2f,%.2f,%.2f,%.3f\n"),
        CsvEngineVersion,
        *PlatformName,
        *GPU,
        *CPU,
        RAM,
        RandomSeed,
        *StopReason,
        StopTimeS,
        FinalSpawnCount,
        AvgFps,
        FinalFpsSmooth,
        FinalFpsInstant,
        FinalFrameTimeMs
    );

    FFileHelper::SaveStringToFile(Header + Data, *RunInfoFilePath);
}

/*
 * WriteMarker
 *
 * Appends one performance sample to the marker CSV file.
 * The marker file remains open to minimize I/O overhead.
 */
void FResourceBenchmarkCsvWriter::WriteMarker(
    double TimeS,
    int32 Count,
    float FrameMs,
    float FpsSmooth,
    float FpsInstant)
{
    if (!MarkerFile)
    {
        return;
    }

    const FString Row = FString::Printf(
        TEXT("%.3f,%d,%.3f,%.2f,%.2f\n"),
        TimeS,
        Count,
        FrameMs,
        FpsSmooth,
        FpsInstant
    );

    FTCHARToUTF8 Convert(*Row);
    MarkerFile->Write((uint8*)Convert.Get(), Convert.Length());
}

/*
 * Close
 *
 * Flushes buffered writes and releases the file handle.
 */
void FResourceBenchmarkCsvWriter::Close()
{
    if (MarkerFile)
    {
        MarkerFile->Flush();
        MarkerFile.Reset();
    }
}

/*
 * GetNextRunIndex
 *
 * Finds the first free numeric suffix for the given base file name.
 */
int32 FResourceBenchmarkCsvWriter::GetNextRunIndex(
    const FString& Directory,
    const FString& BaseName)
{
    int32 Index = 1;

    while (true)
    {
        FString Test = Directory / FString::Printf(TEXT("%s_%03d.csv"), *BaseName, Index);

        if (!FPaths::FileExists(Test))
        {
            return Index;
        }

        Index++;
    }
}