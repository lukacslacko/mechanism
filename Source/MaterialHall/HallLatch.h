#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HallLatchDynamics.h"
#include "HallLatch.generated.h"

class AHallAndGate;
class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;
class UMaterialInterface;

UCLASS()
class AHallLatch : public AActor
{
    GENERATED_BODY()
public:
    AHallLatch();
    virtual void BeginPlay() override;
    virtual void Tick(float DT) override;
    void SetControls(bool SBar,bool RBar);
    void Pulse(int32 Index);
    void ToggleControl(int32 Index);
    void ToggleCycle();
    void ResetDemo();
    void SetDemoVisible(bool bVisible);
    void SetFinish(float Roughness);
    bool IsCycling() const { return bCycle; }
    FString Status() const;
    const FHallLatchDynamics& State() const { return Dynamics; }
    bool CheckAssembly(double& JointError,double& ConnectionError) const;
private:
    UStaticMeshComponent* Part(const FString& Name,const TCHAR* Mesh,const TCHAR* Finish,FVector Position,FVector Scale=FVector::OneVector);
    UTextRenderComponent* Label(const FString& Name,const FString& Text,FVector Position,float Size=3.5f);
    void UpdateAssembly();
    FVector GatePoint(int32 Index,FVector Local) const;
    UPROPERTY() TArray<TObjectPtr<AHallAndGate>> Gates;
    UPROPERTY() TMap<FName,TObjectPtr<UStaticMesh>> Meshes;
    UPROPERTY() TMap<FName,TObjectPtr<UMaterialInterface>> Finishes;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Springs;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Cores;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Collars;
    UPROPERTY() TArray<TObjectPtr<UTextRenderComponent>> StateLabels;
    FHallLatchDynamics Dynamics;
    double PulseTime=0;
    double CycleTime=0;
    bool bCycle=false;
    int32 CycleStep=0;
};
