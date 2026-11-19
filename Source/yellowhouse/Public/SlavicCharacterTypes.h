#pragma once

#include "CoreMinimal.h"
#include "SlavicCharacterTypes.generated.h"

UENUM(BlueprintType)
enum class ECharacterStance : uint8
{
    Standing,
    Sprinting,
    Crouching,
    Proning
};

USTRUCT(BlueprintType)
struct YELLOWHOUSE_API FStanceConfig
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stance", meta = (ClampMin = "1.0", Units = "cm"))
    float CapsuleHalfHeight = 60.0f;

    /** Camera Z relative to the center of the capsule at this stance. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stance", meta = (Units = "cm"))
    float CameraEyeHeight = 50.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stance", meta = (ClampMin = "0.0"))
    float MaxWalkSpeed = 400.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stance", meta = (ClampMin = "0.0"))
    float MaxAcceleration = 1600.0f;
};
