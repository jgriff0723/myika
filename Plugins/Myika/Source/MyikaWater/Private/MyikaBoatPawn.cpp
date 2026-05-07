// Copyright (c) Myika AI. All rights reserved.

#include "MyikaBoatPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "MyikaBuoyancyComponent.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyikaBoatPawn, Log, All);

AMyikaBoatPawn::AMyikaBoatPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	HullMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HullMesh"));
	SetRootComponent(HullMesh);
	HullMesh->SetCanEverAffectNavigation(false);
	HullMesh->SetRelativeScale3D(FVector(3.25f, 1.35f, 0.55f));

	// Physics-touching setters (SetSimulatePhysics, SetCollisionProfileName,
	// SetNotifyRigidBodyCollision) call FBodyInstance::GetSimplePhysicalMaterial,
	// which logs "GEngine not initialized" during native CDO construction and
	// can escalate to a stack overflow when a derived BP is opened in the editor.
	// Defer to non-CDO instances; final flags are reasserted in RefreshHullSetup
	// during PostInitProperties / OnConstruction / BeginPlay.
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		HullMesh->SetSimulatePhysics(true);
		HullMesh->SetEnableGravity(true);
		HullMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
		HullMesh->SetNotifyRigidBodyCollision(true);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		HullMesh->SetStaticMesh(CubeMesh.Object);
	}

	BuoyancyComponent = CreateDefaultSubobject<UMyikaBuoyancyComponent>(TEXT("MyikaBuoyancy"));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(HullMesh);
	CameraBoom->TargetArmLength = 650.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 6.0f;
	CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> DefaultIMC(TEXT("/Game/Input/IMC_Default.IMC_Default"));
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> MouseLookIMC(TEXT("/Game/Input/IMC_MouseLook.IMC_MouseLook"));
	static ConstructorHelpers::FObjectFinder<UInputAction> MoveIA(TEXT("/Game/Input/Actions/IA_Move.IA_Move"));
	static ConstructorHelpers::FObjectFinder<UInputAction> LookIA(TEXT("/Game/Input/Actions/IA_Look.IA_Look"));
	static ConstructorHelpers::FObjectFinder<UInputAction> MouseLookIA(TEXT("/Game/Input/Actions/IA_MouseLook.IA_MouseLook"));

	DefaultInputMappingContext = DefaultIMC.Object;
	MouseLookMappingContext = MouseLookIMC.Object;
	MoveAction = MoveIA.Object;
	LookAction = LookIA.Object;
	MouseLookAction = MouseLookIA.Object;

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		RefreshHullSetup();
	}
}

void AMyikaBoatPawn::BeginPlay()
{
	Super::BeginPlay();

	RefreshHullSetup();
	if (BuoyancyComponent)
	{
		BuoyancyComponent->RebuildPontoons();
	}
	EnsureInputMappings();
}

void AMyikaBoatPawn::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RefreshHullSetup();
	if (BuoyancyComponent)
	{
		BuoyancyComponent->RebuildPontoons();
	}
}

void AMyikaBoatPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyikaBoatPawn::HandleMove);
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AMyikaBoatPawn::HandleMoveCompleted);
		}

		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMyikaBoatPawn::HandleLook);
		}

		if (MouseLookAction)
		{
			EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AMyikaBoatPawn::HandleLook);
		}
	}
	else
	{
		UE_LOG(LogMyikaBoatPawn, Warning, TEXT("%s expected an Enhanced Input component for boat controls."), *GetPathName());
	}
}

void AMyikaBoatPawn::RefreshHullSetup()
{
	if (!HullMesh)
	{
		return;
	}

	HullMesh->SetLinearDamping(HullLinearDamping);
	HullMesh->SetAngularDamping(HullAngularDamping);
	HullMesh->SetMassOverrideInKg(NAME_None, HullMassKg, true);
	HullMesh->SetCenterOfMass(FVector(0.0f, 0.0f, CenterOfMassOffsetZ));
}

void AMyikaBoatPawn::EnsureInputMappings()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		if (DefaultInputMappingContext)
		{
			Subsystem->RemoveMappingContext(DefaultInputMappingContext);
			Subsystem->AddMappingContext(DefaultInputMappingContext, 0);
		}

		if (MouseLookMappingContext)
		{
			Subsystem->RemoveMappingContext(MouseLookMappingContext);
			Subsystem->AddMappingContext(MouseLookMappingContext, 1);
		}
	}
}

void AMyikaBoatPawn::HandleMove(const FInputActionValue& Value)
{
	const FVector2D MoveVector = Value.Get<FVector2D>();
	ApplyThrottle(MoveVector.Y);
	ApplySteering(MoveVector.X);
}

void AMyikaBoatPawn::HandleMoveCompleted(const FInputActionValue& Value)
{
	ApplyThrottle(0.0f);
	ApplySteering(0.0f);
}

void AMyikaBoatPawn::HandleLook(const FInputActionValue& Value)
{
	const FVector2D LookVector = Value.Get<FVector2D>();
	AddControllerYawInput(LookVector.X);
	AddControllerPitchInput(LookVector.Y);
}

void AMyikaBoatPawn::ApplyThrottle(float Value)
{
	if (!HullMesh || FMath::IsNearlyZero(Value))
	{
		return;
	}

	const float ForceMagnitude = Value >= 0.0f ? ForwardThrust * Value : ReverseThrust * -Value;
	HullMesh->AddForce(HullMesh->GetForwardVector() * ForceMagnitude, NAME_None, true);
}

void AMyikaBoatPawn::ApplySteering(float Value)
{
	if (!HullMesh || FMath::IsNearlyZero(Value))
	{
		return;
	}

	const float Speed = HullMesh->GetComponentVelocity().Size();
	const float SteeringScale = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 900.0f), FVector2D(0.35f, 1.0f), Speed);
	HullMesh->AddTorqueInDegrees(FVector(0.0f, 0.0f, Value * SteeringTorque * SteeringScale), NAME_None, true);
}
