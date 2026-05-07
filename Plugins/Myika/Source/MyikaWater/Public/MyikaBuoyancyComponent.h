// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "BuoyancyComponent.h"
#include "MyikaBuoyancyComponent.generated.h"

class UPrimitiveComponent;
struct FPropertyChangedEvent;

UCLASS(BlueprintType, Blueprintable, ClassGroup = (Myika), meta = (BlueprintSpawnableComponent, DisplayName = "Myika Buoyancy"))
class MYIKAWATER_API UMyikaBuoyancyComponent : public UBuoyancyComponent
{
	GENERATED_BODY()

public:
	UMyikaBuoyancyComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void OnRegister() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** Rebuild the pontoon layout from the current primitive bounds. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Myika|Water|Buoyancy")
	void RebuildPontoons();

	/** Returns the primitive component this buoyancy wrapper will drive. */
	UFUNCTION(BlueprintPure, Category = "Myika|Water|Buoyancy")
	UPrimitiveComponent* ResolveTargetPrimitive() const;

private:
	void ApplyAuthoringDefaults();
	void AutoConfigurePontoons();

	/** Automatically rebuild the pontoon layout from the owner's primitive bounds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Buoyancy|AutoPontoon", meta = (AllowPrivateAccess = "true"))
	bool bAutoConfigurePontoons = true;

	/** Adds a stabilizing center pontoon after the corner pontoons are generated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Buoyancy|AutoPontoon", meta = (AllowPrivateAccess = "true"))
	bool bAddCenterPontoon = true;

	/** Scales the pontoon radius from the owner's X/Y bounds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Buoyancy|AutoPontoon",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.05", ClampMax = "0.50", UIMin = "0.05", UIMax = "0.50", SliderExponent = "2.0"))
	float AutoPontoonRadiusScale = 0.22f;

	/** Pulls the pontoons inward from the primitive bounds to keep them under the hull. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Buoyancy|AutoPontoon",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.00", ClampMax = "0.45", UIMin = "0.00", UIMax = "0.35"))
	float AutoPontoonInsetFraction = 0.18f;

	/** Offsets generated pontoon centers vertically in local space. Negative values sink them deeper under the hull. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Buoyancy|AutoPontoon",
		meta = (AllowPrivateAccess = "true", ClampMin = "-100.0", ClampMax = "100.0", UIMin = "-25.0", UIMax = "25.0"))
	float AutoPontoonDepthBias = -6.0f;

	/** Higher values make the hull displace more water and ride higher. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Buoyancy",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.01", ClampMax = "5.00", UIMin = "0.05", UIMax = "2.00", SliderExponent = "2.0"))
	float DensityScale = 1.35f;

	/** Applies first-order linear resistance while the hull is moving through water. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Buoyancy",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "200.0", UIMin = "0.0", UIMax = "80.0", SliderExponent = "2.0"))
	float LinearDrag = 28.0f;

	/** Applies second-order drag at higher water speeds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Buoyancy",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "2.0", UIMin = "0.0", UIMax = "0.5", SliderExponent = "2.0"))
	float QuadraticDrag = 0.08f;

	/** Resists rolling and yawing while the hull is floating. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Buoyancy",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "20.0", UIMin = "0.0", UIMax = "6.0", SliderExponent = "2.0"))
	float AngularDrag = 2.0f;

	/** Caps how much upward force each update can apply across the hull. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Buoyancy",
		meta = (AllowPrivateAccess = "true", ClampMin = "1000.0", ClampMax = "10000000.0", UIMin = "10000.0", UIMax = "1000000.0", SliderExponent = "2.0"))
	float MaxBuoyantForce = 800000.0f;

	/** Enables downstream/current forces when the boat overlaps a river water body. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Buoyancy|River", meta = (AllowPrivateAccess = "true"))
	bool bApplyRiverForces = true;

	/** Multiplies river current velocity so drift is visible on lightweight demo hulls. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Buoyancy|River",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "5.0", UIMin = "0.0", UIMax = "2.0", SliderExponent = "2.0"))
	float RiverVelocityStrength = 1.0f;

	/** Caps the maximum downstream push a river can apply to the boat. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Buoyancy|River",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "200000.0", UIMin = "0.0", UIMax = "60000.0", SliderExponent = "2.0"))
	float MaxRiverForce = 30000.0f;

	/** Rotates the hull so it tends to align with the downstream river direction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Buoyancy|River", meta = (AllowPrivateAccess = "true"))
	bool bAlignToRiverFlow = true;

	/** Controls how aggressively the hull yaws to follow downstream flow. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Buoyancy|River",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "0.5", SliderExponent = "2.0"))
	float RiverAlignmentStrength = 0.10f;
};
