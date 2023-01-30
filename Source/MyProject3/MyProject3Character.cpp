// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyProject3Character.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include <Kismet/GameplayStatics.h>
#include <Kismet/KismetMathLibrary.h>

//////////////////////////////////////////////////////////////////////////
// AMyProject3Character

AMyProject3Character::AMyProject3Character()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void AMyProject3Character::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("ChangeMaterial", IE_Pressed, this, &AMyProject3Character::ChangeMaterialParams);

	PlayerInputComponent->BindAxis("MoveForward", this, &AMyProject3Character::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMyProject3Character::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AMyProject3Character::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AMyProject3Character::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AMyProject3Character::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AMyProject3Character::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AMyProject3Character::OnResetVR);
}


void AMyProject3Character::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> Cubes;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName(TEXT("Cube")), Cubes);

	if (Cubes.Num())
	{
		Cube = Cubes[0];
	}

	GetDynamicMaterialInstance();

	bDefault = true;
	ChangeMaterialParams();
}

void AMyProject3Character::OnResetVR()
{
	// If MyProject3 is added to a project via 'Add Feature' in the Unreal Editor the dependency on HeadMountedDisplay in MyProject3.Build.cs is not automatically propagated
	// and a linker error will result.
	// You will need to either:
	//		Add "HeadMountedDisplay" to [YourProject].Build.cs PublicDependencyModuleNames in order to build successfully (appropriate if supporting VR).
	// or:
	//		Comment or delete the call to ResetOrientationAndPosition below (appropriate if not supporting VR)
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AMyProject3Character::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void AMyProject3Character::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void AMyProject3Character::ChangeMaterialParams()
{
	TArray<float> rgba = { VectorParam.R, VectorParam.G, VectorParam.B, VectorParam.A};

	bool bIsAll1 = true;

	for (auto color : rgba)
	{
		if (color != 1)
		{
			bIsAll1 = false;
			break;
		}
	}

	bIsAll1 = bIsAll1 && (ScalarParam == 1);

	if (bIsAll1 || bDefault)
	{
		MaterialInstanceDynamic->SetTextureParameterValue(TEXT("TextureParam"), Textures[UKismetMathLibrary::RandomIntegerInRange(0, Textures.Num() - 1)]);
		rgba = { 0, 0, 0, 0 };
		ScalarParam = 0;
	}

	float delta = UKismetMathLibrary::RandomFloatInRange(0, 0.2);

	int ind;
	do
	{
		ind = UKismetMathLibrary::RandomIntegerInRange(0, 2);
	}
	while (rgba[ind] == 1);

	rgba[ind] = rgba[ind] + delta;
	if (rgba[ind] > 1)
	{
		rgba[ind] = 1;
	}

	rgba[3] = rgba[3] + delta;
	if (rgba[3] > 1)
	{
		rgba[3] = 1;
	}

	VectorParam = FLinearColor(rgba[0], rgba[1], rgba[2], rgba[3]);

	ScalarParam = ScalarParam + delta;
	if (ScalarParam > 1)
	{
		ScalarParam = 1;
	}

	MaterialInstanceDynamic->SetVectorParameterValue(TEXT("VectorParam"), VectorParam);
	MaterialInstanceDynamic->SetScalarParameterValue(TEXT("ScalarParam"), ScalarParam);

	bDefault = false;
}

void AMyProject3Character::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AMyProject3Character::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AMyProject3Character::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AMyProject3Character::GetDynamicMaterialInstance()
{
	UMaterial* Material = nullptr;
	UStaticMeshComponent* CubeMesh = Cast<UStaticMeshComponent>(Cube->GetComponentByClass(UStaticMeshComponent::StaticClass()));

	if (CubeMesh)
	{
		Material = CubeMesh->GetMaterial(0)->GetMaterial();
	}

	MaterialInstanceDynamic = UMaterialInstanceDynamic::Create(Material, this);

	if (CubeMesh && MaterialInstanceDynamic)
	{
		CubeMesh->SetMaterial(0, MaterialInstanceDynamic);
	}
}

void AMyProject3Character::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}
