#include "SlavicCharacter.h"

#include "Camera/CameraComponent.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Engine/ScopedMovementUpdate.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SlavicPlayerController.h"

namespace
{
    constexpr float HeightTolerance = 0.01f;

    bool IsUpright(ECharacterStance Stance)
    {
        return Stance == ECharacterStance::Standing || Stance == ECharacterStance::Sprinting;
    }
}

ASlavicCharacter::ASlavicCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    CrouchingConfig = {30.0f, 22.0f, 220.0f, 1100.0f}; 
                        // capsule half-height in cm
                        // camera height in cm above the capsule center
                        // maximum walking speed in cm/s
                        // maximum walking acceleration in cm/s2


    ProningConfig = {10.0f, 8.0f, 100.0f, 650.0f};
    GetCapsuleComponent()->InitCapsuleSize(CapsuleRadius, StandingConfig.CapsuleHalfHeight);

    FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("SlavicFirstPersonCamera"));
    FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
    FirstPersonCameraComponent->SetRelativeLocation(FVector(0.0, 0.0, StandingConfig.CameraEyeHeight));
    FirstPersonCameraComponent->bUsePawnControlRotation = true;

    bUseControllerRotationYaw = true;
    UCharacterMovementComponent* Movement = GetCharacterMovement();
    Movement->bOrientRotationToMovement = false;
    Movement->GetNavAgentPropertiesRef().bCanCrouch = false;
    Movement->MaxWalkSpeed = StandingConfig.MaxWalkSpeed;
    Movement->MaxAcceleration = StandingConfig.MaxAcceleration;
}

void ASlavicCharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    const float Radius = FMath::Max(1.0f, CapsuleRadius);
    const float HalfHeight = FMath::Max(Radius, StandingConfig.CapsuleHalfHeight);
    GetCapsuleComponent()->SetCapsuleSize(Radius, HalfHeight);
    TransitionStartHalfHeight = HalfHeight;

    FVector CameraLocation = FirstPersonCameraComponent->GetRelativeLocation();
    CameraLocation.Z = StandingConfig.CameraEyeHeight;
    FirstPersonCameraComponent->SetRelativeLocation(CameraLocation);
    ApplyMovementTuning();
}

void ASlavicCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    ASlavicPlayerController* SlavicController = Cast<ASlavicPlayerController>(GetController());
    UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (SlavicController && EnhancedInput)
    {
        SlavicController->BindCharacterInput(*EnhancedInput, *this);
    }
}

void ASlavicCharacter::PawnClientRestart()
{
    Super::PawnClientRestart();
    MoveInput = FVector2D::ZeroVector;
    RequestSprint(false);
    RequestCrouch(false);
    ReleaseJump();

    if (ASlavicPlayerController* SlavicController = Cast<ASlavicPlayerController>(GetController()))
    {
        SlavicController->InstallGameplayMapping();
    }
}

void ASlavicCharacter::UnPossessed()
{
    MoveInput = FVector2D::ZeroVector;
    RequestSprint(false);
    RequestCrouch(false);
    ReleaseJump();
    Super::UnPossessed();
}

void ASlavicCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateCapsule(DeltaSeconds);
    UpdateSprint();
    UpdateCamera(DeltaSeconds);
}

void ASlavicCharacter::ApplyMoveInput(FVector2D Value)
{
    MoveInput = Value;
    const FRotator Heading(0.0, GetControlRotation().Yaw, 0.0);
    const FRotationMatrix Basis(Heading);
    AddMovementInput(Basis.GetUnitAxis(EAxis::X), Value.Y);
    AddMovementInput(Basis.GetUnitAxis(EAxis::Y), Value.X);
    UpdateSprint();
}

void ASlavicCharacter::ApplyLookInput(FVector2D Value)
{
    AddControllerYawInput(Value.X);
    AddControllerPitchInput(Value.Y);
}

void ASlavicCharacter::RequestSprint(bool bPressed)
{
    bSprintHeld = bPressed;
    if (bPressed && !IsUpright(TargetStance))
    {
        TrySetTargetStance(ECharacterStance::Standing);
    }
    UpdateSprint();
}

void ASlavicCharacter::RequestCrouch(bool bPressed)
{
    if (!bPressed)
    {
        const bool bReleaseCrouch = bCrouchHeld && TargetStance == ECharacterStance::Crouching;
        bCrouchHeld = false;
        if (bReleaseCrouch)
        {
            RequestStand();
        }
        return;
    }

    bSprintHeld = false;
    bCrouchHeld = !bToggleCrouch;
    if (bToggleCrouch && TargetStance == ECharacterStance::Crouching)
    {
        RequestStand();
        return;
    }
    TrySetTargetStance(ECharacterStance::Crouching);
}

void ASlavicCharacter::RequestProne()
{
    bCrouchHeld = false;
    if (TargetStance == ECharacterStance::Proning)
    {
        RequestStand();
        return;
    }
    bSprintHeld = false;
    TrySetTargetStance(ECharacterStance::Proning);
}

void ASlavicCharacter::RequestStand()
{
    bSprintHeld = false;
    bCrouchHeld = false;
    TrySetTargetStance(ECharacterStance::Standing);
}

void ASlavicCharacter::RequestJump()
{
    Jump();
}

void ASlavicCharacter::ReleaseJump()
{
    StopJumping();
}

void ASlavicCharacter::Jump()
{
    if (!IsFullyUpright())
    {
        RequestStand();
        return;
    }
    Super::Jump();
}

bool ASlavicCharacter::CanJumpInternal_Implementation() const
{
    return IsFullyUpright() && Super::CanJumpInternal_Implementation();
}

void ASlavicCharacter::OnMovementModeChanged(EMovementMode PreviousMode, uint8 PreviousCustomMode)
{
    Super::OnMovementModeChanged(PreviousMode, PreviousCustomMode);
    UpdateSprint();
}

const FStanceConfig& ASlavicCharacter::GetConfig(ECharacterStance Stance) const
{
    switch (Stance)
    {
    case ECharacterStance::Proning:
        return ProningConfig;
    case ECharacterStance::Crouching:
        return CrouchingConfig;
    default:
        return StandingConfig;
    }
}

float ASlavicCharacter::GetStanceHalfHeight(ECharacterStance Stance) const
{
    const float Radius = GetCapsuleComponent()->GetUnscaledCapsuleRadius();
    const float StandingHeight = FMath::Max(Radius, StandingConfig.CapsuleHalfHeight);
    const float CrouchingHeight = FMath::Clamp(CrouchingConfig.CapsuleHalfHeight, Radius, StandingHeight);

    if (Stance == ECharacterStance::Proning)
    {
        return FMath::Clamp(ProningConfig.CapsuleHalfHeight, Radius, CrouchingHeight);
    }
    return Stance == ECharacterStance::Crouching ? CrouchingHeight : StandingHeight;
}

bool ASlavicCharacter::CanOccupyStance(ECharacterStance Stance) const
{
    return CanFitCapsuleHeight(GetStanceHalfHeight(Stance));
}

bool ASlavicCharacter::CanTransitionToStance(ECharacterStance Stance) const
{
    return (Stance != ECharacterStance::Sprinting || MeetsSprintConditions()) && CanOccupyStance(Stance);
}

bool ASlavicCharacter::CanFitCapsuleHeight(float TargetHalfHeight) const
{
    const UWorld* World = GetWorld();
    const UCapsuleComponent* Capsule = GetCapsuleComponent();
    if (!World || !Capsule->IsQueryCollisionEnabled())
    {
        return false;
    }

    const float ShapeScale = Capsule->GetShapeScale();
    const float Radius = Capsule->GetScaledCapsuleRadius();
    const float CurrentHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    const float ScaledTargetHalfHeight = FMath::Max(Radius, TargetHalfHeight * ShapeScale);
    const float DeltaHeight = ScaledTargetHalfHeight - CurrentHalfHeight;
    const FVector CurrentCenter = Capsule->GetComponentLocation();
    const FVector TargetCenter = CurrentCenter + FVector::UpVector * DeltaHeight;
    const ECollisionChannel Channel = Capsule->GetCollisionObjectType();

    FCollisionQueryParams Params(SCENE_QUERY_STAT(SlavicStanceClearance), false, this);
    FCollisionResponseParams Responses;
    Capsule->InitSweepCollisionParams(Params, Responses);
    TArray<AActor*> AttachedActors;
    GetAttachedActors(AttachedActors, true, true);
    Params.AddIgnoredActors(AttachedActors);

    if (DeltaHeight > UE_KINDA_SMALL_NUMBER)
    {
        // With a fixed radius, the growing capsule adds the volume swept by its upper cap.
        // Starting that sweep at the current upper cap keeps the query out of the floor.
        const FVector UpperCapCenter = CurrentCenter + FVector::UpVector * (CurrentHalfHeight - Radius);
        const FVector TargetUpperCapCenter = UpperCapCenter + FVector::UpVector * (2.0f * DeltaHeight);
        const float Skin = FMath::Clamp(ClearanceSkinWidth * ShapeScale, 0.0f, Radius * 0.5f);
        FHitResult Hit;
        if (World->SweepSingleByChannel(Hit, UpperCapCenter, TargetUpperCapCenter, FQuat::Identity,
            Channel, FCollisionShape::MakeSphere(Radius - Skin), Params, Responses))
        {
            return false;
        }
    }

    // The exact destination shape catches side contact and any overlap masked by the sweep skin.
    return !World->OverlapBlockingTestByChannel(TargetCenter, Capsule->GetComponentQuat(), Channel,
        FCollisionShape::MakeCapsule(Radius, ScaledTargetHalfHeight), Params, Responses);
}

bool ASlavicCharacter::IsFullyUpright() const
{
    return IsUpright(CurrentStance) && IsUpright(TargetStance) &&
        FMath::IsNearlyEqual(GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(),
            GetStanceHalfHeight(ECharacterStance::Standing), HeightTolerance);
}

bool ASlavicCharacter::MeetsSprintConditions() const
{
    const FRotator Heading(0.0, GetControlRotation().Yaw, 0.0);
    return GetCharacterMovement()->IsMovingOnGround() && !IsMoveInputIgnored() &&
        MoveInput.Y > UE_KINDA_SMALL_NUMBER &&
        FVector::DotProduct(GetVelocity(), Heading.Vector()) > UE_KINDA_SMALL_NUMBER;
}

bool ASlavicCharacter::TrySetTargetStance(ECharacterStance Stance)
{
    if (GetStanceHalfHeight(Stance) > GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() &&
        !CanOccupyStance(Stance))
    {
        ResolveBlockedGrowth(Stance);
        return false;
    }
    SetTargetStance(Stance);
    return true;
}

void ASlavicCharacter::ResolveBlockedGrowth(ECharacterStance AttemptedStance)
{
    const ECharacterStance RequestedStance = bSprintHeld && AttemptedStance == ECharacterStance::Standing
        ? ECharacterStance::Sprinting
        : AttemptedStance;
    bSprintHeld = false;
    const ECharacterStance SafeStance = IsUpright(AttemptedStance) && CanOccupyStance(ECharacterStance::Crouching)
        ? ECharacterStance::Crouching
        : ECharacterStance::Proning;
    SetTargetStance(SafeStance);
    OnStanceChangeBlocked(RequestedStance);
}

void ASlavicCharacter::SetTargetStance(ECharacterStance Stance)
{
    if (Stance == TargetStance)
    {
        return;
    }

    if (IsSprinting() && !IsUpright(Stance))
    {
        CurrentStance = ECharacterStance::Standing;
    }
    TargetStance = Stance;
    TransitionStartHalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
    StanceAlpha = 0.0f;
    if (FMath::IsNearlyEqual(GetStanceHalfHeight(Stance), TransitionStartHalfHeight, HeightTolerance))
    {
        CurrentStance = Stance;
        StanceAlpha = 1.0f;
    }
    if (!IsUpright(Stance))
    {
        StopJumping();
    }
    ApplyMovementTuning();
}

void ASlavicCharacter::UpdateSprint()
{
    if (TargetStance == ECharacterStance::Sprinting)
    {
        if (!bSprintHeld || !MeetsSprintConditions())
        {
            SetTargetStance(ECharacterStance::Standing);
        }
    }
    else if (bSprintHeld && IsFullyUpright() && MeetsSprintConditions() &&
        CanOccupyStance(ECharacterStance::Sprinting))
    {
        SetTargetStance(ECharacterStance::Sprinting);
    }
}

void ASlavicCharacter::UpdateCapsule(float DeltaSeconds)
{
    UCapsuleComponent* Capsule = GetCapsuleComponent();
    const float CurrentHeight = Capsule->GetUnscaledCapsuleHalfHeight();
    const float DesiredHeight = GetStanceHalfHeight(TargetStance);
    if (CurrentHeight == DesiredHeight)
    {
        return;
    }

    float NextHeight = FMath::FInterpTo(CurrentHeight, DesiredHeight, DeltaSeconds,
        FMath::Max(0.01f, StanceTransitionSpeed));
    if (FMath::IsNearlyEqual(NextHeight, DesiredHeight, HeightTolerance))
    {
        NextHeight = DesiredHeight;
    }
    if (NextHeight == CurrentHeight)
    {
        return;
    }
    if (NextHeight > CurrentHeight && !CanFitCapsuleHeight(DesiredHeight))
    {
        ResolveBlockedGrowth(TargetStance);
        return;
    }

    const FVector CenterDelta = FVector::UpVector * (NextHeight - CurrentHeight) * Capsule->GetShapeScale();
    {
        // Shrink before moving down. Move up before growing. Both paths stay in the tested volume.
        FScopedMovementUpdate ScopedResize(Capsule, EScopedUpdate::DeferredUpdates);
        if (NextHeight < CurrentHeight)
        {
            Capsule->SetCapsuleHalfHeight(NextHeight, false);
        }
        Capsule->MoveComponent(CenterDelta, Capsule->GetComponentQuat(), false, nullptr, MOVECOMP_NoFlags,
            ETeleportType::TeleportPhysics);
        if (NextHeight > CurrentHeight)
        {
            Capsule->SetCapsuleHalfHeight(NextHeight, false);
        }
        Capsule->UpdateOverlaps();
        GetCharacterMovement()->bForceNextFloorCheck = true;

        const float Travel = DesiredHeight - TransitionStartHalfHeight;
        StanceAlpha = FMath::Abs(Travel) <= HeightTolerance ? 1.0f :
            FMath::Clamp((NextHeight - TransitionStartHalfHeight) / Travel, 0.0f, 1.0f);
        if (NextHeight == DesiredHeight)
        {
            CurrentStance = TargetStance;
            StanceAlpha = 1.0f;
            ApplyMovementTuning();
        }
    }
}

void ASlavicCharacter::UpdateCamera(float DeltaSeconds)
{
    FVector Location = FirstPersonCameraComponent->GetRelativeLocation();
    const float CameraTarget = GetConfig(TargetStance).CameraEyeHeight;
    Location.Z = FMath::FInterpTo(static_cast<float>(Location.Z), CameraTarget, DeltaSeconds,
        FMath::Max(0.01f, CameraTransitionSpeed));
    FirstPersonCameraComponent->SetRelativeLocation(Location);
}

void ASlavicCharacter::ApplyMovementTuning()
{
    UCharacterMovementComponent* Movement = GetCharacterMovement();
    const FStanceConfig& Current = GetConfig(CurrentStance);
    const FStanceConfig& Target = GetConfig(TargetStance);
    const bool bSprinting = IsSprinting();
    const float Speed = bSprinting ? SprintMaxWalkSpeed : FMath::Min(Current.MaxWalkSpeed, Target.MaxWalkSpeed);
    const float Acceleration = bSprinting ? SprintMaxAcceleration :
        FMath::Min(Current.MaxAcceleration, Target.MaxAcceleration);
    Movement->MaxWalkSpeed = FMath::Max(0.0f, Speed);
    Movement->MaxAcceleration = FMath::Max(0.0f, Acceleration);
}
