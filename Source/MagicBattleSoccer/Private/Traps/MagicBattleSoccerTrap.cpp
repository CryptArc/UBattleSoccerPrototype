
#include "MagicBattleSoccer.h"
#include "MagicBattleSoccerTrap.h"

AMagicBattleSoccerTrap::AMagicBattleSoccerTrap(const class FObjectInitializer& OI)
	: Super(OI)
{
	bReplicates = true;
	bNetUseOwnerRelevancy = true;
	this->SetActorTickEnabled(true);
	PrimaryActorTick.bCanEverTick = true;
}
