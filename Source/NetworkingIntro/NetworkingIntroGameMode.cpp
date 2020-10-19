// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetworkingIntroGameMode.h"
#include "NetworkingIntroCharacter.h"
#include "UObject/ConstructorHelpers.h"

ANetworkingIntroGameMode::ANetworkingIntroGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
