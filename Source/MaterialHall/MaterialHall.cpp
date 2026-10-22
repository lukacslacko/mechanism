#include "MaterialHall.h"
#include "HallVRPawn.h"
#include "HallAndGate.h"
#include "HallLatch.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "InputKeyEventArgs.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformFileManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ConfigCacheIni.h"
#include "Serialization/JsonSerializer.h"
#include "Brushes/SlateColorBrush.h"
#include "Styling/CoreStyle.h"
#include "UnrealClient.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, MaterialHall, "MaterialHall");
DEFINE_LOG_CATEGORY_STATIC(LogMaterialHall, Log, All);

AMaterialHallGameMode::AMaterialHallGameMode()
{
    DefaultPawnClass = nullptr;
    PlayerControllerClass = AMaterialHallController::StaticClass();
    HUDClass = nullptr;
}

AMaterialHallController::AMaterialHallController()
{
    PrimaryActorTick.bCanEverTick = true;
    bShowMouseCursor = true;
    bAutoManageActiveCameraTarget = false;
}

void AMaterialHallController::BeginPlay()
{
    Super::BeginPlay();
    bSmokeTest = FParse::Param(FCommandLine::Get(), TEXT("HallSelfTest"));
    bPerformanceTest = FParse::Param(FCommandLine::Get(), TEXT("HallPerformanceTest"));
    bGateTest = FParse::Param(FCommandLine::Get(), TEXT("HallGateTest"));
    bLatchTest = FParse::Param(FCommandLine::Get(), TEXT("HallLatchTest"));
    bVRTest = FParse::Param(FCommandLine::Get(), TEXT("HallVRTest"));
    bVRHardwareTest = FParse::Param(FCommandLine::Get(), TEXT("HallVRHardwareTest"));
    bVRMode = bVRTest || bVRHardwareTest || GEngine->IsStereoscopic3D();
    if (FParse::Param(FCommandLine::Get(), TEXT("HallVR")) && !bVRMode)
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Quest Link is not active. Open Meta Horizon Link on the PC, enter Quest Link in the headset, then launch Mechanism Quest again.")));
        FGenericPlatformMisc::RequestExit(false);
        return;
    }
    if (bVRMode) AntiAliasingMethod = 2;
    if (!bSmokeTest && !bPerformanceTest && !bVRTest && !bVRHardwareTest && !bGateTest && !bLatchTest)
    {
        const TCHAR* Section = bVRMode ? TEXT("MaterialHall.VRPerformance") : TEXT("MaterialHall.Performance");
        GConfig->GetBool(Section, TEXT("AutoTune"), bAutoTune, GGameUserSettingsIni);
        GConfig->GetBool(Section, TEXT("VSync"), bVSync, GGameUserSettingsIni);
        GConfig->GetInt(Section, TEXT("AntiAliasingMethod"), AntiAliasingMethod, GGameUserSettingsIni);
        GConfig->GetInt(Section, TEXT("ManualQuality"), ManualQualityIndex, GGameUserSettingsIni);
        AntiAliasingMethod = AntiAliasingMethod == 2 ? 2 : 4;
        ManualQualityIndex = FMath::Clamp(ManualQualityIndex, 0, 2);
    }
    ViewCamera = GetWorld()->SpawnActor<ACameraActor>();
    ViewCamera->GetCameraComponent()->bConstrainAspectRatio = false;
    SetViewTarget(ViewCamera);
    if (bVRMode)
    {
        VRPawn = GetWorld()->SpawnActor<AHallVRPawn>();
        VRPawn->Initialize(bVRTest);
        Possess(VRPawn);
        SetViewTarget(VRPawn);
        bShowMouseCursor = false;
    }
    if (GEngine->GameViewport && GEngine->GameViewport->GetWindow().IsValid())
        GEngine->GameViewport->GetWindow()->SetTitle(FText::FromString(TEXT("Material Hall")));
    for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
    {
        UStaticMeshComponent* Mesh = It->GetStaticMeshComponent();
        if (!Mesh || !Mesh->GetMaterial(0)) continue;
        const FString MaterialName = Mesh->GetMaterial(0)->GetName();
        if (MaterialName == TEXT("Brass") || MaterialName == TEXT("BrushedSteel"))
        {
            Metals.Add(Mesh->CreateDynamicMaterialInstance(0));
            const FString MeshName = Mesh->GetStaticMesh()->GetName();
            if (MeshName == TEXT("BrassPointer"))
            {
                BrassRod = *It;
                BrassRestPosition = It->GetActorLocation();
                BrassRestYaw = It->GetActorRotation().Yaw;
                Mesh->SetMobility(EComponentMobility::Movable);
            }
            else if (MeshName == TEXT("SteelPlate")) SteelPlate = *It;
            else if (MeshName == TEXT("PivotPin")) PivotPin = *It;
            else if (MeshName == TEXT("PivotCollar")) PivotCollar = *It;
        }
    }
    AndGate = GetWorld()->SpawnActor<AHallAndGate>(FVector(0,0,82),FRotator::ZeroRotator);
    Latch = GetWorld()->SpawnActor<AHallLatch>(FVector(0,0,82),FRotator::ZeroRotator);
    bEnableClickEvents = true;
    SelectDemo(!bSmokeTest && !bPerformanceTest && !bVRTest);
    if (!bVRMode && !bSmokeTest && !bPerformanceTest && !bGateTest) SelectLatchDemo();
    ApplyQuality(bAutoTune ? 1 : ManualQualityIndex);
    FInputModeGameAndUI Mode;
    Mode.SetHideCursorDuringCapture(false);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(Mode);
    if (bVRMode) SetInputMode(FInputModeGameOnly());
    BuildInterface();
    FParse::Value(FCommandLine::Get(), TEXT("HallCaptureDir="), CaptureDirectory);
    if (CaptureDirectory.IsEmpty()) CaptureDirectory = FPaths::ProjectSavedDir() / TEXT("PivotValidation");
    TestStart = FPlatformTime::Seconds();
    UE_LOG(LogMaterialHall, Display, TEXT("MATERIAL HALL RUNTIME READY: metals=%d brass=%s; compiled native controls"), Metals.Num(), BrassRod ? TEXT("yes") : TEXT("no"));
}

void AMaterialHallController::SetView(int32 Index)
{
    if (bVRMode && VRPawn) { VRPawn->SetStation(Index); return; }
    ViewIndex = Index;
    FVector Position;
    float FOV;
    if (bGateDemo || bLatchDemo)
    {
        if (Index==1) { OrbitTarget=FVector(-20,5,93); Position=OrbitTarget+FVector(120,170,195); FOV=55; }
        else if (Index==2) { OrbitTarget=FVector(0,0,200); Position=FVector(650,1100,450); FOV=64; }
        else if (Index==3) { OrbitTarget=FVector(-40,0,86); Position=OrbitTarget+FVector(0,.01,300); FOV=65; }
        else { OrbitTarget=FVector(-42,30,89); Position=OrbitTarget+FVector(190,280,235); FOV=53; }
        SetCameraPose(Position,OrbitTarget,FOV);
        return;
    }
    if (Index == 1) { Position = FVector(140,185,185); OrbitTarget = FVector(0,0,84); FOV=40; }
    else if (Index == 3) { Position=FVector(24,32,115); OrbitTarget=FVector(0,0,88); FOV=43; }
    else if (Index == 2) { Position=FVector(650,1100,400); OrbitTarget=FVector(0,0,300); FOV=64; }
    else { Position=FVector(340,480,240); OrbitTarget=FVector(0,0,65); FOV=43; }
    SetCameraPose(Position,OrbitTarget,FOV);
}

void AMaterialHallController::SetCameraPose(const FVector& Position, const FVector& Target, float FOV)
{
    OrbitTarget=Target;
    const FVector Offset = Position - OrbitTarget;
    const FRotator Angles = Offset.Rotation();
    OrbitYaw=Angles.Yaw; OrbitPitch=Angles.Pitch; OrbitDistance=Offset.Size();
    if (ViewCamera) ViewCamera->GetCameraComponent()->SetFieldOfView(FOV);
    UpdateCamera();
}

void AMaterialHallController::Navigate(float Forward, float Right, float Up, float Yaw, float Pitch, float DeltaSeconds, float SpeedMultiplier)
{
    if (!ViewCamera || (Forward==0 && Right==0 && Up==0 && Yaw==0 && Pitch==0)) return;
    const float DT=FMath::Min(DeltaSeconds,.1f);
    FRotator Look=ViewCamera->GetActorRotation();
    Look.Yaw=FRotator::NormalizeAxis(Look.Yaw+Yaw*65.f*DT);
    Look.Pitch=FMath::Clamp(Look.Pitch+Pitch*65.f*DT,-89.f,89.f);
    Look.Roll=0;
    const FVector Direction=(Look.Vector()*Forward+FRotationMatrix(Look).GetUnitAxis(EAxis::Y)*Right+FVector::UpVector*Up).GetClampedToMaxSize(1);
    const FVector Position=ViewCamera->GetActorLocation()+Direction*120.f*SpeedMultiplier*DT;
    ViewCamera->SetActorLocationAndRotation(Position,Look);
    // Keep orbit and scroll coherent after flying; keyboard look pivots in place.
    OrbitTarget=Position+Look.Vector()*OrbitDistance;
    const FRotator Orbit=(-Look.Vector()).Rotation();
    OrbitYaw=Orbit.Yaw; OrbitPitch=Orbit.Pitch;
}

void AMaterialHallController::UpdateCamera()
{
    if (!ViewCamera) return;
    const FVector Location=OrbitTarget+FRotator(OrbitPitch,OrbitYaw,0).Vector()*OrbitDistance;
    ViewCamera->SetActorLocationAndRotation(Location,(OrbitTarget-Location).Rotation());
}

static void SetHallCVar(const TCHAR* Name, float Value)
{
    if (IConsoleVariable* Variable=IConsoleManager::Get().FindConsoleVariable(Name))
        Variable->Set(Value, ECVF_SetByCode);
}

void AMaterialHallController::ApplyQuality(int32 Index)
{
    QualityIndex=FMath::Clamp(Index,0,2);
    if (UGameUserSettings* Settings=GEngine->GetGameUserSettings())
    {
        Settings->SetOverallScalabilityLevel(2);
        Settings->SetAntiAliasingQuality(3);
        Settings->SetTextureQuality(3);
        Settings->SetViewDistanceQuality(Index==0 ? 1 : 2);
        Settings->SetShadowQuality(Index==0 ? 1 : Index==2 ? 3 : 2);
        Settings->SetVisualEffectQuality(Index==0 ? 1 : 2);
        Settings->SetGlobalIlluminationQuality(Index==2 ? 3 : 2);
        Settings->SetReflectionQuality(Index==0 ? 2 : 3);
        Settings->SetFrameRateLimit(bVRMode ? 0 : 60);
        Settings->SetVSyncEnabled(!bVRMode && bVSync);
        Settings->SetDynamicResolutionEnabled(!bVRMode && bAutoTune);
        Settings->ApplyNonResolutionSettings();
    }
    SetHallCVar(TEXT("r.ScreenPercentage"), bVRMode ? (bAutoTune ? VRResolutionGovernor.Percentage : Index==0 ? 60 : Index==1 ? 80 : 100) : Index==0 ? 50 : Index==1 ? 65 : 85);
    SetHallCVar(TEXT("r.Shadow.Virtual.SMRT.RayCountLocal"), Index==2 ? 8 : 4);
    SetHallCVar(TEXT("r.Shadow.Virtual.SMRT.SamplesPerRayLocal"), Index==2 ? 8 : 4);
    SetHallCVar(TEXT("r.Lumen.ScreenProbeGather.Temporal.MaxFramesAccumulated"), 8);
    SetHallCVar(TEXT("r.Lumen.TraceMeshSDFs"), 1);
    SetHallCVar(TEXT("r.Lumen.TraceMeshSDFs.Allow"), 1);
    SetHallCVar(TEXT("r.LumenScene.SurfaceCache.CardMinResolution"), 2);
    SetHallCVar(TEXT("r.Lumen.ScreenProbeGather.DownsampleFactor"), Index==0 ? 32 : 16);
    ApplyPerformanceSettings();
    UE_LOG(LogMaterialHall, Display, TEXT("QUALITY PROFILE: %d"), QualityIndex);
}

void AMaterialHallController::SetFinish(float Value)
{
    if (AndGate) AndGate->SetFinish(FMath::Lerp(.08f,.55f,Value));
    if (Latch) Latch->SetFinish(FMath::Lerp(.08f,.55f,Value));
    for (UMaterialInstanceDynamic* Material : Metals)
        if (Material) Material->SetScalarParameterValue(TEXT("Roughness"),FMath::Lerp(.08f,.55f,Value));
}

void AMaterialHallController::ToggleMotion()
{
    if (bLatchDemo && Latch) Latch->ToggleCycle();
    else if (bGateDemo && AndGate) AndGate->ToggleCycle();
    else bAnimateRod=!bAnimateRod;
}

void AMaterialHallController::ResetScene()
{
    if (bLatchDemo && Latch) { Latch->ResetDemo(); SetView(0); return; }
    if (bGateDemo && AndGate)
    {
        AndGate->ResetGate();
        if (!bVRMode) SetView(0);
        return;
    }
    bAnimateRod=true; MotionTime=0;
    if (BrassRod) BrassRod->SetActorLocationAndRotation(BrassRestPosition,FRotator(0,BrassRestYaw,0));
    if (!bVRMode) SetView(0);
}

void AMaterialHallController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    SmoothedFrameTime=FMath::Lerp(SmoothedFrameTime,DeltaSeconds,.05f);
    TickPerformance(DeltaSeconds);
    if (bVRMode && VRPawn) VRPawn->UpdateInput(*this, IsVRActive());
    if (!bVRMode)
    {
        bool bOrbitChanged=false;
        if (IsInputKeyDown(EKeys::RightMouseButton))
        {
            float DX=0,DY=0; GetInputMouseDelta(DX,DY);
            OrbitYaw-=DX*.25f; OrbitPitch=FMath::Clamp(OrbitPitch+DY*.25f,-89.f,89.f);
            bOrbitChanged=true;
        }
        if (WasInputKeyJustPressed(EKeys::MouseScrollUp)) { OrbitDistance=FMath::Max(20.f,OrbitDistance*.9f); bOrbitChanged=true; }
        if (WasInputKeyJustPressed(EKeys::MouseScrollDown)) { OrbitDistance=FMath::Min(3000.f,OrbitDistance/0.9f); bOrbitChanged=true; }
        if (bOrbitChanged) UpdateCamera();
        if (WasInputKeyJustPressed(EKeys::One)) SetView(0);
        if (WasInputKeyJustPressed(EKeys::Two)) SetView(1);
        if (WasInputKeyJustPressed(EKeys::Three)) SetView(2);
        if (WasInputKeyJustPressed(EKeys::Four)) SetView(3);
        if (WasInputKeyJustPressed(EKeys::F)) SelectQuality((QualityIndex+1)%3);
        if (WasInputKeyJustPressed(EKeys::SpaceBar)) ToggleMotion();
        if (WasInputKeyJustPressed(EKeys::R)) ResetScene();
        if (WasInputKeyJustPressed(EKeys::Z)) ToggleGateInput(0);
        if (WasInputKeyJustPressed(EKeys::X)) ToggleGateInput(1);
        if (WasInputKeyJustPressed(EKeys::G)) { if (bLatchDemo) SelectDemo(true); else if (bGateDemo) SelectDemo(false); else SelectLatchDemo(); }
        auto Axis=[this](const FKey& Positive,const FKey& Negative) { return float(IsInputKeyDown(Positive))-float(IsInputKeyDown(Negative)); };
        Navigate(Axis(EKeys::W,EKeys::S),Axis(EKeys::D,EKeys::A),Axis(EKeys::E,EKeys::Q),
            Axis(EKeys::L,EKeys::J),Axis(EKeys::I,EKeys::K),DeltaSeconds,
            IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift) ? 3.f : 1.f);
    }
    if (WasInputKeyJustPressed(EKeys::Escape)) FGenericPlatformMisc::RequestExit(false);
    if (!bGateDemo && !bLatchDemo && bAnimateRod && BrassRod)
    {
        const float OldYaw=BrassRod->GetActorRotation().Yaw;
        MotionTime=FMath::Fmod(MotionTime+DeltaSeconds,60.f);
        // The mesh origin is the drilled bore axis. Rotate in place, 6 degrees/s.
        BrassRod->SetActorRotation(FRotator(0,BrassRestYaw+MotionTime*6.f,0));
        if (bSmokeTest)
        {
            RotationTravelDegrees+=FMath::Abs(FRotator::NormalizeAxis(BrassRod->GetActorRotation().Yaw-OldYaw));
            ++MotionFrames;
        }
    }
    if (bSmokeTest && !bTestFinished) TickSmokeTest();
    if (bPerformanceTest && !bTestFinished) TickPerformanceTest();
    if ((bVRTest || bVRHardwareTest) && !bTestFinished) TickVRTest();
    if (bGateTest && !bTestFinished) TickGateTest();
    if (bLatchTest && !bTestFinished) TickLatchTest();
}

void AMaterialHallController::BuildInterface()
{
    if (bVRMode) { BuildVRInterface(); return; }
    const FLinearColor Ink(.035f,.045f,.05f,1), Muted(.10f,.13f,.15f,1);
    static FButtonStyle HallButtonStyle = FButtonStyle()
        .SetNormal(FSlateColorBrush(FLinearColor(.76f,.74f,.68f,1)))
        .SetHovered(FSlateColorBrush(FLinearColor(.87f,.84f,.76f,1)))
        .SetPressed(FSlateColorBrush(FLinearColor(.63f,.60f,.53f,1)))
        .SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));
    const FSlateFontInfo Small=FCoreStyle::GetDefaultFontStyle("Regular",11);
    const FSlateFontInfo Medium=FCoreStyle::GetDefaultFontStyle("Regular",13);
    auto Button=[&](const FString& Label,TFunction<void()> Action)
    {
        return SNew(SButton).ButtonStyle(&HallButtonStyle).IsFocusable(false).ContentPadding(FMargin(13,8))
            .OnClicked_Lambda([Action](){ Action(); return FReply::Handled(); })
            [SNew(STextBlock).Text(FText::FromString(Label)).Font(Medium).ColorAndOpacity(Ink)];
    };
    TSharedRef<SHorizontalBox> Cameras=SNew(SHorizontalBox);
    const TCHAR* Names[]={TEXT("Table"),TEXT("Detail"),TEXT("Hall"),TEXT("Top / pivot")};
    for (int32 Index=0;Index<4;++Index)
        Cameras->AddSlot().AutoWidth().Padding(0,0,5,0)[Button(Names[Index],[this,Index](){ SetView(Index); })];
    TSharedRef<SHorizontalBox> Qualities=SNew(SHorizontalBox);
    const TCHAR* QualityNames[]={TEXT("Smooth"),TEXT("Balanced"),TEXT("Fine")};
    Qualities->AddSlot().AutoWidth().Padding(0,0,5,0)[Button(TEXT("Auto 60"),[this](){ SetAutoTune(true); })];
    for (int32 Index=0;Index<3;++Index)
        Qualities->AddSlot().AutoWidth().Padding(0,0,5,0)[Button(QualityNames[Index],[this,Index](){ SelectQuality(Index); })];

    auto ToggleButton=[&](TFunction<FText()> Label,TFunction<void()> Action)
    {
        return SNew(SButton).ButtonStyle(&HallButtonStyle).IsFocusable(false).ContentPadding(FMargin(13,8))
            .OnClicked_Lambda([Action](){ Action(); return FReply::Handled(); })
            [SNew(STextBlock).Text_Lambda([Label](){ return Label(); }).Font(Medium).ColorAndOpacity(Ink)];
    };

    Interface=SNew(SOverlay)
    + SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(24)
    [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
      .BorderBackgroundColor(FLinearColor(.95f,.94f,.90f,.94f)).Padding(FMargin(20,17))
      [SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString("MATERIAL HALL")).Font(FCoreStyle::GetDefaultFontStyle("Bold",18)).ColorAndOpacity(Ink)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,3,0,12)[SNew(STextBlock).Text_Lambda([this](){ return FText::FromString(bLatchDemo ? TEXT("Rod logic / RS latch") : bGateDemo ? TEXT("Rod logic / AND gate") : TEXT("Steel plate & rotating brass pointer")); }).Font(Small).ColorAndOpacity(Muted)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,10)[SNew(SHorizontalBox)
          +SHorizontalBox::Slot().AutoWidth().Padding(0,0,5,0)[Button(TEXT("RS latch"),[this](){SelectLatchDemo();})]
          +SHorizontalBox::Slot().AutoWidth().Padding(0,0,5,0)[Button(TEXT("AND gate"),[this](){SelectDemo(true);})]
          +SHorizontalBox::Slot().AutoWidth()[Button(TEXT("Pointer"),[this](){SelectDemo(false);})]]
        +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)[BuildGateControls(false)]
        +SVerticalBox::Slot().AutoHeight()[BuildLatchControls()]
        +SVerticalBox::Slot().AutoHeight()[Cameras]
        +SVerticalBox::Slot().AutoHeight().Padding(0,15,0,6)[SNew(STextBlock).Text(FText::FromString("RENDER QUALITY")).Font(Small).ColorAndOpacity(Muted)]
        +SVerticalBox::Slot().AutoHeight()[Qualities]
        +SVerticalBox::Slot().AutoHeight().Padding(0,8,0,8)[SNew(STextBlock).Font(Small).ColorAndOpacity(Muted).Text_Lambda([this](){ return FText::FromString(GetPerformanceStatus()); })]
        +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)[SNew(SHorizontalBox)
          +SHorizontalBox::Slot().AutoWidth().Padding(0,0,5,0)[ToggleButton([this](){ return FText::FromString(bVSync ? TEXT("VSync: On") : TEXT("VSync: Off")); },[this](){ bVSync=!bVSync; ApplyPerformanceSettings(); SavePerformanceSettings(); })]
          +SHorizontalBox::Slot().AutoWidth()[ToggleButton([this](){ return FText::FromString(AntiAliasingMethod==4 ? TEXT("Anti-aliasing: TSR") : TEXT("Anti-aliasing: TAA")); },[this](){ AntiAliasingMethod=AntiAliasingMethod==4 ? 2 : 4; ApplyPerformanceSettings(); QualityGovernor.Reset(); SavePerformanceSettings(); })]]
        +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString("SURFACE FINISH")).Font(Small).ColorAndOpacity(Muted)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,7,0,3)[SNew(SSlider).IsFocusable(false).Value((.24f-.08f)/.47f).OnValueChanged_Lambda([this](float Value){SetFinish(Value);})]
        +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString("Polished                                    Brushed")).Font(Small).ColorAndOpacity(Muted)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,15,0,0)[SNew(SHorizontalBox)
          +SHorizontalBox::Slot().AutoWidth().Padding(0,0,5,0)[SNew(SButton).ButtonStyle(&HallButtonStyle).IsFocusable(false).ContentPadding(FMargin(13,8)).OnClicked_Lambda([this](){ToggleMotion();return FReply::Handled();})[SNew(STextBlock).Font(Medium).ColorAndOpacity(Ink).Text_Lambda([this](){return GetMotionLabel();})]]
          +SHorizontalBox::Slot().AutoWidth()[Button("Reset",[this](){ResetScene();})]]
      ]]
    +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(24)
      [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.95f,.94f,.90f,.90f)).Padding(12)
        [SNew(STextBlock).Text_Lambda([this](){ return FText::FromString(bLatchDemo ? TEXT("Z / X toggle Set / Reset rods  |  Space cycle Set / Hold / Reset / Hold  |  R reset  |  G switch demo\n1 / 2 / 3 / 4 views  |  WASD + Q/E move  |  Right-drag orbit  |  Scroll zoom  |  Esc quit") : bGateDemo ? TEXT("Click input handles or Z / X to toggle A / B  |  Space cycle states  |  R reset  |  G switch demo\n1 / 2 / 3 / 4 views  |  WASD + Q/E move  |  Right-drag orbit  |  Scroll zoom  |  Esc quit") : TEXT("WASD move  |  Q/E down/up  |  IJKL look  |  Shift faster  |  Right-drag orbit  |  Scroll zoom\n1 / 2 / 3 / 4 views  |  F quality  |  Space pause / resume  |  R reset  |  G switch demo  |  Esc quit")); }).Font(Small).ColorAndOpacity(Ink)]];
    if (GEngine && GEngine->GameViewport) GEngine->GameViewport->AddViewportWidgetContent(Interface.ToSharedRef(),10);
}

void AMaterialHallController::Capture(const FString& Name)
{
    IFileManager::Get().MakeDirectory(*CaptureDirectory,true);
    FScreenshotRequest::RequestScreenshot(CaptureDirectory/Name,true,false);
}

void AMaterialHallController::TickSmokeTest()
{
    CheckAssembly();
    const double Elapsed=FPlatformTime::Seconds()-TestStart;
    auto KeyEvent=[this](FKey Key,EInputEvent Event)
    {
        InputKey(FInputKeyEventArgs(nullptr,IPlatformInputDeviceMapper::Get().GetDefaultInputDevice(),
            Key,Event,FPlatformTime::Cycles64()));
    };
    if (TestStage==0 && Elapsed>8) { ++TestStage; Capture(TEXT("table.png")); }
    else if (TestStage==1 && Elapsed>10) { ++TestStage; SetView(1); }
    else if (TestStage==2 && Elapsed>16) { ++TestStage; Capture(TEXT("detail.png")); }
    else if (TestStage==3 && Elapsed>18) { ++TestStage; SetView(2); SelectQuality(0); }
    else if (TestStage==4 && Elapsed>24) { ++TestStage; Capture(TEXT("hall.png")); }
    else if (TestStage==5 && Elapsed>26)
    { ++TestStage; SetView(0); SelectQuality(1); KeyEvent(EKeys::SpaceBar,IE_Pressed); }
    else if (TestStage==6 && Elapsed>27)
    { ++TestStage; KeyEvent(EKeys::SpaceBar,IE_Released); PausedYaw=BrassRod->GetActorRotation().Yaw; }
    else if (TestStage==7 && Elapsed>30)
    {
        ++TestStage;
        bPauseResumePassed &= !bAnimateRod && FMath::Abs(FRotator::NormalizeAxis(BrassRod->GetActorRotation().Yaw-PausedYaw))<.001;
        KeyEvent(EKeys::SpaceBar,IE_Pressed);
    }
    else if (TestStage==8 && Elapsed>31) { ++TestStage; KeyEvent(EKeys::SpaceBar,IE_Released); }
    else if (TestStage==9 && Elapsed>35)
    {
        ++TestStage;
        bPauseResumePassed &= bAnimateRod && FMath::Abs(FRotator::NormalizeAxis(BrassRod->GetActorRotation().Yaw-PausedYaw))>20;
        KeyEvent(EKeys::R,IE_Pressed);
    }
    else if (TestStage==10 && Elapsed>35.2)
    {
        ++TestStage; KeyEvent(EKeys::R,IE_Released);
        bResetPassed=bAnimateRod && MotionTime<1 && BrassRod->GetActorLocation().Equals(BrassRestPosition,.001);
    }
    else if (TestStage==11 && Elapsed>37) { ++TestStage; SetView(3); }
    else if (TestStage==12 && Elapsed>43) { ++TestStage; Capture(TEXT("pivot.png")); }
    else if (TestStage==13 && Elapsed>45) { ++TestStage; SetView(1); }
    else if (TestStage==14 && Elapsed>51) { ++TestStage; Capture(TEXT("rotation.png")); }
    // Keep running for a complete uninterrupted revolution after reset.
    else if (TestStage==15 && Elapsed>97) TickNavigationTest();
}

void AMaterialHallController::TickNavigationTest()
{
    const FKey Keys[]={EKeys::W,EKeys::S,EKeys::A,EKeys::D,EKeys::Q,EKeys::E,
        EKeys::I,EKeys::K,EKeys::J,EKeys::L,EKeys::W,EKeys::W,EKeys::F};
    auto KeyEvent=[this](FKey Key,EInputEvent Event)
    {
        InputKey(FInputKeyEventArgs(nullptr,IPlatformInputDeviceMapper::Get().GetDefaultInputDevice(),
            Key,Event,FPlatformTime::Cycles64()));
    };
    const double Now=FPlatformTime::Seconds();
    if (NavigationTestIndex>=0)
    {
        const double Duration=Now-NavigationStepStart;
        if (Duration<.35) return;
        KeyEvent(Keys[NavigationTestIndex],IE_Released);
        if (NavigationTestIndex==11) KeyEvent(EKeys::LeftShift,IE_Released);
        const FVector Delta=ViewCamera->GetActorLocation()-NavigationStartPosition;
        const FRotator Turn=(ViewCamera->GetActorRotation()-NavigationStartRotation).GetNormalized();
        const double Forward=FVector::DotProduct(Delta,NavigationStartRotation.Vector());
        const double Right=FVector::DotProduct(Delta,FRotationMatrix(NavigationStartRotation).GetUnitAxis(EAxis::Y));
        bool Passed=false;
        switch (NavigationTestIndex)
        {
            case 0: case 10: Passed=Forward>5 && FMath::Abs(Right)<.01; break;
            case 1: Passed=Forward<-5 && FMath::Abs(Right)<.01; break;
            case 2: Passed=Right<-5 && FMath::Abs(Forward)<.01; break;
            case 3: Passed=Right>5 && FMath::Abs(Forward)<.01; break;
            case 4: Passed=Delta.Z<-5 && Delta.Size2D()<.01 && QualityIndex==1; break;
            case 5: Passed=Delta.Z>5 && Delta.Size2D()<.01; break;
            case 6: Passed=Turn.Pitch>3 && Delta.Size()<.01; break;
            case 7: Passed=Turn.Pitch<-3 && Delta.Size()<.01; break;
            case 8: Passed=Turn.Yaw<-3 && Delta.Size()<.01; break;
            case 9: Passed=Turn.Yaw>3 && Delta.Size()<.01; break;
            case 11: Passed=Forward/Duration>240; break;
            case 12: Passed=QualityIndex==2; break;
        }
        bNavigationTestPassed &= Passed;
        TSharedRef<FJsonObject> Record=MakeShared<FJsonObject>();
        Record->SetStringField(TEXT("key"),NavigationTestIndex==10 ? TEXT("W after turning L") : NavigationTestIndex==11 ? TEXT("Shift+W") : Keys[NavigationTestIndex].ToString());
        Record->SetBoolField(TEXT("passed"),Passed);
        Record->SetNumberField(TEXT("forward_cm"),Forward);
        Record->SetNumberField(TEXT("right_cm"),Right);
        Record->SetNumberField(TEXT("world_up_cm"),Delta.Z);
        Record->SetNumberField(TEXT("yaw_degrees"),Turn.Yaw);
        Record->SetNumberField(TEXT("pitch_degrees"),Turn.Pitch);
        NavigationResults.Add(MakeShared<FJsonValueObject>(Record));
    }
    ++NavigationTestIndex;
    if (NavigationTestIndex==UE_ARRAY_COUNT(Keys))
    {
        bTestFinished=true; FinishSmokeTest(); return;
    }
    // The forward-after-turn case deliberately keeps the result of the L test.
    if (NavigationTestIndex!=10) SetView(0);
    NavigationStartPosition=ViewCamera->GetActorLocation();
    NavigationStartRotation=ViewCamera->GetActorRotation();
    NavigationStepStart=Now;
    if (NavigationTestIndex==11) KeyEvent(EKeys::LeftShift,IE_Pressed);
    KeyEvent(Keys[NavigationTestIndex],IE_Pressed);
}

void AMaterialHallController::CheckAssembly()
{
    UStaticMeshComponent* Top=nullptr;
    for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
    {
        UStaticMeshComponent* Mesh=It->GetStaticMeshComponent();
        if (Mesh && Mesh->GetStaticMesh() && Mesh->GetStaticMesh()->GetName()==TEXT("TableTop")) Top=Mesh;
    }
    if (!Top || !SteelPlate || !PivotPin || !PivotCollar || !BrassRod) { bAssemblyPassed=false; return; }
    auto Bounds=[](AStaticMeshActor* Actor)
    {
        FVector Min,Max; Actor->GetStaticMeshComponent()->GetLocalBounds(Min,Max);
        return FBox(Min,Max);
    };
    const FBox Plate=Bounds(SteelPlate), Pin=Bounds(PivotPin), Collar=Bounds(PivotCollar), Brass=Bounds(BrassRod);
    const FTransform PlateTransform=SteelPlate->GetActorTransform(), BrassTransform=BrassRod->GetActorTransform();
    FVector TopMin,TopMax; Top->GetLocalBounds(TopMin,TopMax);
    for (const double X : {Plate.Min.X+.2,Plate.Max.X-.2})
    for (const double Y : {Plate.Min.Y+.2,Plate.Max.Y-.2})
    {
        const FVector Local=Top->GetComponentTransform().InverseTransformPosition(PlateTransform.TransformPosition(FVector(X,Y,Plate.Min.Z)));
        const double Error=FMath::Abs(Local.Z-TopMax.Z);
        MaxSupportErrorCm=FMath::Max(MaxSupportErrorCm,Error);
        bAssemblyPassed &= Error<.001 && Local.X>TopMin.X+.6 && Local.X<TopMax.X-.6
            && Local.Y>TopMin.Y+.6 && Local.Y<TopMax.Y-.6;
    }
    const double PlateTop=SteelPlate->GetActorLocation().Z+Plate.Max.Z;
    const double CollarBottom=PivotCollar->GetActorLocation().Z+Collar.Min.Z;
    const double CollarTop=PivotCollar->GetActorLocation().Z+Collar.Max.Z;
    const double PinBottom=PivotPin->GetActorLocation().Z+Pin.Min.Z;
    const double PinTop=PivotPin->GetActorLocation().Z+Pin.Max.Z;
    const double BrassBottom=BrassRod->GetActorLocation().Z+Brass.Min.Z;
    for (const double Error : {FMath::Abs(CollarBottom-PlateTop),FMath::Abs(PinBottom-PlateTop),FMath::Abs(BrassBottom-CollarTop)})
    {
        MaxSupportErrorCm=FMath::Max(MaxSupportErrorCm,Error);
        bAssemblyPassed &= Error<.001;
    }
    const double Drift=FVector::Dist(BrassRod->GetActorLocation(),BrassRestPosition);
    MaxPivotDriftCm=FMath::Max(MaxPivotDriftCm,Drift);
    bAssemblyPassed &= Drift<.001 && FVector::Dist2D(BrassRod->GetActorLocation(),PivotPin->GetActorLocation())<.001
        && FVector::Dist2D(SteelPlate->GetActorLocation(),PivotPin->GetActorLocation())<.001
        && FVector::Dist2D(PivotCollar->GetActorLocation(),PivotPin->GetActorLocation())<.001
        && FMath::Abs(BrassRod->GetActorRotation().Pitch)<.001 && FMath::Abs(BrassRod->GetActorRotation().Roll)<.001
        && PinTop>BrassRod->GetActorLocation().Z+Brass.Max.Z && Pin.Max.X<1.7;
    // Every bottom corner stays above the plate and inside its footprint throughout the turn.
    for (const double X : {Brass.Min.X+.12,Brass.Max.X-.12})
    for (const double Y : {Brass.Min.Y+.12,Brass.Max.Y-.12})
    {
        const FVector Local=PlateTransform.InverseTransformPosition(BrassTransform.TransformPosition(FVector(X,Y,Brass.Min.Z)));
        bAssemblyPassed &= Local.Z>Plate.Max.Z+.7 && Local.X>Plate.Min.X+.2 && Local.X<Plate.Max.X-.2
            && Local.Y>Plate.Min.Y+.2 && Local.Y<Plate.Max.Y-.2;
    }
    ++AssemblyFrames;
}

void AMaterialHallController::FinishSmokeTest()
{
    const bool Passed=ViewCamera && BrassRod && Metals.Num()==4 && RotationTravelDegrees>540 && MotionFrames>20
        && bAssemblyPassed && AssemblyFrames>20 && bPauseResumePassed && bResetPassed
        && bNavigationTestPassed && NavigationResults.Num()==13;
    TSharedRef<FJsonObject> Result=MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("passed"),Passed);
    Result->SetStringField(TEXT("engine"),FEngineVersion::Current().ToString());
    Result->SetNumberField(TEXT("metal_materials"),Metals.Num());
    Result->SetNumberField(TEXT("rotation_travel_degrees"),RotationTravelDegrees);
    Result->SetNumberField(TEXT("max_pivot_drift_cm"),MaxPivotDriftCm);
    Result->SetNumberField(TEXT("motion_frames"),MotionFrames);
    Result->SetBoolField(TEXT("assembly_fit_passed"),bAssemblyPassed);
    Result->SetNumberField(TEXT("max_support_error_cm"),MaxSupportErrorCm);
    Result->SetNumberField(TEXT("assembly_frames"),AssemblyFrames);
    Result->SetBoolField(TEXT("pause_resume_passed"),bPauseResumePassed);
    Result->SetBoolField(TEXT("reset_passed"),bResetPassed);
    Result->SetBoolField(TEXT("keyboard_navigation_passed"),bNavigationTestPassed);
    Result->SetArrayField(TEXT("keyboard_tests"),NavigationResults);
    Result->SetStringField(TEXT("brass_mesh"),BrassRod && BrassRod->GetStaticMeshComponent()->GetStaticMesh()
        ? BrassRod->GetStaticMeshComponent()->GetStaticMesh()->GetName() : TEXT("missing"));
    Result->SetStringField(TEXT("tests"),TEXT("Cooked map; four camera presets; quality switching; full revolution with fixed bore axis; plate/table and pointer/collar contact; Space pause/resume; R reset; injected WASD/QE/IJKL key events; look rotates in place; movement follows new look direction; Shift speed; Q keeps quality; F changes quality; screenshot capture; clean exit"));
    FString Output; auto Writer=TJsonWriterFactory<>::Create(&Output); FJsonSerializer::Serialize(Result,Writer);
    FFileHelper::SaveStringToFile(Output,*(CaptureDirectory/TEXT("runtime-check.json")));
    UE_LOG(LogMaterialHall,Display,TEXT("MATERIAL HALL SELF TEST %s"),Passed?TEXT("PASSED"):TEXT("FAILED"));
    FGenericPlatformMisc::RequestExit(false);
}

void AMaterialHallController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (Interface.IsValid() && GEngine && GEngine->GameViewport)
        GEngine->GameViewport->RemoveViewportWidgetContent(Interface.ToSharedRef());
    Interface.Reset();
    Super::EndPlay(EndPlayReason);
}
