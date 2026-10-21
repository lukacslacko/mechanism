#include "HallVRPawn.h"
#include "MaterialHall.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "Engine/StaticMesh.h"
#include "MotionControllerComponent.h"
#include "Materials/MaterialInterface.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "UObject/ConstructorHelpers.h"

AHallVRPawn::AHallVRPawn()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("TrackingOrigin"));
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Head"));
    Camera->SetupAttachment(RootComponent);
    Camera->bLockToHmd = true;
    Camera->bUsePawnControlRotation = false;
    Camera->bConstrainAspectRatio = false;
    LeftGrip = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftGrip"));
    RightGrip = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightGrip"));
    RightAim = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightAim"));
    LeftGrip->SetupAttachment(RootComponent);
    RightGrip->SetupAttachment(RootComponent);
    RightAim->SetupAttachment(RootComponent);
    LeftGrip->SetTrackingMotionSource(TEXT("LeftGrip"));
    RightGrip->SetTrackingMotionSource(TEXT("RightGrip"));
    RightAim->SetTrackingMotionSource(TEXT("RightAim"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    LeftHand = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftHand"));
    RightHand = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightHand"));
    LeftHand->SetupAttachment(LeftGrip);
    RightHand->SetupAttachment(RightGrip);
    for (UStaticMeshComponent* Hand : { LeftHand.Get(), RightHand.Get() })
    {
        Hand->SetStaticMesh(Sphere.Object);
        Hand->SetRelativeScale3D(FVector(.09, .055, .055));
        Hand->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Hand->SetCastShadow(false);
    }
    Laser = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PointerBeam"));
    Laser->SetupAttachment(RightAim);
    Laser->SetStaticMesh(Cube.Object);
    Laser->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Laser->SetCastShadow(false);
    Pointer = CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("UIInteraction"));
    Pointer->SetupAttachment(RightAim);
    Pointer->InteractionDistance = 400;
    Pointer->InteractionSource = EWidgetInteractionSource::World;
    Pointer->TraceChannel = ECC_Visibility;
    Pointer->VirtualUserIndex = 1;
    Pointer->PointerIndex = 0;
    Panel = CreateDefaultSubobject<UWidgetComponent>(TEXT("ControlPanel"));
    Panel->SetupAttachment(RootComponent);
    // World-anchored: looking around never drags the panel across the eyes.
    Panel->SetAbsolute(true, true, true);
    Panel->SetWidgetSpace(EWidgetSpace::World);
    Panel->SetDrawSize(FVector2D(900, 980));
    Panel->SetPivot(FVector2D(.5, .5));
    Panel->SetTwoSided(true);
    Panel->SetWindowFocusable(false);
    Panel->SetTickWhenOffscreen(true);
    Panel->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Panel->SetCollisionResponseToAllChannels(ECR_Ignore);
    Panel->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Panel->SetCastShadow(false);
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> PanelMaterial(TEXT("/Game/MaterialHall/Materials/VRWidget.VRWidget"));
    Panel->SetMaterial(0, PanelMaterial.Object);
}

void AHallVRPawn::Initialize(bool bSimulated)
{
    bInitialPosePending = !bSimulated;
    UHeadMountedDisplayFunctionLibrary::SetWorldToMetersScale(this, 100);
    UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Stage);
    if (bSimulated)
    {
        Camera->bLockToHmd = false;
        Camera->SetRelativeLocation(FVector(0, 0, 165));
    }
    SetStation(0);
}

void AHallVRPawn::SetStation(int32 Index)
{
    // Floor positions outside the 250 x 132 cm tabletop. No artificial pitch,
    // vertical flight, or smooth acceleration; physical movement stays 1:1.
    static const FVector Positions[] = { {0, 150, 0}, {205, 0, 0}, {0, -150, 0}, {-205, 0, 0} };
    Station = (Index % 4 + 4) % 4;
    const FVector Target = Positions[Station];
    const float Facing = (-Target).Rotation().Yaw;
    SetActorRotation(FRotator(0, Facing - Camera->GetRelativeRotation().Yaw, 0));
    const FVector HeadOffset = Camera->GetComponentLocation() - GetActorLocation();
    SetActorLocation(Target - FVector(HeadOffset.X, HeadOffset.Y, 0));
    PlacePanel();
}

void AHallVRPawn::SnapTurn(float Degrees)
{
    const FVector Before = Camera->GetComponentLocation();
    AddActorWorldRotation(FRotator(0, Degrees, 0));
    const FVector After = Camera->GetComponentLocation();
    AddActorWorldOffset(FVector(Before.X - After.X, Before.Y - After.Y, 0));
}

void AHallVRPawn::SetPanel(const TSharedPtr<SWidget>& Widget)
{
    Panel->SetSlateWidget(Widget);
    PlacePanel();
}

void AHallVRPawn::PlacePanel()
{
    const FRotator Yaw(0, Camera->GetComponentRotation().Yaw, 0);
    const FVector Head = Camera->GetComponentLocation();
    const FVector Position = Head + Yaw.RotateVector(FVector(100, 95, -5));
    Panel->SetWorldLocationAndRotation(Position, FRotator(0, (Head - Position).Rotation().Yaw, 0));
    Panel->SetWorldScale3D(FVector(.085));
}

void AHallVRPawn::TogglePanel()
{
    const bool bVisible = !Panel->IsVisible();
    if (bVisible) PlacePanel();
    Panel->SetVisibility(bVisible);
    Panel->SetCollisionEnabled(bVisible ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

bool AHallVRPawn::IsPanelVisible() const { return Panel->IsVisible(); }
bool AHallVRPawn::AreControllersTracked() const { return LeftGrip->IsTracked() && RightGrip->IsTracked() && RightAim->IsTracked(); }
bool AHallVRPawn::IsRigValid() const
{
    return Camera && Camera->GetAttachParent() == RootComponent && !Camera->bUsePawnControlRotation &&
        LeftGrip->GetTrackingMotionSource() == TEXT("LeftGrip") && RightGrip->GetTrackingMotionSource() == TEXT("RightGrip") &&
        RightAim->GetTrackingMotionSource() == TEXT("RightAim") && Panel->GetDrawSize() == FVector2D(900, 980) &&
        Pointer->InteractionSource == EWidgetInteractionSource::World;
}

void AHallVRPawn::UpdateInput(AMaterialHallController& Player, bool bActive)
{
    // The tracked camera pose arrives after BeginPlay. Place the initial panel
    // and station only once that pose is available, rather than at floor height.
    if (bInitialPosePending && bActive && Camera->GetRelativeLocation().Z > 20)
    {
        SetStation(0);
        bInitialPosePending = false;
    }
    const bool bRightTracked = bActive && RightAim->IsTracked();
    LeftHand->SetVisibility(bActive && LeftGrip->IsTracked());
    RightHand->SetVisibility(bActive && RightGrip->IsTracked());
    Laser->SetVisibility(bRightTracked && IsPanelVisible());
    Pointer->SetActive(bRightTracked && IsPanelVisible());
    const bool bPressed = bRightTracked && IsPanelVisible() &&
        Player.GetInputAnalogKeyState(EKeys::OculusTouch_Right_Trigger_Axis) > .55f;
    if (bPressed && !bTriggerHeld) Pointer->PressPointerKey(EKeys::LeftMouseButton);
    if (!bPressed && bTriggerHeld) Pointer->ReleasePointerKey(EKeys::LeftMouseButton);
    bTriggerHeld = bPressed;
    const FHitResult& Hit = Pointer->GetLastHitResult();
    const float Length = Hit.bBlockingHit ? FMath::Min(400.f, Hit.Distance) : 400.f;
    Laser->SetRelativeLocation(FVector(Length * .5f, 0, 0));
    Laser->SetRelativeScale3D(FVector(Length / 100.f, .003f, .003f));
    if (!bActive) { bStickReady = false; return; }
    const float Stick = Player.GetInputAnalogKeyState(EKeys::OculusTouch_Right_Thumbstick_X);
    if (FMath::Abs(Stick) < .25f) bStickReady = true;
    if (bStickReady && FMath::Abs(Stick) > .7f)
    {
        SnapTurn(Stick > 0 ? 45 : -45);
        bStickReady = false;
    }
    if (Player.WasInputKeyJustPressed(EKeys::OculusTouch_Left_X_Click)) SetStation(Station + 1);
    if (Player.WasInputKeyJustPressed(EKeys::OculusTouch_Left_Y_Click)) TogglePanel();
    if (Player.WasInputKeyJustPressed(EKeys::OculusTouch_Right_A_Click)) Player.ToggleMotion();
    if (Player.WasInputKeyJustPressed(EKeys::OculusTouch_Right_B_Click)) Player.ResetScene();
}
