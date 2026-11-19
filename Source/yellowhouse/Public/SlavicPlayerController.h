#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SlavicPlayerController.generated.h"

class ASlavicCharacter;
class UActorComponent;
class UEnhancedInputComponent;
class UEnhancedInputLocalPlayerSubsystem;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

UCLASS(Blueprintable)
class YELLOWHOUSE_API ASlavicPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    void BindCharacterInput(UEnhancedInputComponent& Input, ASlavicCharacter& SlavicPawn);
    void InstallGameplayMapping();

    virtual void SetPawn(APawn* InPawn) override;
    virtual void FlushPressedKeys() override;

protected:
    virtual void SetupInputComponent() override;
    virtual void ReceivedPlayer() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slavic|Input")
    TObjectPtr<UInputMappingContext> GameplayMappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slavic|Input")
    int32 GameplayMappingPriority = 0;

    /** Turn off when Blueprint handles gameplay action events. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slavic|Input")
    bool bBindNativeGameplayInput = true;

    /** Turn off when Blueprint owns the gameplay mapping context's lifetime. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slavic|Input")
    bool bInstallNativeGameplayMapping = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slavic|Input")
    TObjectPtr<UInputAction> IA_Move;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slavic|Input")
    TObjectPtr<UInputAction> IA_Look;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slavic|Input")
    TObjectPtr<UInputAction> IA_Look_Gamepad;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slavic|Input")
    TObjectPtr<UInputAction> IA_Jump;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slavic|Input")
    TObjectPtr<UInputAction> IA_Sprint;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slavic|Input")
    TObjectPtr<UInputAction> IA_Crouch;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slavic|Input")
    TObjectPtr<UInputAction> IA_Prone;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slavic|Menu")
    TObjectPtr<UInputAction> IA_Pause;

    /** Blueprint component on this controller that implements OpenPauseMenu. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slavic|Menu")
    TSubclassOf<UActorComponent> MenuControllerComponentClass;

private:
#if PLATFORM_WINDOWS
    void RouteSonyGamepadForSingleLocalPlayer();
#endif

    void RemoveGameplayMapping();
    void HandleMove(const FInputActionValue& Value);
    void HandleLook(const FInputActionValue& Value, bool bMouseLook);
    void HandlePause();

    TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> MappingSubsystem;

    UPROPERTY(Transient)
    TObjectPtr<UInputMappingContext> InstalledMapping;
};
