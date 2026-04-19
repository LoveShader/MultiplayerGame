// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/BlasterCharacter.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

class ABlasterCharacter;
class ABlasterPlayerController;
class AHitScanWeapon;

USTRUCT(BlueprintType)
struct FBoxInformation
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY()
	FVector BoxExtent = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()

	UPROPERTY()
	float Time = 0.f;

	UPROPERTY()
	TMap<FName, FBoxInformation> HitBoxInfo;

	UPROPERTY()
	ABlasterCharacter* Character;
};

USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHitConfirmed;

	UPROPERTY()
	bool bHeadShot;
};

USTRUCT(BlueprintType)
struct FShotgunServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<ABlasterCharacter*, uint32> HeadShots;

	UPROPERTY()
	TMap<ABlasterCharacter*, uint32> BodyShots;
};


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MULTIPLAYERGAME_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	friend class ABlasterCharacter;
	
	ULagCompensationComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// HitScan Weapon
	UFUNCTION(Server, Reliable)
	void ServerScoreRequest(
		ABlasterCharacter* HitCharacter,
		float HitTime,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitTarget,
		AHitScanWeapon* DamageCauser
	);

	//Shotgun Weapon
	UFUNCTION(Server, Reliable)
	void ShotgunServerScoreRequest(
		const TArray<ABlasterCharacter*>& HitCharacters,
		float HitTime,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitTargets
	);
protected:
	virtual void BeginPlay() override;
	FFramePackage GetFrameToCheck(ABlasterCharacter* HitCharacter, float HitTime);	
	//Server Side Rewind for HitScan Weapons except Shotgun
	FServerSideRewindResult ServerSideRewind(
		ABlasterCharacter* HitCharacter,
		float HitTime,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitTarget
	);
	// Server side Rewind for Projectile Weapon
	FServerSideRewindResult ProjectileServerSideRewind(
		ABlasterCharacter* HitCharacter,
		float HitTime,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity
		);
	
	//Shotgun Server side Rewind
	FShotgunServerSideRewindResult ShotgunServerSideRewind(
		TArray<ABlasterCharacter*> HitCharacters,
		float HitTime,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitTargets
	);

	// HitScan Confirm Hit
	FServerSideRewindResult ConfirmHit(const FFramePackage& Package, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitTarget);
	// Projectile Confirm Hit
	FServerSideRewindResult ProjectileConfirmHit(const FFramePackage& Package, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime);
	// Shotgun Confirm Hit
	FShotgunServerSideRewindResult ShotgunConfirmHit(const TArray<FFramePackage>& FramesToCheck, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitTargets);
	void CacheBoxPositions(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage);
	void MoveBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package);
	void ResetHitBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package);
	void EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled);
private:
	void SaveFrameHistory();
	void SaveFramePackage(FFramePackage& Package);
	FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime) const;
	void ShowFramePackage(const FFramePackage& Package, const FColor& Color) const;
	
	UPROPERTY()
	ABlasterCharacter* Character = nullptr;

	UPROPERTY()
	ABlasterPlayerController* Controller = nullptr;

	TDoubleLinkedList<FFramePackage> FrameHistory;

	UPROPERTY(EditAnywhere, Category = "Lag Compensation")
	float MaxRecordTime = 4.f;
};
