// Copyright (c) Myika AI. All rights reserved.
//
// Artist-authored cloud presets for the MyikaSky runtime material instance.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MyikaCloudPreset.generated.h"

UCLASS(BlueprintType)
class MYIKASKY_API UMyikaCloudPreset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Overall cloud density 0..1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Clouds", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", SliderExponent = "2.0"))
	float Coverage = 0.4f;

	/** Cloud-shape blend from wispy to puffy. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Clouds", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", SliderExponent = "2.0"))
	float Morphology = 0.5f;

	/** Wind-driven cloud animation speed multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Clouds", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", SliderExponent = "2.0"))
	float WindStrength = 0.1f;

	/** Light absorption through the cloud volume. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Clouds", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", SliderExponent = "2.0"))
	float Extinction = 0.5f;

	/** High-frequency noise contribution. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Clouds", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", SliderExponent = "2.0"))
	float Detail = 0.5f;

	/** World-space advection direction for cloud motion. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Clouds", meta = (MakeEditWidget = "true"))
	FVector WindDirection = FVector(1.0f, 0.0f, 0.0f);

	/** Base tint applied to the cloud material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky|Clouds")
	FLinearColor BaseColor = FLinearColor::White;
};
