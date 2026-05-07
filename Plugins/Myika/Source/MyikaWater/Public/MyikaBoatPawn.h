// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MyikaBoatPawn.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UMyikaBuoyancyComponent;
class USpringArmComponent;
class UStaticMeshComponent;
struct FInputActionValue;

UCLASS(BlueprintType, Blueprintable, ClassGroup = (Myika), meta = (DisplayName = "Myika Boat Pawn"))
class MYIKAWATER_API AMyikaBoatPawn : public APawn
{
	GENERATED_BODY()

public:
	AMyikaBoatPawn();

	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	void RefreshHullSetup();
	void EnsureInputMappings();
	void HandleMove(const FInputActionValue& Value);
	void HandleMoveCompleted(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void ApplyThrottle(float Value);
	void ApplySteering(float Value);

	/** Physics-driven hull mesh that the buoyancy component simulates. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myika|Water|Boat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> HullMesh;

	/** Myika-friendly wrapper around UE Water's buoyancy component. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myika|Water|Boat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMyikaBuoyancyComponent> BuoyancyComponent;

	/** Spring arm for the boat chase camera. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myika|Water|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Follow camera mounted behind the boat. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myika|Water|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	/** Force applied while accelerating forward. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Boat",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "250000.0", UIMin = "0.0", UIMax = "100000.0", SliderExponent = "2.0"))
	float ForwardThrust = 45000.0f;

	/** Force applied while reversing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Boat",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "150000.0", UIMin = "0.0", UIMax = "60000.0", SliderExponent = "2.0"))
	float ReverseThrust = 22000.0f;

	/** Torque applied when steering left or right. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Boat",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1000000.0", UIMin = "0.0", UIMax = "250000.0", SliderExponent = "2.0"))
	float SteeringTorque = 135000.0f;

	/** Dampens linear motion so the demo hull does not skate unrealistically. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Boat",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "10.0", UIMin = "0.0", UIMax = "3.0", SliderExponent = "2.0"))
	float HullLinearDamping = 0.55f;

	/** Dampens angular motion so the boat settles naturally after steering input. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Boat",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "20.0", UIMin = "0.0", UIMax = "8.0", SliderExponent = "2.0"))
	float HullAngularDamping = 2.5f;

	/** Approximate mass for the demo hull. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Boat",
		meta = (AllowPrivateAccess = "true", ClampMin = "25.0", ClampMax = "5000.0", UIMin = "100.0", UIMax = "1000.0", SliderExponent = "2.0"))
	float HullMassKg = 450.0f;

	/** Lowers the center of mass so the hull resists unrealistic tipping. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Boat",
		meta = (AllowPrivateAccess = "true", ClampMin = "-100.0", ClampMax = "100.0", UIMin = "-50.0", UIMax = "50.0"))
	float CenterOfMassOffsetZ = -20.0f;

	/** Default mapping context reused from the tracked third-person input assets. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Myika|Water|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> DefaultInputMappingContext;

	/** Mouse look mapping context reused from the tracked third-person input assets. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Myika|Water|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> MouseLookMappingContext;

	/** Shared move action from the third-person template. X steers, Y throttles. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Myika|Water|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction;

	/** Shared look action from the third-person template. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Myika|Water|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction;

	/** Shared mouse look action from the third-person template. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Myika|Water|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MouseLookAction;
};
