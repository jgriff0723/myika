// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"
#include "WaterBodyOceanActor.h"
#include "MyikaOceanActor.generated.h"

class UChildActorComponent;
class UMaterialInterface;
struct FPropertyChangedEvent;

UCLASS(BlueprintType, Blueprintable, ClassGroup = (Myika), meta = (DisplayName = "Myika Ocean"))
class MYIKAWATER_API AMyikaOcean : public AWaterBodyOcean
{
	GENERATED_BODY()

public:
	AMyikaOcean(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** Apply the configured default water material to the UE Water body component. */
	UFUNCTION(BlueprintCallable, Category = "Myika|Water|Surface")
	bool ApplyDefaultWaterMaterial();

	/** Rebuild the child caustic actor after extent or placement changes. */
	UFUNCTION(BlueprintCallable, Category = "Myika|Water|Caustics")
	void RefreshCausticChild();

protected:
	/** Soft reference to the default ocean material instance used by the water body component. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Myika|Water|Surface", meta = (AllowedClasses = "/Script/Engine.MaterialInterface"))
	TSoftObjectPtr<UMaterialInterface> WaterMaterialAsset;

	/** Spawn a projected caustic child actor under the ocean body. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Myika|Water|Caustics")
	TObjectPtr<UChildActorComponent> CausticActorComponent;

	/** Whether this ocean should spawn its companion caustic decal automatically. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Caustics")
	bool bAutoSpawnCaustics = true;

	/** Scales the collision-derived projection box so large oceans can exaggerate the decal reach. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Caustics",
		meta = (ClampMin = "0.25", ClampMax = "4.0", UIMin = "0.5", UIMax = "2.0", SliderExponent = "2.0"))
	float CausticExtentScale = 1.0f;

	/** Offsets the caustic projector downward from the water surface. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Water|Caustics",
		meta = (ClampMin = "-50000.0", ClampMax = "50000.0", UIMin = "-5000.0", UIMax = "5000.0"))
	float CausticVerticalOffset = -200.0f;

private:
	FVector ResolveCausticExtent() const;

	// Re-entry guards: SetWaterMaterial / CreateChildActor / SetMobility can each
	// trigger PostEditChangeProperty or further OnConstruction passes that re-enter
	// these helpers, blowing the stack when AMyikaOcean is opened in the editor.
	bool bApplyingDefaultMaterial = false;
	bool bRefreshingCausticChild = false;
};
