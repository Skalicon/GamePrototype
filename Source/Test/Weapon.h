// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "HitDetectionNode.h"
#include "Net/UnrealNetwork.h"
#include "Engine.h"
#include "Weapon.generated.h"

UENUM()
enum class EWeaponState
{
	Idle,
	Firing,
	Equipping,
	Reloading
};

UENUM()
enum class EInventorySlot : uint8
{
	/* For currently equipped items/weapons */
	Hands,

	/* For primary weapons on spine bone */
	Primary,

	/* Storage for small items like flashlight on pelvis */
	Secondary,
};

UCLASS(Blueprintable)
class TEST_API AWeapon : public AActor
{
	GENERATED_BODY()
	virtual void PostInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	float GetEquipStartedTime() const;
	float GetEquipDuration() const;

	/** last time when this weapon was switched to */
	float EquipStartedTime;
	/** how much time weapon needs to be equipped */
	float EquipDuration;
	bool bIsEquipped;
	bool bPendingEquip;
	EWeaponState CurrentState;
	class UStaticMeshComponent* WeaponMesh;
	
	void Attack();
	UFUNCTION(reliable, server, WithValidation)
	void ServerAttack();
	virtual bool ServerAttack_Validate();
	virtual void ServerAttack_Implementation();
	
	int numberOfHitDetectionNodes;
	bool bIsAttacking;

	TArray<HitDetectionNode*> HitDetectionNodes;
	TArray <AActor*> HitDetectionActorsToIgnore;


public:	
	// Sets default values for this actor's properties
	AWeapon();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	/* You can assign default values to function parameters, these are then optional to specify/override when calling the function. */
	void AttachMeshToPawn();

	EWeaponState GetCurrentState() const;

	/** get weapon mesh (needs pawn owner to determine variant) */
	UFUNCTION(BlueprintCallable, Category = "Game|Weapon")
		USkeletalMeshComponent* GetWeaponMesh() const;


	void OnEquip(bool bPlayAnimation);

	/* Set the weapon's owning pawn */
	void SetOwningPawn(ATestCharacter* NewOwner);

	/* Get pawn owner */
	UFUNCTION(BlueprintCallable, Category = "Game|Weapon")
	class ATestCharacter* GetPawnOwner() const;
	virtual void OnEnterInventory(ATestCharacter* NewOwner);
	virtual void OnLeaveInventory();

	FORCEINLINE EInventorySlot GetStorageSlot()
	{
		return StorageSlot;
	}

	void StartAttacking();
	void StopAttacking();

protected:

	/** weapon mesh: 3rd person view */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USkeletalMeshComponent* Mesh;

	/* The character socket to store this item at. */
	EInventorySlot StorageSlot;

	/** pawn owner */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_MyPawn)
	class ATestCharacter* MyPawn;

	UFUNCTION()
	void OnRep_MyPawn();

	/** detaches weapon mesh from pawn */
	void DetachMeshFromPawn();

	bool IsEquipped() const;

	bool IsAttachedToPawn() const;

	FName GetHitNodeSocketName(int);

};
