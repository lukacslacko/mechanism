#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GPUProfiler.h"
#include "HallQualityGovernor.h"
#include "HallVRResolutionGovernor.h"
#include "MaterialHall.generated.h"

class ACameraActor;
class AStaticMeshActor;
class UMaterialInstanceDynamic;
class SWidget;
class FJsonValue;
class AHallVRPawn;

UCLASS()
class AMaterialHallController : public APlayerController
{
    GENERATED_BODY()
public:
    AMaterialHallController();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    friend class AHallVRPawn;
    void BuildInterface();
    void BuildVRInterface();
    bool IsVRActive() const;
    void TickVRTest();
    void SetView(int32 Index);
    void ApplyQuality(int32 Index);
    void SelectQuality(int32 Index);
    void SetAutoTune(bool bEnabled);
    void ApplyPerformanceSettings();
    void SavePerformanceSettings();
    void TickPerformance(float DeltaSeconds);
    void TickPerformanceTest();
    float GetRenderPercentage() const;
    FString GetPerformanceStatus() const;
    void UpdateCamera();
    void SetCameraPose(const FVector& Position, const FVector& Target, float FOV);
    void Navigate(float Forward, float Right, float Up, float Yaw, float Pitch, float DeltaSeconds, float SpeedMultiplier);
    void SetFinish(float Value);
    void ToggleMotion();
    void ResetScene();
    void TickSmokeTest();
    void Capture(const FString& Name);
    void FinishSmokeTest();
    void CheckAssembly();
    void TickNavigationTest();
    UPROPERTY() TObjectPtr<ACameraActor> ViewCamera;
    UPROPERTY() TObjectPtr<AHallVRPawn> VRPawn;
    UPROPERTY() TObjectPtr<AStaticMeshActor> BrassRod;
    UPROPERTY() TObjectPtr<AStaticMeshActor> SteelPlate;
    UPROPERTY() TObjectPtr<AStaticMeshActor> PivotPin;
    UPROPERTY() TObjectPtr<AStaticMeshActor> PivotCollar;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> Metals;
    TSharedPtr<SWidget> Interface;
    FVector OrbitTarget = FVector(0,0,65);
    FVector BrassRestPosition = FVector::ZeroVector;
    float OrbitYaw = 55, OrbitPitch = 17, OrbitDistance = 615;
    int32 QualityIndex = 1;
    int32 ViewIndex = 0;
    bool bAnimateRod = true;
    float BrassRestYaw = 25;
    float MotionTime = 0;
    float SmoothedFrameTime = 1.f / 60.f;
    bool bAutoTune = true;
    bool bVRMode = false;
    bool bVRTest = false;
    bool bVRHardwareTest = false;
    bool bVRInputTestPassed = true;
    bool bVRSeenHead = false;
    bool bVRSeenControllers = false;
    bool bVRSeenFocus = false;
    float VRRefreshRate = 0;
    FHallVRResolutionGovernor VRResolutionGovernor;
    bool bVSync = true;
    int32 AntiAliasingMethod = 4;
    int32 ManualQualityIndex = 1;
    bool bPerformanceTest = false;
    bool bPerformanceTestPassed = true;
    bool bDynamicResolutionPaused = false;
    bool bHasGPUTiming = false;
    double GPUTimeMs = 0;
    double CPUTimeMs = 0;
    double LastGPUTimingTime = 0;
    double PerformanceFrameSum = 0;
    int32 PerformanceFrameCount = 0;
    float LowestTestPercentage = 100;
    float RecoveredTestPercentage = 50;
    FHallQualityGovernor QualityGovernor;
    FRHIGPUFrameTimeHistory::FState GPUTimingState;
    TArray<TSharedPtr<FJsonValue>> PerformanceResults;
    bool bSmokeTest = false;
    bool bTestFinished = false;
    bool bNavigationTestPassed = true;
    int32 NavigationTestIndex = -1;
    double NavigationStepStart = 0;
    FVector NavigationStartPosition;
    FRotator NavigationStartRotation;
    TArray<TSharedPtr<FJsonValue>> NavigationResults;
    int32 TestStage = 0;
    double TestStart = 0;
    double RotationTravelDegrees = 0;
    double MaxPivotDriftCm = 0;
    double PausedYaw = 0;
    bool bPauseResumePassed = true;
    bool bResetPassed = false;
    int32 MotionFrames = 0;
    bool bAssemblyPassed = true;
    double MaxSupportErrorCm = 0;
    int32 AssemblyFrames = 0;
    FString CaptureDirectory;
};

UCLASS()
class AMaterialHallGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AMaterialHallGameMode();
};
