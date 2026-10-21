#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "HallVRPawn.generated.h"

class UCameraComponent;
class UMotionControllerComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class UWidgetInteractionComponent;
class SWidget;
class AMaterialHallController;

UCLASS()
class AHallVRPawn : public APawn
{
    GENERATED_BODY()
public:
    AHallVRPawn();
    void Initialize(bool bSimulated);
    void UpdateInput(AMaterialHallController& Player, bool bActive);
    void SetPanel(const TSharedPtr<SWidget>& Widget);
    void PlacePanel();
    void TogglePanel();
    void SetStation(int32 Index);
    void SnapTurn(float Degrees);
    bool IsRigValid() const;
    bool AreControllersTracked() const;
    UCameraComponent* GetVRCamera() const { return Camera; }
    int32 GetStation() const { return Station; }
    bool IsPanelVisible() const;
    UWidgetComponent* GetPanel() const { return Panel; }
private:
    UPROPERTY() TObjectPtr<UCameraComponent> Camera;
    UPROPERTY() TObjectPtr<UMotionControllerComponent> LeftGrip;
    UPROPERTY() TObjectPtr<UMotionControllerComponent> RightGrip;
    UPROPERTY() TObjectPtr<UMotionControllerComponent> RightAim;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> LeftHand;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> RightHand;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Laser;
    UPROPERTY() TObjectPtr<UWidgetInteractionComponent> Pointer;
    UPROPERTY() TObjectPtr<UWidgetComponent> Panel;
    bool bTriggerHeld = false;
    bool bInitialPosePending = true;
    bool bStickReady = true;
    int32 Station = 0;
};
