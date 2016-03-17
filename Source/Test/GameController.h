// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/WorldSettings.h"
#include "TestCharacter.h"
#include "GameController.generated.h"


/**
 * 
 */
UCLASS()
class TEST_API AGameController : public AWorldSettings
{
	GENERATED_BODY()

public:
	AGameController();
	
	TArray <ATestCharacter*> Players;
};
