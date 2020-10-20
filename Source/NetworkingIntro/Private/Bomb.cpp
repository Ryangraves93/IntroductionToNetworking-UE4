// Fill out your copyright notice in the Description page of Project Settings.

#include "Bomb.h"
#include "../NetworkingIntro.h"

// Sets default values
ABomb::ABomb()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SphereComp = CreateDefaultSubobject<USphereComponent>(FName("SphereComp"));

	SetRootComponent(SphereComp);

	SM = CreateDefaultSubobject<UStaticMeshComponent>(FName("SM"));
	SM->SetupAttachment(SphereComp);

	ProjectileMovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(FName("ProjectileMovementComp"));
	ProjectileMovementComp->bShouldBounce = true;

	//Need to replicate function so set as true
	SetReplicates(true);

}

// Called when the game starts or when spawned
void ABomb::BeginPlay()
{
	Super::BeginPlay();
	
	//Register the function that will be called in any bounce event
	ProjectileMovementComp->OnProjectileBounce.AddDynamic(this, &ABomb::OnProjectileBounce);
}

// Called every frame
void ABomb::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABomb::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//Tell the engine to replicate the bIsArmed variable
	DOREPLIFETIME(ABomb, bIsArmed);
}

void ABomb::OnRep_IsArmed()
{
	//Called when the comb is armed from the authority client
	if (bIsArmed)
	{
		ArmBomb();
	}
}

void ABomb::ArmBomb()
{
	if (bIsArmed)
	{
		//Change the base color of the static mesh to red
		UMaterialInstanceDynamic* DynamicMat = SM->CreateAndSetMaterialInstanceDynamic(0);

		DynamicMat->SetVectorParameterValue(FName("Color"), FLinearColor::Red);
	}
}

void ABomb::OnProjectileBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (!bIsArmed && HasAuthority())
	{
		bIsArmed = true;
		ArmBomb();

		PerformDelayedExplosion(FuseTime);
	}
}

void ABomb::PerformDelayedExplosion(float ExplosionDelay)
{
	FTimerHandle TimerHandle;

	FTimerDelegate TimerDel;
	TimerDel.BindUFunction(this, FName("Explode"));

	GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDel, ExplosionDelay, false);
}

void ABomb::Explode()
{
	SimulateExplosionFX();

	TSubclassOf<UDamageType> DmgType;
	//Do not ignore any actors
	TArray<AActor*> IgnoreActors;

	//Call the TakeDamageFunction that has been override in the character class
	UGameplayStatics::ApplyRadialDamage(GetWorld(), ExplosionDamage, GetActorLocation(), ExplosionRadius, DmgType, IgnoreActors, this, GetInstigatorController());

	FTimerHandle TimerHandle;
	FTimerDelegate TimerDel;

	TimerDel.BindLambda([&]()
		{
			Destroy();
		});

	GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDel, 0.3f, false);
}

void ABomb::SimulateExplosionFX_Implementation()
{
	if (ExplosionFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionFX, GetTransform(), true);
	}
}

