#include "SlavicPlayerController.h"

#include "Components/ActorComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "SlavicCharacter.h"
#include "UObject/Class.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

#if PLATFORM_WINDOWS
#include "Core/DeviceRegistry.h"
#include "Engine/GameInstance.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#endif

void ASlavicPlayerController::ReceivedPlayer()
{
    Super::ReceivedPlayer();

#if PLATFORM_WINDOWS
    if (GetLocalPlayer())
    {
        auto& Mapper = IPlatformInputDeviceMapper::Get();
        auto& ConnectionChanged = Mapper.GetOnInputDeviceConnectionChange();
        ConnectionChanged.RemoveAll(this);
        ConnectionChanged.AddWeakLambda(this,
            [this](EInputDeviceConnectionState, FPlatformUserId, FInputDeviceId)
            {
                RouteSonyGamepadForSingleLocalPlayer();
            });
        RouteSonyGamepadForSingleLocalPlayer();
    }
#endif
}

#if PLATFORM_WINDOWS
void ASlavicPlayerController::RouteSonyGamepadForSingleLocalPlayer()
{
    const ULocalPlayer* LocalPlayer = GetLocalPlayer();
    const UGameInstance* GameInstance = GetGameInstance();
    auto& Mapper = IPlatformInputDeviceMapper::Get();
    if (!LocalPlayer || !GameInstance || GameInstance->GetNumLocalPlayers() != 1 ||
        GetNetMode() != NM_Standalone || LocalPlayer->GetPlatformUserId() != Mapper.GetPrimaryPlatformUser())
    {
        return;
    }

    TArray<FInputDeviceId> ConnectedDevices;
    Mapper.GetAllConnectedInputDevices(ConnectedDevices);
    const TSharedPtr<FDeviceRegistry> SonyDevices = FDeviceRegistry::Get();
    FInputDeviceId SonyGamepad = INPUTDEVICEID_NONE;
    for (const FInputDeviceId Device : ConnectedDevices)
    {
        if (SonyDevices->GetLibraryInstance(Device))
        {
            if (SonyGamepad.IsValid())
            {
                return;
            }
            SonyGamepad = Device;
        }
    }

    if (SonyGamepad.IsValid())
    {
        const FPlatformUserId PlayerUser = LocalPlayer->GetPlatformUserId();
        const FPlatformUserId CurrentOwner = Mapper.GetUserForInputDevice(SonyGamepad);
        if (CurrentOwner != PlayerUser)
        {
            // The DualSense plugin can assign a new user after switching between USB and Bluetooth.
            if (Mapper.Internal_ChangeInputDeviceUserMapping(SonyGamepad, PlayerUser, CurrentOwner))
            {
                UE_LOG(LogTemp, Display, TEXT("Slavic input routed Sony gamepad %d from platform user %d to %d."),
                    SonyGamepad.GetId(), CurrentOwner.GetInternalId(), PlayerUser.GetInternalId());
            }
        }
    }
}
#endif

void ASlavicPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent); EnhancedInput && IA_Pause)
    {
        EnhancedInput->BindAction(IA_Pause, ETriggerEvent::Completed, this, &ASlavicPlayerController::HandlePause);
    }
}

void ASlavicPlayerController::BindCharacterInput(UEnhancedInputComponent& Input, ASlavicCharacter& SlavicPawn)
{
    if (!bBindNativeGameplayInput)
    {
        return;
    }

    if (IA_Move)
    {
        Input.BindAction(IA_Move, ETriggerEvent::Triggered, this, &ASlavicPlayerController::HandleMove);
        Input.BindAction(IA_Move, ETriggerEvent::Completed, &SlavicPawn, &ASlavicCharacter::ApplyMoveInput, FVector2D::ZeroVector);
        Input.BindAction(IA_Move, ETriggerEvent::Canceled, &SlavicPawn, &ASlavicCharacter::ApplyMoveInput, FVector2D::ZeroVector);
    }
    if (IA_Look)
    {
        Input.BindAction(IA_Look, ETriggerEvent::Triggered, this, &ASlavicPlayerController::HandleLook, true);
    }
    if (IA_Look_Gamepad && IA_Look_Gamepad != IA_Look)
    {
        Input.BindAction(IA_Look_Gamepad, ETriggerEvent::Triggered, this, &ASlavicPlayerController::HandleLook, false);
    }
    if (IA_Jump)
    {
        Input.BindAction(IA_Jump, ETriggerEvent::Started, &SlavicPawn, &ASlavicCharacter::RequestJump);
        Input.BindAction(IA_Jump, ETriggerEvent::Completed, &SlavicPawn, &ASlavicCharacter::ReleaseJump);
        Input.BindAction(IA_Jump, ETriggerEvent::Canceled, &SlavicPawn, &ASlavicCharacter::ReleaseJump);
    }
    if (IA_Sprint)
    {
        Input.BindAction(IA_Sprint, ETriggerEvent::Started, &SlavicPawn, &ASlavicCharacter::RequestSprint, true);
        Input.BindAction(IA_Sprint, ETriggerEvent::Completed, &SlavicPawn, &ASlavicCharacter::RequestSprint, false);
        Input.BindAction(IA_Sprint, ETriggerEvent::Canceled, &SlavicPawn, &ASlavicCharacter::RequestSprint, false);
    }
    if (IA_Crouch)
    {
        Input.BindAction(IA_Crouch, ETriggerEvent::Started, &SlavicPawn, &ASlavicCharacter::RequestCrouch, true);
        Input.BindAction(IA_Crouch, ETriggerEvent::Completed, &SlavicPawn, &ASlavicCharacter::RequestCrouch, false);
        Input.BindAction(IA_Crouch, ETriggerEvent::Canceled, &SlavicPawn, &ASlavicCharacter::RequestCrouch, false);
    }
    if (IA_Prone)
    {
        Input.BindAction(IA_Prone, ETriggerEvent::Started, &SlavicPawn, &ASlavicCharacter::RequestProne);
    }
}

void ASlavicPlayerController::InstallGameplayMapping()
{
    UEnhancedInputLocalPlayerSubsystem* Subsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());

    if (InstalledMapping && (InstalledMapping != GameplayMappingContext || MappingSubsystem.Get() != Subsystem))
    {
        RemoveGameplayMapping();
    }

    if (bInstallNativeGameplayMapping && Subsystem && GameplayMappingContext &&
        !Subsystem->HasMappingContext(GameplayMappingContext))
    {
        Subsystem->AddMappingContext(GameplayMappingContext, GameplayMappingPriority);
        MappingSubsystem = Subsystem;
        InstalledMapping = GameplayMappingContext;
    }
}

void ASlavicPlayerController::RemoveGameplayMapping()
{
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = MappingSubsystem.Get(); Subsystem && InstalledMapping)
    {
        Subsystem->RemoveMappingContext(InstalledMapping);
    }
    MappingSubsystem.Reset();
    InstalledMapping = nullptr;
}

void ASlavicPlayerController::SetPawn(APawn* InPawn)
{
    Super::SetPawn(InPawn);
    if (!Cast<ASlavicCharacter>(InPawn))
    {
        RemoveGameplayMapping();
    }
}

void ASlavicPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
#if PLATFORM_WINDOWS
    IPlatformInputDeviceMapper::Get().GetOnInputDeviceConnectionChange().RemoveAll(this);
#endif
    RemoveGameplayMapping();
    Super::EndPlay(EndPlayReason);
}

void ASlavicPlayerController::FlushPressedKeys()
{
    Super::FlushPressedKeys();
    if (ASlavicCharacter* SlavicPawn = Cast<ASlavicCharacter>(GetPawn()))
    {
        SlavicPawn->ApplyMoveInput(FVector2D::ZeroVector);
        SlavicPawn->RequestSprint(false);
        SlavicPawn->RequestCrouch(false);
        SlavicPawn->ReleaseJump();
    }
}

void ASlavicPlayerController::HandleMove(const FInputActionValue& Value)
{
    if (ASlavicCharacter* SlavicPawn = Cast<ASlavicCharacter>(GetPawn()))
    {
        SlavicPawn->ApplyMoveInput(Value.Get<FVector2D>());
    }
}

void ASlavicPlayerController::HandleLook(const FInputActionValue& Value, bool bMouseLook)
{
    if (ASlavicCharacter* SlavicPawn = Cast<ASlavicCharacter>(GetPawn()))
    {
        FVector2D Look = Value.Get<FVector2D>();
        if (bMouseLook && Look.Y != 0.0 && MenuControllerComponentClass)
        {
            if (const UActorComponent* Menu = GetComponentByClass(MenuControllerComponentClass))
            {
                static const FName InvertYAxisName(TEXT("bInvertYAxis"));
                const auto* InvertYAxis = FindFProperty<FBoolProperty>(Menu->GetClass(), InvertYAxisName);
                // Menu System Pro's raw mouse Y needs a sign change for standard pitch.
                if (InvertYAxis && !InvertYAxis->GetPropertyValue_InContainer(Menu))
                {
                    Look.Y = -Look.Y;
                }
            }
        }
        SlavicPawn->ApplyLookInput(Look);
    }
}

void ASlavicPlayerController::HandlePause()
{
    if (!IsLocalController() || !MenuControllerComponentClass)
    {
        return;
    }

    UActorComponent* Menu = GetComponentByClass(MenuControllerComponentClass);
    if (!ensureMsgf(Menu, TEXT("%s is missing its menu controller component."), *GetName()))
    {
        return;
    }

    // Menu System Pro supplies its API and the active global menu configuration through Blueprints.
    const auto* ConfigProperty = FindFProperty<FObjectPropertyBase>(Menu->GetClass(), TEXT("GlobalMenuConfig"));
    const UObject* Config = ConfigProperty ? ConfigProperty->GetObjectPropertyValue_InContainer(Menu) : nullptr;
    const auto* IngameMenu = Config ? FindFProperty<FSoftClassProperty>(Config->GetClass(), TEXT("IngameMenuClass")) : nullptr;
    if (!IngameMenu || IngameMenu->GetPropertyValue_InContainer(Config).IsNull())
    {
        return;
    }

    UFunction* OpenPauseMenu = Menu->FindFunction(TEXT("OpenPauseMenu"));
    const auto* PlayerParam = OpenPauseMenu ? FindFProperty<FObjectPropertyBase>(OpenPauseMenu, TEXT("PlayerController")) : nullptr;
    const auto* MenuParam = OpenPauseMenu ? FindFProperty<FSoftClassProperty>(OpenPauseMenu, TEXT("Menu")) : nullptr;
    if (!ensureMsgf(PlayerParam && MenuParam && PlayerParam->HasAnyPropertyFlags(CPF_Parm) &&
        MenuParam->HasAnyPropertyFlags(CPF_Parm) && IsA(PlayerParam->PropertyClass),
        TEXT("%s has an incompatible OpenPauseMenu signature."), *GetName()))
    {
        return;
    }

    FStructOnScope Parameters(OpenPauseMenu);
    uint8* ParameterData = Parameters.GetStructMemory();
    PlayerParam->SetObjectPropertyValue_InContainer(ParameterData, this);
    MenuParam->SetPropertyValue_InContainer(ParameterData, IngameMenu->GetPropertyValue_InContainer(Config));
    // The initialized parameters select submenu 0 and pass no payload, the default pause screen.
    Menu->ProcessEvent(OpenPauseMenu, ParameterData);
}
