#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "MaterialHall.generated.h"

class ACameraActor;
class AStaticMeshActor;
class UMaterialInstanceDynamic;
class SWidget;
class FJsonValue;

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
    void BuildInterface();
    void SetView(int32 Index);
    void ApplyQuality(int32 Index);
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
    float SmoothedFrameTime = 1.f / 30.f;
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
