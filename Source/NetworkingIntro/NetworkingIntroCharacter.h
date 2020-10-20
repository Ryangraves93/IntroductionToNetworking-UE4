// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/TextRenderComponent.h"
#include "Bomb.h"
#include "NetworkingIntroCharacter.generated.h"

UCLASS(config=Game)
class ANetworkingIntroCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
public:
	ANetworkingIntroCharacter();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	//The health of the character
	UPROPERTY(VisibleAnywhere, Transient , ReplicatedUsing = OnRep_Health, Category = Stats)
	float Health;

	//The max health of the character
	UPROPERTY(EditAnywhere, Category = Stats)
	float MaxHealth = 100.f;

	//The max number of bombs that a character can have
	UPROPERTY(VisibleAnywhere, Transient, ReplicatedUsing = OnRep_BombCount, Category = Stats)
	int32 BombCount;

	UPROPERTY(VisibleAnywhere)
	int32 MaxBombCount = 3;

	//Text render component - used instead of UMG for convenience sake
	UPROPERTY(VisibleAnywhere)
	UTextRenderComponent* CharText;

	//Bomb blueprint
	UPROPERTY(EditAnywhere)
	TSubclassOf<ABomb> BombActorBp;
protected:

	/** Resets HMD orientation in VR. */
	void OnResetVR();

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	//Marks the properties we wish to replace
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void BeginPlay() override;

	/** Applies damage to the character */
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
private:
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerTakeDamage(float damage, struct FDamageEvent const& DamageEvent, AController* EventInstrigator, AActor* DamageCauser);

	/** Contains the actual implementation of the ServerTakeDamage function */
	void ServerTakeDamage_Implementation(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

	//Validates the client. If the result is false the client will be disconnected
	bool ServerTakeDamage_Validate(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

	//Bomb relate functions

	//Will try to spawn a bomb
	void AttemptToSpawnBomb();

	bool HasBombs() { return BombCount > 0; }

	//Spawns a bomb. Call this function when authorized to. If not authorized use the serverSpawnBomb function
	void SpawnBomb();

	//SpawnBomb Server version. Call this instead of SpawnBomb when you're a client.Automaticall calls ServerTakeDamage_Implementation function
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSpawnBomb();

	//Actual Implementation of ServerSpawnBomb
	void ServerSpawnBomb_Implementation();

	//Validates the client. If the result is false the client will be disconnected
	bool ServerSpawnBomb_Validate();

	//Called when health variable gets updated
	UFUNCTION()
	void OnRep_Health();

	//Called when the BombCount variable gets updated
	UFUNCTION()
	void OnRep_BombCount();

	//Initializes health
	void InitHealth();

	//Initializes bomb count
	void InitBombCount();

	//Updates the characters text to match with the updated stats
	void UpdateCharText();
};
