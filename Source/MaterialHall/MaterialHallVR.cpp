#include "MaterialHall.h"
#include "HallVRPawn.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "IXRTrackingSystem.h"
#if PLATFORM_WINDOWS
#include "IOpenXRHMD.h"
#endif
#include "InputKeyEventArgs.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Brushes/SlateColorBrush.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

bool AMaterialHallController::IsVRActive() const
{
    if (bVRTest) return true;
#if PLATFORM_WINDOWS
    if (!GEngine->XRSystem.IsValid() || !GEngine->IsStereoscopic3D() || !GEngine->XRSystem->HasValidTrackingPosition()) return false;
    IOpenXRHMD* XR = GEngine->XRSystem->GetIOpenXRHMD();
    return XR && XR->IsRunning() && XR->IsFocused();
#else
    return false;
#endif
}

void AMaterialHallController::BuildVRInterface()
{
    static FButtonStyle Style = FButtonStyle()
        .SetNormal(FSlateColorBrush(FLinearColor(.12f,.17f,.19f,1)))
        .SetHovered(FSlateColorBrush(FLinearColor(.25f,.38f,.42f,1)))
        .SetPressed(FSlateColorBrush(FLinearColor(.08f,.11f,.13f,1)));
    const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 24);
    auto Label = [Font](const FString& Text)
    {
        return SNew(STextBlock).Text(FText::FromString(Text)).Font(Font).ColorAndOpacity(FLinearColor::White);
    };
    auto Button = [&](const FString& Text, TFunction<void()> Action)
    {
        return SNew(SButton).ButtonStyle(&Style).IsFocusable(false).ContentPadding(FMargin(20,16))
            .OnClicked_Lambda([Action] { Action(); return FReply::Handled(); })[Label(Text)];
    };
    TSharedRef<SHorizontalBox> Qualities = SNew(SHorizontalBox);
    Qualities->AddSlot().FillWidth(1).Padding(4)[Button(TEXT("Auto"), [this] { SetAutoTune(true); })];
    const TCHAR* Names[] = { TEXT("Smooth"), TEXT("Balanced"), TEXT("Fine") };
    for (int32 Index = 0; Index < 3; ++Index)
        Qualities->AddSlot().FillWidth(1).Padding(4)[Button(Names[Index], [this,Index] { SelectQuality(Index); })];
    Interface = SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FLinearColor(.025f,.04f,.045f,1)).Padding(28)
        [SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("MATERIAL HALL  /  QUEST LINK")))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold",28)).ColorAndOpacity(FLinearColor::White)]
        + SVerticalBox::Slot().AutoHeight().Padding(0,16)[Label(TEXT("Point with right hand; trigger to select"))]
        + SVerticalBox::Slot().AutoHeight()[Qualities]
        + SVerticalBox::Slot().AutoHeight().Padding(4,12)[SNew(STextBlock).Font(Font).ColorAndOpacity(FLinearColor::White)
            .Text_Lambda([this] { return FText::FromString(GetPerformanceStatus()); })]
        + SVerticalBox::Slot().AutoHeight()[SNew(SButton).ButtonStyle(&Style).IsFocusable(false).ContentPadding(FMargin(20,14))
            .OnClicked_Lambda([this] { AntiAliasingMethod = AntiAliasingMethod == 2 ? 4 : 2; ApplyPerformanceSettings(); QualityGovernor.Reset(); VRResolutionGovernor.Reset(); SavePerformanceSettings(); return FReply::Handled(); })
            [SNew(STextBlock).Font(Font).ColorAndOpacity(FLinearColor::White).Text_Lambda([this] { return FText::FromString(AntiAliasingMethod == 2 ? TEXT("Anti-aliasing: TAA") : TEXT("Anti-aliasing: TSR")); })]]
        + SVerticalBox::Slot().AutoHeight().Padding(0,18,0,8)[Label(TEXT("Surface finish: polished to brushed"))]
        + SVerticalBox::Slot().AutoHeight().Padding(0,10)[SNew(SSlider).IsFocusable(false).Value((.24f-.08f)/.47f)
            .OnValueChanged_Lambda([this](float Value) { SetFinish(Value); })]
        + SVerticalBox::Slot().AutoHeight().Padding(0,18)[SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1).Padding(4)[Button(TEXT("Pause / resume"), [this] { ToggleMotion(); })]
            + SHorizontalBox::Slot().FillWidth(1).Padding(4)[Button(TEXT("Reset mechanism"), [this] { ResetScene(); })]]
        + SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1).Padding(4)[Button(TEXT("Next position"), [this] { VRPawn->SetStation(VRPawn->GetStation()+1); })]
            + SHorizontalBox::Slot().FillWidth(1).Padding(4)[Button(TEXT("Hide panel"), [this] { VRPawn->TogglePanel(); })]]
        + SVerticalBox::Slot().AutoHeight().Padding(0,18)[Label(TEXT("A: pause   B: reset   X: next position\nY: show / hide panel   Right stick: 45 degree turn"))]];
    VRPawn->SetPanel(Interface);
}

void AMaterialHallController::TickVRTest()
{
    const double Elapsed = FPlatformTime::Seconds() - TestStart;
    auto Key = [this](const FKey& Value, EInputEvent Event)
    {
        InputKey(FInputKeyEventArgs(nullptr, IPlatformInputDeviceMapper::Get().GetDefaultInputDevice(), Value, Event, FPlatformTime::Cycles64()));
    };
    if (bVRHardwareTest)
    {
        bVRSeenHead |= GEngine->XRSystem.IsValid() && GEngine->XRSystem->HasValidTrackingPosition();
        bVRSeenFocus |= IsVRActive();
        bVRSeenControllers |= VRPawn && VRPawn->AreControllersTracked();
        if (IsVRActive() && Elapsed > 10)
        {
            PerformanceFrameSum += GetWorld()->GetDeltaSeconds();
            ++PerformanceFrameCount;
        }
        if (Elapsed < 45) return;
    }
    else
    {
        if (TestStage == 0 && Elapsed > 2) { ++TestStage; Key(EKeys::OculusTouch_Right_A_Click, IE_Pressed); return; }
        if (TestStage == 1 && Elapsed > 2.3)
        {
            ++TestStage; bVRInputTestPassed &= !bAnimateRod;
            Key(EKeys::OculusTouch_Right_A_Click, IE_Released); Key(EKeys::OculusTouch_Right_B_Click, IE_Pressed); return;
        }
        if (TestStage == 2 && Elapsed > 2.6)
        {
            ++TestStage; bVRInputTestPassed &= bAnimateRod && MotionTime < .5;
            Key(EKeys::OculusTouch_Right_B_Click, IE_Released); Key(EKeys::OculusTouch_Left_X_Click, IE_Pressed); return;
        }
        if (TestStage == 3 && Elapsed > 2.9)
        {
            ++TestStage; bVRInputTestPassed &= VRPawn->GetStation() == 1;
            Key(EKeys::OculusTouch_Left_X_Click, IE_Released); Key(EKeys::OculusTouch_Left_Y_Click, IE_Pressed); return;
        }
        if (TestStage == 4 && Elapsed > 3.2)
        {
            ++TestStage; bVRInputTestPassed &= !VRPawn->IsPanelVisible();
            Key(EKeys::OculusTouch_Left_Y_Click, IE_Released);
            // Snap turning must pivot around the head even after room-scale walking.
            VRPawn->GetVRCamera()->SetRelativeLocation(FVector(35,-20,165));
            const FVector Before = VRPawn->GetVRCamera()->GetComponentLocation();
            const float Yaw = VRPawn->GetVRCamera()->GetComponentRotation().Yaw;
            VRPawn->SnapTurn(45);
            bVRInputTestPassed &= VRPawn->GetVRCamera()->GetComponentLocation().Equals(Before, .01) &&
                FMath::IsNearlyEqual(FRotator::NormalizeAxis(VRPawn->GetVRCamera()->GetComponentRotation().Yaw-Yaw),45.f,.01f);
            VRPawn->SetStation(0);
            bVRInputTestPassed &= VRPawn->GetVRCamera()->GetComponentLocation().Equals(FVector(0,150,165),.01);
            VRPawn->TogglePanel();
            // Exercise the cooked world-space widget's collision used by the ray.
            UWidgetComponent* Panel = VRPawn->GetPanel();
            const FVector Center = Panel->GetComponentLocation();
            FHitResult Hit;
            const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Center + Panel->GetForwardVector()*30,
                Center - Panel->GetForwardVector()*10, ECC_Visibility);
            bVRInputTestPassed &= bHit && Hit.GetComponent() == Panel;
            Capture(TEXT("vr-rig.png"));
            return;
        }
        if (Elapsed < 5) return;
    }
    const bool bStereo = GEngine->IsStereoscopic3D();
    const bool bPacing = IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS"))->GetInt() == 0 &&
        IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync"))->GetInt() == 0;
    const bool bPassed = VRPawn && VRPawn->IsRigValid() && bPacing && (bVRHardwareTest ?
        bStereo && bVRSeenHead && bVRSeenControllers && bVRSeenFocus && VRRefreshRate > 0 && bHasGPUTiming && PerformanceFrameCount > 120 : bVRInputTestPassed);
    TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("passed"), bPassed);
    Result->SetStringField(TEXT("test_type"), bVRHardwareTest ? TEXT("live_openxr_headset") : TEXT("simulated_rig_no_headset"));
    Result->SetBoolField(TEXT("rig_valid"), VRPawn && VRPawn->IsRigValid());
    Result->SetBoolField(TEXT("compositor_pacing"), bPacing);
    Result->SetBoolField(TEXT("stereo"), bStereo);
    Result->SetBoolField(TEXT("head_tracked"), bVRSeenHead);
    Result->SetBoolField(TEXT("controllers_tracked"), bVRSeenControllers);
    Result->SetBoolField(TEXT("session_focused"), bVRSeenFocus);
    Result->SetBoolField(TEXT("simulated_controls_and_transforms"), bVRTest && bVRInputTestPassed);
    Result->SetNumberField(TEXT("refresh_hz"), VRRefreshRate);
    Result->SetNumberField(TEXT("focused_frames"), PerformanceFrameCount);
    Result->SetNumberField(TEXT("average_fps"), PerformanceFrameSum > 0 ? PerformanceFrameCount / PerformanceFrameSum : 0);
    Result->SetNumberField(TEXT("gpu_ms"), GPUTimeMs);
    Result->SetNumberField(TEXT("resolution_percentage"), GetRenderPercentage());
    Result->SetNumberField(TEXT("quality"), QualityIndex);
    FString Output; auto Writer = TJsonWriterFactory<>::Create(&Output); FJsonSerializer::Serialize(Result, Writer);
    IFileManager::Get().MakeDirectory(*CaptureDirectory, true);
    FFileHelper::SaveStringToFile(Output, *(CaptureDirectory / TEXT("vr-check.json")));
    UE_LOG(LogTemp, Display, TEXT("MATERIAL HALL VR TEST %s: %s"), bPassed ? TEXT("PASSED") : TEXT("FAILED"), *Output);
    bTestFinished = true;
    FGenericPlatformMisc::RequestExit(false);
}
