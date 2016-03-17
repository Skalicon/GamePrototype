// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Test.h"
#include "Net/UnrealNetwork.h"
#include "TestCharacter.h"

//////////////////////////////////////////////////////////////////////////
// ATestCharacter

ATestCharacter::ATestCharacter()
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
	CameraBoom->AttachTo(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->AttachTo(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm


	WeaponAttachPoint = TEXT("WeaponSocket");
	GetMesh()->bEnablePhysicsOnDedicatedServer = true;
	
	

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

void ATestCharacter::BeginPlay()
{
	Super::BeginPlay();

	//Something
	UWorld* const World = GetWorld();
	if (World) {
		AWeapon* sword = World->SpawnActor<AWeapon>(AWeapon::StaticClass());
		if (sword) {
			//Do things
			AddWeapon(sword);
		}
	}
}



//////////////////////////////////////////////////////////////////////////
// Input

void ATestCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	// Set up gameplay key bindings
	check(InputComponent);
	InputComponent->BindAction("Jump", IE_Pressed, this, &ATestCharacter::Jump);
	InputComponent->BindAction("Jump", IE_Released, this, &ATestCharacter::StopJumping);

	InputComponent->BindAction("Attack", IE_Pressed, this, &ATestCharacter::Attack);
	InputComponent->BindAction("Attack", IE_Released, this, &ATestCharacter::StopAttacking);

	InputComponent->BindAxis("MoveForward", this, &ATestCharacter::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &ATestCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	InputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	InputComponent->BindAxis("TurnRate", this, &ATestCharacter::TurnAtRate);
	InputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	InputComponent->BindAxis("LookUpRate", this, &ATestCharacter::LookUpAtRate);

	// handle touch devices
	InputComponent->BindTouch(IE_Pressed, this, &ATestCharacter::TouchStarted);
	InputComponent->BindTouch(IE_Released, this, &ATestCharacter::TouchStopped);
}

void ATestCharacter::Attack()
{
	CurrentWeapon->StartAttacking();
}

void ATestCharacter::StopAttacking()
{
	CurrentWeapon->StopAttacking();
}


void ATestCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	// jump, but only on the first touch
	if (FingerIndex == ETouchIndex::Touch1) {
		Jump();
	}
}

void ATestCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	if (FingerIndex == ETouchIndex::Touch1) {
		StopJumping();
	}
}

void ATestCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ATestCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ATestCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f)) {
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ATestCharacter::MoveRight(float Value)
{
	if ( (Controller != NULL) && (Value != 0.0f) ) {
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void ATestCharacter::SpawnDefaultInventory()
{
	if (Role < ROLE_Authority) {
		return;
	}

	for (int32 i = 0; i < DefaultInventoryClasses.Num(); i++) {
		if (DefaultInventoryClasses[i]) {
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.bNoCollisionFail = true;
			AWeapon* NewWeapon = GetWorld()->SpawnActor<AWeapon>(DefaultInventoryClasses[i], SpawnInfo);

			AddWeapon(NewWeapon);
		}
	}
}

void ATestCharacter::DestroyInventory()
{
	if (Role < ROLE_Authority) {
		return;
	}

	for (int32 i = Inventory.Num() - 1; i >= 0; i--) {
		AWeapon* Weapon = Inventory[i];
		if (Weapon) {
			RemoveWeapon(Weapon, true);
		}
	}
}


void ATestCharacter::SetCurrentWeapon(class AWeapon* NewWeapon, class AWeapon* LastWeapon)
{
	/* Maintain a reference for visual weapon swapping */
	PreviousWeapon = LastWeapon;

	AWeapon* LocalLastWeapon = nullptr;
	if (LastWeapon) {
		LocalLastWeapon = LastWeapon;
	} else if (NewWeapon != CurrentWeapon) {
		LocalLastWeapon = CurrentWeapon;
	}

	// UnEquip the current
	bool bHasPreviousWeapon = false;
	if (LocalLastWeapon) { 
		bHasPreviousWeapon = true;
	}

	CurrentWeapon = NewWeapon;

	if (NewWeapon) {
		NewWeapon->SetOwningPawn(this);

		// Don't care about this right now ::TODO::

		/* Only play equip animation when we already hold an item in hands */
		//NewWeapon->OnEquip(bHasPreviousWeapon);
	}

	/* NOTE: If you don't have an equip animation w/ animnotify to swap the meshes halfway through, then uncomment this to immediately swap instead */
	//SwapToNewWeaponMesh();
}


void ATestCharacter::OnRep_CurrentWeapon(AWeapon* LastWeapon)
{
	SetCurrentWeapon(CurrentWeapon, LastWeapon);
}


AWeapon* ATestCharacter::GetCurrentWeapon() const
{
	return CurrentWeapon;
}


void ATestCharacter::EquipWeapon(AWeapon* Weapon)
{
	if (Weapon) {
		/* Ignore if trying to equip already equipped weapon */
		if (Weapon == CurrentWeapon) {
			return;
		}

		if (Role == ROLE_Authority) {
			SetCurrentWeapon(Weapon, CurrentWeapon);
		} else {
			ServerEquipWeapon(Weapon);
		}
	}
}


bool ATestCharacter::ServerEquipWeapon_Validate(AWeapon* Weapon)
{
	return true;
}


void ATestCharacter::ServerEquipWeapon_Implementation(AWeapon* Weapon)
{
	EquipWeapon(Weapon);
}


void ATestCharacter::AddWeapon(class AWeapon* Weapon)
{
	if (Weapon && Role == ROLE_Authority) {
		Weapon->SetActorEnableCollision(false);
		Weapon->OnEnterInventory(this);
		Inventory.AddUnique(Weapon);
		Mesh->MoveIgnoreActors.Add(Weapon);
		// Equip first weapon in inventory
		if (Inventory.Num() > 0 && CurrentWeapon == nullptr) {
			EquipWeapon(Inventory[0]);
		}
	}
}


void ATestCharacter::RemoveWeapon(class AWeapon* Weapon, bool bDestroy)
{
	if (Weapon && Role == ROLE_Authority) {
		bool bIsCurrent = CurrentWeapon == Weapon;

		if (Inventory.Contains(Weapon)) {
			Weapon->OnLeaveInventory();
		}
		Inventory.RemoveSingle(Weapon);

		/* Replace weapon if we removed our current weapon */
		if (bIsCurrent && Inventory.Num() > 0) {
			SetCurrentWeapon(Inventory[0]);
		}

		/* Clear reference to weapon if we have no items left in inventory */
		if (Inventory.Num() == 0) {
			SetCurrentWeapon(nullptr);
		}

		if (bDestroy) {
			Weapon->Destroy();
		}
	}
}

void ATestCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Value is already updated locally, skip in replication step
	//DOREPLIFETIME_CONDITION(ATestCharacter, bIsJumping, COND_SkipOwner);

	DOREPLIFETIME(ATestCharacter, CurrentWeapon);
	DOREPLIFETIME(ATestCharacter, Inventory);
	/* If we did not display the current inventory on the player mesh we could optimize replication by using this replication condition. */
	/* DOREPLIFETIME_CONDITION(ASCharacter, Inventory, COND_OwnerOnly);*/
}