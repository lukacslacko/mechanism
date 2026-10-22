#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HallAndKinematics.h"
#include "HallAndGate.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class UTextRenderComponent;
DECLARE_MULTICAST_DELEGATE_OneParam(FHallInputRequest, int32);

UCLASS()
class AHallAndGate : public AActor
{
    GENERATED_BODY()
public:
    AHallAndGate();
    virtual void BeginPlay() override;
    virtual void Tick(float DT) override;
    void SetInputs(bool A, bool B);
    void ToggleInput(int32 Index);
    void ToggleCycle();
    void ResetGate();
    void SetFinish(float Roughness);
    void SetDemoVisible(bool bVisible);
    bool ActivateHandle(UPrimitiveComponent* Component);
    bool InputTarget(int32 Index) const { return Targets[Index ? 1 : 0]; }
    bool IsCycling() const { return bCycle; }
    bool IsSettled() const;
    bool OutputBit() const { return Pose.OutputTravel > .5; }
    FHallAndPose GetPose() const { return Pose; }
    double GetInputPosition(int32 Index) const { return Motion[Index ? 1 : 0].Position; }
    bool CheckVisualConstraints(double& MaxError) const;
    // A latch drives both continuous input positions and owns the feedback loop.
    void ConfigureEmbedded() { bEmbedded=true; }
    void SetDrivenPose(double A, double B);
    void SetExternalColor(bool bOrange);
    FHallInputRequest OnInputRequested;
private:
    UStaticMeshComponent* Part(const TCHAR* Name, const TCHAR* Mesh, const TCHAR* Finish,
        FVector Location, FVector Scale=FVector::OneVector, USceneComponent* Parent=nullptr);
    UTextRenderComponent* Label(const TCHAR* Name, const FString& Text, FVector Location, float Size);
    USceneComponent* Joint(const TCHAR* Name, double Bottom, double Top);
    void UpdatePose();
    UFUNCTION() void HandleClicked(UPrimitiveComponent* Component, FKey Button);
    UPROPERTY() TMap<FName,TObjectPtr<UStaticMesh>> Meshes;
    UPROPERTY() TMap<FName,TObjectPtr<UMaterialInterface>> Finishes;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Sliders;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Handles;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Links;
    UPROPERTY() TArray<TObjectPtr<USceneComponent>> Joints;
    UPROPERTY() TArray<TObjectPtr<UTextRenderComponent>> StateLabels;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> TruthMarkers;
    FHallRodMotion Motion[2];
    FHallAndPose Pose;
    bool Targets[2] = { false,false };
    bool bCycle = false;
    bool bDemoVisible = true;
    bool bEmbedded = false;
    double CycleTime = 0;
    int32 CycleIndex = 0;
};
