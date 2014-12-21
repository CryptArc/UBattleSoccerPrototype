#pragma once

#include "MagicBattleSoccerWeapon.h"
#include "MagicBattleSoccerWeapon_Projectile.generated.h"

USTRUCT(BlueprintType)
struct FProjectileWeaponData
{
	GENERATED_USTRUCT_BODY()

	/** projectile class */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = WeaponStat)
	TSubclassOf<class AMagicBattleSoccerProjectile> ProjectileClass;

	/** life time */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = WeaponStat)
	float ProjectileLife;

	/** damage at impact point */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = WeaponStat)
	int32 ExplosionDamage;

	/** radius of damage */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = WeaponStat)
	float ExplosionRadius;

	/** type of damage */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = WeaponStat)
	TSubclassOf<UDamageType> DamageType;

	/** defaults */
	FProjectileWeaponData()
	{
		ProjectileClass = NULL;
		ProjectileLife = 10.0f;
		ExplosionDamage = 100;
		ExplosionRadius = 300.0f;
		DamageType = UDamageType::StaticClass();
	}
};

UCLASS(Abstract)
class MAGICBATTLESOCCER_API AMagicBattleSoccerWeapon_Projectile : public AMagicBattleSoccerWeapon
{
	GENERATED_UCLASS_BODY()

	/** apply config on projectile */
	void ApplyWeaponConfig(FProjectileWeaponData& Data);

	//////////////////////////////////////////////////////////////////////////
	// Input

	/** [local + server] sets the firing target */
	virtual void SetTargetLocationAdjustedForVelocity(FVector TargetLocation, FVector TargetVelocity);

protected:

	/** weapon config */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Soccer)
	FProjectileWeaponData ProjectileConfig;

	//////////////////////////////////////////////////////////////////////////
	// AI

	/** Returns how effective this weapon would be on scene actors in the world's current state */
	TArray<FWeaponActorEffectiveness> GetCurrentEffectiveness();

	//////////////////////////////////////////////////////////////////////////
	// Weapon usage

	/** [local] weapon specific fire implementation */
	virtual void FireWeapon() override;

	/** spawn projectile on server */
	UFUNCTION(reliable, server, WithValidation)
	void ServerFireProjectile(FVector Origin, FVector_NetQuantizeNormal ShootDir);
};
