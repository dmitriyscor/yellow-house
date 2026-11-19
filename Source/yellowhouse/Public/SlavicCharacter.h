#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SlavicCharacterTypes.h"
#include "SlavicCharacter.generated.h"

class UCameraComponent;

UCLASS(Blueprintable)
class YELLOWHOUSE_API ASlavicCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ASlavicCharacter();

    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void PawnClientRestart() override;
    virtual void UnPossessed() override;
    virtual void Jump() override;

    UFUNCTION(BlueprintCallable, Category = "Slavic|Input")
    void ApplyMoveInput(FVector2D Value);

    UFUNCTION(BlueprintCallable, Category = "Slavic|Input")
    void ApplyLookInput(FVector2D Value);

    UFUNCTION(BlueprintCallable, Category = "Slavic|Input")
    void RequestJump();

    UFUNCTION(BlueprintCallable, Category = "Slavic|Input")
    void ReleaseJump();

    UFUNCTION(BlueprintCallable, Category = "Slavic|Stance")
    void RequestSprint(bool bPressed = true);

    UFUNCTION(BlueprintCallable, Category = "Slavic|Stance")
    void RequestCrouch(bool bPressed = true);

    UFUNCTION(BlueprintCallable, Category = "Slavic|Stance")
    void RequestProne();

    UFUNCTION(BlueprintCallable, Category = "Slavic|Stance")
    void RequestStand();

    UFUNCTION(BlueprintPure, Category = "Slavic|Stance")
    bool CanOccupyStance(ECharacterStance Stance) const;

    UFUNCTION(BlueprintPure, Category = "Slavic|Stance")
    bool CanTransitionToStance(ECharacterStance Stance) const;

    UFUNCTION(BlueprintPure, Category = "Slavic|Stance")
    ECharacterStance GetCurrentStance() const { return CurrentStance; }

    UFUNCTION(BlueprintPure, Category = "Slavic|Stance")
    ECharacterStance GetTargetStance() const { return TargetStance; }

    UFUNCTION(BlueprintPure, Category = "Slavic|Stance")
    bool IsSprinting() const { return CurrentStance == ECharacterStance::Sprinting; }

    UFUNCTION(BlueprintPure, Category = "Slavic|Stance")
    bool IsCrouching() const { return CurrentStance == ECharacterStance::Crouching; }

    UFUNCTION(BlueprintPure, Category = "Slavic|Stance")
    bool IsProning() const { return CurrentStance == ECharacterStance::Proning; }

    /** Physical capsule progress from the current transition's start to its target. */
    UFUNCTION(BlueprintPure, Category = "Slavic|Stance")
    float GetStanceAlpha() const { return StanceAlpha; }

protected:
    virtual void PostInitializeComponents() override;
    virtual void OnMovementModeChanged(EMovementMode PreviousMode, uint8 PreviousCustomMode) override;
    virtual bool CanJumpInternal_Implementation() const override;

    UFUNCTION(BlueprintImplementableEvent, Category = "Slavic|Stance")
    void OnStanceChangeBlocked(ECharacterStance AttemptedStance);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Slavic|Camera")
    TObjectPtr<UCameraComponent> FirstPersonCameraComponent;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slavic|Stance")
    FStanceConfig StandingConfig;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slavic|Stance")
    FStanceConfig CrouchingConfig;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slavic|Stance")
    FStanceConfig ProningConfig;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slavic|Stance", meta = (ClampMin = "1.0", Units = "cm"))
    float CapsuleRadius = 30.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slavic|Stance", meta = (ClampMin = "0.0", Units = "cm"))
    float ClearanceSkinWidth = 0.5f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slavic|Stance", meta = (ClampMin = "0.01"))
    float StanceTransitionSpeed = 6.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slavic|Camera", meta = (ClampMin = "0.01"))
    float CameraTransitionSpeed = 5.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slavic|Stance")
    bool bToggleCrouch = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slavic|Sprint", meta = (ClampMin = "0.0"))
    float SprintMaxWalkSpeed = 700.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slavic|Sprint", meta = (ClampMin = "0.0"))
    float SprintMaxAcceleration = 2400.0f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Slavic|Stance")
    ECharacterStance CurrentStance = ECharacterStance::Standing;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Slavic|Stance")
    ECharacterStance TargetStance = ECharacterStance::Standing;

private:
    const FStanceConfig& GetConfig(ECharacterStance Stance) const;
    float GetStanceHalfHeight(ECharacterStance Stance) const;
    bool CanFitCapsuleHeight(float TargetHalfHeight) const;
    bool IsFullyUpright() const;
    bool MeetsSprintConditions() const;
    bool TrySetTargetStance(ECharacterStance Stance);
    void ResolveBlockedGrowth(ECharacterStance AttemptedStance);
    void SetTargetStance(ECharacterStance Stance);
    void UpdateSprint();
    void UpdateCapsule(float DeltaSeconds);
    void UpdateCamera(float DeltaSeconds);
    void ApplyMovementTuning();

    FVector2D MoveInput = FVector2D::ZeroVector;
    float TransitionStartHalfHeight = 60.0f;
    float StanceAlpha = 1.0f;
    bool bSprintHeld = false;
    bool bCrouchHeld = false;
};
