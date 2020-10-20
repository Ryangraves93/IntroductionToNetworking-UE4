// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "Bomb.generated.h"

UCLASS()
class NETWORKINGINTRO_API ABomb : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABomb();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//Static mesh of the comp
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* SM;

	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* ProjectileMovementComp;

	//Sphere comp used for collision. Movement component needs a collision component as a root to function properly
	UPROPERTY(VisibleAnywhere)
	USphereComponent* SphereComp;

	//Delay until explosion
	UPROPERTY(EditAnywhere, Category = BombProps)
	float FuseTime = 2.5f;

	UPROPERTY(EditAnywhere, Category = BombProps)
	float ExplosionRadius = 200.f;

	UPROPERTY(EditAnywhere, Category = BombProps)
	float ExplosionDamage = 25.f;

	//The particle system of the explosion
	UPROPERTY(EditAnywhere)
	UParticleSystem* ExplosionFX;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_IsArmed)
	bool bIsArmed = false;

	//Called when bIsArmed gets updated
	UFUNCTION()
	void OnRep_IsArmed();

	//Arms the bomb for the explosion
	void ArmBomb();

	//Called when the bomb bounces
	UFUNCTION()
	void OnProjectileBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

	//Performs an explosion after a certain amount of time
	void PerformDelayedExplosion(float ExplosionDelay);

	//Performs an explosion when called
	UFUNCTION()
	void Explode();

	//Simulate explosion functions

	//The multicast specifier, indicates that every client will call the SimulateExplosionFX_Implementation.
	//No need to generate an implementation for this function

	UFUNCTION(Reliable, NetMulticast)
	void SimulateExplosionFX();

	//Actual implementation of the SimulateExplosionFX
	void SimulateExplosionFX_Implementation();

};
