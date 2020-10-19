// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetworkingIntroCharacter.h"
#include "NetworkingIntro.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

//////////////////////////////////////////////////////////////////////////
// ANetworkingIntroCharacter

ANetworkingIntroCharacter::ANetworkingIntroCharacter()
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

	CharText = CreateDefaultSubobject<UTextRenderComponent>(FName("CharText"));
	CharText->SetRelativeLocation(FVector(0, 0, 100));
	CharText->SetupAttachment(GetRootComponent());

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void ANetworkingIntroCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &ANetworkingIntroCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ANetworkingIntroCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ANetworkingIntroCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ANetworkingIntroCharacter::LookUpAtRate);

	PlayerInputComponent->BindAction("ThrowBomb", IE_Pressed, this, &ANetworkingIntroCharacter::AttemptToSpawnBomb);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &ANetworkingIntroCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &ANetworkingIntroCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &ANetworkingIntroCharacter::OnResetVR);
}

void ANetworkingIntroCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//Telling the engine to call the OnRep_Health and OnRep_BombCount each time
	//a variable changes
	DOREPLIFETIME(ANetworkingIntroCharacter, Health);
	DOREPLIFETIME(ANetworkingIntroCharacter, BombCount);
}

void ANetworkingIntroCharacter::BeginPlay()
{
	Super::BeginPlay();

	InitHealth();
	InitBombCount();
}

float ANetworkingIntroCharacter::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(Damage,DamageEvent,EventInstigator,DamageCauser);

	//Decrease the character's hp

	Health -= Damage;
	if (Health <= 0) InitHealth();

	//Call the update text on the local client
	//OnRep_Health will be called in every other client so the character's text
	//will contain a text with the right values
	UpdateCharText();
	return Health;
}

void ANetworkingIntroCharacter::ServerTakeDamage_Implementation(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
}

bool ANetworkingIntroCharacter::ServerTakeDamage_Validate(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	//Assume that everything is ok without any further checks and return true
	return true;
}

void ANetworkingIntroCharacter::AttemptToSpawnBomb()
{
	if (HasBombs())
	{
		//If we don't have authority, meaning that we're not the server
		//tell the server to spawn the bomb
		//If we're the server just spawn the bomb
		if (!HasAuthority())
		{
			ServerSpawnBomb();
		}
		else SpawnBomb();

		FDamageEvent DmgEvent;

		if (HasAuthority())
		{
			ServerTakeDamage(25.f, DmgEvent, GetController(), this);
		}
		else TakeDamage(25.f, DmgEvent, GetController(), this);
	}
}

void ANetworkingIntroCharacter::SpawnBomb()
{
	BombCount--;
	UpdateCharText();
}

void ANetworkingIntroCharacter::ServerSpawnBomb_Implementation()
{
	SpawnBomb();
}

bool ANetworkingIntroCharacter::ServerSpawnBomb_Validate()
{
	return true;
}

void ANetworkingIntroCharacter::OnRep_Health()
{
	UpdateCharText();
}

void ANetworkingIntroCharacter::OnRep_BombCount()
{
	UpdateCharText();
}

void ANetworkingIntroCharacter::InitHealth()
{
	Health = MaxHealth;
	UpdateCharText();
}

void ANetworkingIntroCharacter::InitBombCount()
{
	BombCount = MaxBombCount;
}

void ANetworkingIntroCharacter::UpdateCharText()
{
	//Create a string that will display the health and bomb count values
	FString NewText = FString("Health: ") + FString::SanitizeFloat(Health) + FString(" Bomb Count: ") + FString::FromInt(BombCount);

	//Set the created string to the text render comp
	CharText->SetText(FText::FromString(NewText));
}


void ANetworkingIntroCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void ANetworkingIntroCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void ANetworkingIntroCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void ANetworkingIntroCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ANetworkingIntroCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ANetworkingIntroCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ANetworkingIntroCharacter::MoveRight(float Value)
{
	if ( (Controller != NULL) && (Value != 0.0f) )
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
