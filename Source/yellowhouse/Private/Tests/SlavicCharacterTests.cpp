#include "SlavicCharacter.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

namespace
{
    AActor* SpawnBlockingBox(UWorld& World, const FVector& Location, const FVector& Extent)
    {
        AActor* Actor = World.SpawnActor<AActor>();
        UBoxComponent* Box = NewObject<UBoxComponent>(Actor);
        Actor->AddInstanceComponent(Box);
        Actor->SetRootComponent(Box);
        Actor->SetActorLocation(Location);
        Box->SetMobility(EComponentMobility::Movable);
        Box->InitBoxExtent(Extent);
        Box->SetCollisionProfileName(TEXT("BlockAll"));
        Box->RegisterComponent();
        return Actor;
    }

    float GetCapsuleBottom(const ASlavicCharacter& Character)
    {
        return Character.GetActorLocation().Z - Character.GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    }

    void FinishTransition(ASlavicCharacter& Character)
    {
        constexpr float Step = 1.0f / 60.0f;
        for (int32 Frame = 0; Frame < 90; ++Frame)
        {
            Character.Tick(Step);
        }
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSlavicStanceClearanceTest, "YellowHouse.Slavic.StanceClearance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSlavicStanceClearanceTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    SpawnBlockingBox(*World, FVector(0.0, 0.0, -20.0), FVector(500.0, 500.0, 20.0));
    ASlavicCharacter* Character = World->SpawnActor<ASlavicCharacter>(FVector(0.0, 0.0, 90.0), FRotator::ZeroRotator);
    if (!TestNotNull(TEXT("The character spawns above the floor"), Character))
    {
        return false;
    }
    const float InitialBottom = GetCapsuleBottom(*Character);

    Character->RequestProne();
    Character->Tick(1.0f / 60.0f);
    TestTrue(TEXT("First prone frame starts an interpolation"),
        Character->GetStanceAlpha() > 0.0f && Character->GetStanceAlpha() < 1.0f);
    FinishTransition(*Character);
    TestTrue(TEXT("Prone is reached"), Character->IsProning());
    TestEqual(TEXT("Prone half-height reaches its radius"), Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(),
        Character->GetCapsuleComponent()->GetUnscaledCapsuleRadius());
    TestTrue(TEXT("The capsule bottom stays planted"), FMath::IsNearlyEqual(GetCapsuleBottom(*Character), InitialBottom, 0.01f));

    AActor* Ceiling = SpawnBlockingBox(*World, FVector(0.0, 0.0, 87.0), FVector(100.0, 100.0, 5.0));
    Character->RequestCrouch();
    TestTrue(TEXT("A low ceiling blocks prone-to-crouch"), Character->GetTargetStance() == ECharacterStance::Proning);

    Ceiling->SetActorLocation(FVector(0.0, 0.0, 125.0));
    Character->RequestJump();
    TestTrue(TEXT("Jump requests the highest fitting stance"), Character->GetTargetStance() == ECharacterStance::Crouching);
    FinishTransition(*Character);
    TestTrue(TEXT("The fallback completes without jumping"), Character->IsCrouching() && !Character->bPressedJump);
    TestFalse(TEXT("Crouched characters cannot jump"), Character->CanJump());
    Character->RequestSprint();
    TestTrue(TEXT("Sprint cannot bypass the standing clearance check"), Character->GetTargetStance() == ECharacterStance::Crouching);

    Ceiling->SetActorLocation(FVector(0.0, 0.0, 182.75));
    TestFalse(TEXT("The exact capsule catches overlap within the sweep skin"),
        Character->CanOccupyStance(ECharacterStance::Standing));
    Ceiling->SetActorLocation(FVector(0.0, 0.0, 300.0));
    Character->RequestStand();
    Character->Tick(1.0f / 60.0f);
    const float PartialHeight = Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
    Ceiling->SetActorLocation(FVector(0.0, 0.0, 125.0));
    Character->Tick(1.0f / 60.0f);
    TestTrue(TEXT("A moving ceiling cancels growth in the next frame"), Character->GetTargetStance() == ECharacterStance::Crouching);
    TestTrue(TEXT("The capsule never grows after the obstruction enters"), Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() <= PartialHeight);
    FinishTransition(*Character);

    Ceiling->SetActorLocation(FVector(0.0, 0.0, 300.0));
    Character->RequestStand();
    FinishTransition(*Character);
    TestTrue(TEXT("An unobstructed stand succeeds above the real floor"), Character->GetCurrentStance() == ECharacterStance::Standing);
    TestTrue(TEXT("Growing keeps the bottom planted"), FMath::IsNearlyEqual(GetCapsuleBottom(*Character), InitialBottom, 0.01f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSlavicSprintRulesTest, "YellowHouse.Slavic.SprintRules",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSlavicSprintRulesTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    ASlavicCharacter* Character = World->SpawnActor<ASlavicCharacter>();
    if (!TestNotNull(TEXT("Character"), Character))
    {
        return false;
    }
    UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
    Movement->SetMovementMode(MOVE_Walking);
    Character->RequestSprint();
    TestFalse(TEXT("Idle characters cannot sprint"), Character->IsSprinting());

    Movement->Velocity = FVector(-150.0, 0.0, 0.0);
    Character->ApplyMoveInput(FVector2D(0.0, -1.0));
    TestFalse(TEXT("Backward movement cannot sprint"), Character->IsSprinting());

    Movement->Velocity = FVector(150.0, 0.0, 0.0);
    Character->ApplyMoveInput(FVector2D(0.0, 1.0));
    TestTrue(TEXT("Held Sprint starts during forward movement"), Character->IsSprinting());
    TestEqual(TEXT("Sprint uses its configured speed"), Movement->MaxWalkSpeed, 700.0f);

    Movement->SetMovementMode(MOVE_Falling);
    TestFalse(TEXT("Sprint ends as soon as the character is airborne"), Character->IsSprinting());
    TestEqual(TEXT("Leaving sprint restores walk speed"), Movement->MaxWalkSpeed, 400.0f);
    Movement->SetMovementMode(MOVE_Walking);
    TestTrue(TEXT("Held Sprint can resume after landing"), Character->IsSprinting());
    Character->RequestCrouch();
    TestFalse(TEXT("Crouch cancels sprint before the capsule finishes resizing"), Character->IsSprinting());
    TestEqual(TEXT("A lowering transition uses the lower movement speed"), Movement->MaxWalkSpeed, 220.0f);
    return true;
}

#endif
