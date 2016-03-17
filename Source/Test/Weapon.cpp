// Fill out your copyright notice in the Description page of Project Settings.

#include "Test.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "TestCharacter.h"
#include "Weapon.h"

// Sets default values
AWeapon::AWeapon()
{
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh3P"));
	Mesh->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
	Mesh->bChartDistanceFactor = true;
	Mesh->bReceivesDecals = true;
	Mesh->CastShadow = true;
	Mesh->bEnablePhysicsOnDedicatedServer = true;
	//Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	//Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	//Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	//Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	RootComponent = Mesh;

	bIsEquipped = false;
	CurrentState = EWeaponState::Idle;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	SetReplicates(true);
	bNetUseOwnerRelevancy = true;

	bIsAttacking = false;

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Setup of visual mesh for weapon
	static ConstructorHelpers::FObjectFinder<UStaticMesh> StaticMeshOb_AW2(TEXT("StaticMesh'/Game/ThirdPerson/Meshes/albion_sword.albion_sword'"));
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMeshComponent"));
	WeaponMesh->SetStaticMesh(StaticMeshOb_AW2.Object);
	WeaponMesh->AttachParent = RootComponent;

	// Setup hit detection nodes for our ray traces in attack

	numberOfHitDetectionNodes = 0;

	for (int It = 0; It < numberOfHitDetectionNodes; It++) {
		HitDetectionNodes.Add(new HitDetectionNode(It, WeaponMesh->GetSocketLocation(GetHitNodeSocketName(It))));
	}

	HitDetectionActorsToIgnore.Add(this);
	HitDetectionActorsToIgnore.Add(MyPawn);
}

void AWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

// Called when the game starts or when spawned
void AWeapon::BeginPlay()
{
	Super::BeginPlay();
}


void AWeapon::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

// Called every frame
void AWeapon::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );
	if (bIsAttacking) {
		Attack();
	}
}

void AWeapon::Attack()
{
	if (Role < ROLE_Authority) {
		ServerAttack();
	} else {
		for (HitDetectionNode* node : HitDetectionNodes) {
			FVector currentSocketLocation = WeaponMesh->GetSocketLocation(GetHitNodeSocketName(node->socketIndex));
			if (currentSocketLocation != node->socketLastTickLocation) {
				FHitResult hitResult;
				
				static FName FireTraceIdent = FName(TEXT("WeaponTrace"));
				FCollisionQueryParams TraceParams(FireTraceIdent, true, this);
				TraceParams.bTraceAsyncScene = true;
				TraceParams.AddIgnoredActors(HitDetectionActorsToIgnore);
				TraceParams.AddIgnoredComponent(MyPawn->GetCapsuleComponent());

				this->GetWorld()->LineTraceSingleByChannel(hitResult, node->socketLastTickLocation, currentSocketLocation, ECC_Pawn, TraceParams);
				DrawDebugLine(this->GetWorld(), node->socketLastTickLocation, currentSocketLocation, FColor::Red, false, 10.f);

				// Check results of raycast here
				if (hitResult.bBlockingHit) {
					ATestCharacter* hitActor = Cast<ATestCharacter>(hitResult.GetActor());
					if (hitActor) {
						FHitResult hitResult2;
						if (hitActor->Mesh->LineTraceComponent(hitResult2, node->socketLastTickLocation, currentSocketLocation, TraceParams)) {
							bIsAttacking = false;
							UE_LOG(LogTemp, Warning, TEXT("Node %d collided with %s"), node->socketIndex, *hitResult2.BoneName.ToString());
						}
					}
				}
				node->socketLastTickLocation = currentSocketLocation;
			}
		}
	}
}

bool AWeapon::ServerAttack_Validate()
{
	return true;
}

void AWeapon::ServerAttack_Implementation()
{
	Attack();
}

void AWeapon::StartAttacking()
{
	bIsAttacking = true;
	for (HitDetectionNode* node : HitDetectionNodes) {
		node->socketLastTickLocation = WeaponMesh->GetSocketLocation(GetHitNodeSocketName(node->socketIndex));
	}
}

void AWeapon::StopAttacking()
{
	bIsAttacking = false;
}

/*
Return Mesh of Weapon
*/
USkeletalMeshComponent* AWeapon::GetWeaponMesh() const
{
	return Mesh;
}


class ATestCharacter* AWeapon::GetPawnOwner() const
{
	return MyPawn;
}


void AWeapon::SetOwningPawn(ATestCharacter* NewOwner)
{
	if (MyPawn != NewOwner) {
		Instigator = NewOwner;
		MyPawn = NewOwner;
		// Net owner for RPC calls.
		SetOwner(NewOwner);
	}
}


void AWeapon::OnRep_MyPawn()
{
	if (MyPawn) {
		OnEnterInventory(MyPawn);
	} else {
		OnLeaveInventory();

	}
}


void AWeapon::AttachMeshToPawn()
{
	if (MyPawn) {
		// Remove and hide
		DetachMeshFromPawn();

		USkeletalMeshComponent* PawnMesh = MyPawn->GetMesh();
		FName AttachPoint = TEXT("AttachPoint");
		Mesh->SetHiddenInGame(false);
		//Mesh->SetRelativeTransform(FTransform(Mesh->GetSocketQuaternion("HandleGrip"), Mesh->GetSocketLocation("HandleGrip")));
		Mesh->AttachTo(PawnMesh, AttachPoint, EAttachLocation::SnapToTarget);
	}
}


void AWeapon::DetachMeshFromPawn()
{
	Mesh->DetachFromParent();
	Mesh->SetHiddenInGame(true);
}

void AWeapon::OnEnterInventory(ATestCharacter* NewOwner)
{
	SetOwningPawn(NewOwner);
	AttachMeshToPawn();
}


void AWeapon::OnLeaveInventory()
{
	if (Role == ROLE_Authority) {
		SetOwningPawn(nullptr);
	}

	DetachMeshFromPawn();
}


bool AWeapon::IsEquipped() const
{
	return bIsEquipped;
}


bool AWeapon::IsAttachedToPawn() const // TODO: Review name to more accurately specify meaning.
{
	return bIsEquipped || bPendingEquip;
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, MyPawn);

	//DOREPLIFETIME_CONDITION(AWeapon, CurrentAmmo, COND_OwnerOnly);
	//DOREPLIFETIME_CONDITION(AWeapon, CurrentAmmoInClip, COND_OwnerOnly);
	//DOREPLIFETIME_CONDITION(AWeapon, BurstCounter, COND_SkipOwner);
	//DOREPLIFETIME_CONDITION(AWeapon, bPendingReload, COND_SkipOwner);
}

FName AWeapon::GetHitNodeSocketName(int index)
{
	FString socketName = FString(TEXT("HitNode"));
	socketName += FString::FromInt(index);
	return FName(*socketName);
}