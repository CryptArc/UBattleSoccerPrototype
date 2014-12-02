

#include "MagicBattleSoccer.h"
#include "MagicBattleSoccerAIController.h"
#include "MagicBattleSoccerSpawnPoint.h"
#include "MagicBattleSoccerGameState.h"
#include "MagicBattleSoccerGameMode.h"
#include "MagicBattleSoccerPlayerState.h"
#include "MagicBattleSoccerCharacter.h"
#include "MagicBattleSoccerGoal.h"
#include "MagicBattleSoccerBall.h"
#include "Engine/TriggerBox.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

AMagicBattleSoccerAIController::AMagicBattleSoccerAIController(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bWantsPlayerState = true;
	IsAttacking = false;
}

/** Spawns the character */
void AMagicBattleSoccerAIController::SpawnCharacter_Implementation()
{
	// Nothing to do here -- the blueprint should do all the work and it should only be done on the server
}

void AMagicBattleSoccerAIController::PawnPendingDestroy(APawn* inPawn)
{
	Super::PawnPendingDestroy(inPawn);

	// Stop the behaviour tree/logic
	UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(BrainComponent);
	BTComp->StopTree();

	if (nullptr != SpawnPoint)
	{
		SpawnPoint->SpawnedPlayerBeingDestroyed(this);
	}
}

//////////////////////////////////////////////////////////////////////////
// AI

/** True if the player can be pursued by AI */
bool AMagicBattleSoccerAIController::CanBePursued()
{
	AMagicBattleSoccerCharacter* MyBot = Cast<AMagicBattleSoccerCharacter>(GetPawn());
	if (nullptr == MyBot)
	{
		return false;
	}
	else
	{
		AMagicBattleSoccerGameMode* GameMode = Cast<AMagicBattleSoccerGameMode>(GetWorld()->GetAuthGameMode());
		return GameMode->CanBePursued(MyBot);
	}
}

/** The goal we want to kick the ball into */
AMagicBattleSoccerGoal* AMagicBattleSoccerAIController::GetEnemyGoal()
{
	AMagicBattleSoccerGameState* GameState = GetWorld()->GetGameState<AMagicBattleSoccerGameState>();
	AMagicBattleSoccerPlayerState* Player = Cast<AMagicBattleSoccerPlayerState>(PlayerState);
	if (nullptr == Player)
	{
		return nullptr;
	}
	else switch (Player->GetTeamNum())
	{
	case 1:
		return GameState->Team2Goal;
	case 2:
		return GameState->Team1Goal;
	default:
		return nullptr;
	}
}

/** Clips the value n so that it will be within o+d and o-d */
void AMagicBattleSoccerAIController::ClipAxe(float& n, float o, float d)
{
	if (n < o - d)
	{
		n = o - d;
	}
	else if (n > o + d)
	{
		n = o + d;
	}
}

/** If this player is in their action zone, call this function to ensure the location will not leave the zone bounds */
FVector AMagicBattleSoccerAIController::ClipToActionZone(const FVector& Location)
{
	FVector ClippedPoint = Location;
	if (nullptr != ActionZone)
	{
		FVector BoxOrigin, BoxExtent;
		ActionZone->GetActorBounds(false, BoxOrigin, BoxExtent);

		// Clipping is easy since the action box is always aligned with the major axes.
		ClipAxe(ClippedPoint.X, BoxOrigin.X, BoxExtent.X);
		ClipAxe(ClippedPoint.Y, BoxOrigin.Y, BoxExtent.Y);
	}
	return ClippedPoint;
}

/** Gets all the teammates of this player */
TArray<AMagicBattleSoccerCharacter*> AMagicBattleSoccerAIController::GetTeammates()
{
	AMagicBattleSoccerCharacter* MyBot = Cast<AMagicBattleSoccerCharacter>(GetPawn());
	if (nullptr == MyBot)
	{
		return TArray<AMagicBattleSoccerCharacter*>();
	}
	else
	{
		AMagicBattleSoccerGameState* GameState = GetWorld()->GetGameState<AMagicBattleSoccerGameState>();
		return GameState->GetTeammates(Cast<AMagicBattleSoccerPlayerState>(PlayerState));
	}
}

/** Gets all the opponents of this player */
TArray<AMagicBattleSoccerCharacter*> AMagicBattleSoccerAIController::GetOpponents()
{
	AMagicBattleSoccerCharacter* MyBot = Cast<AMagicBattleSoccerCharacter>(GetPawn());
	if (nullptr == MyBot)
	{
		return TArray<AMagicBattleSoccerCharacter*>();
	}
	else
	{
		AMagicBattleSoccerGameState* GameState = GetWorld()->GetGameState<AMagicBattleSoccerGameState>();
		return GameState->GetOpponents(Cast<AMagicBattleSoccerPlayerState>(PlayerState));
	}
}

/** Gets the closest actor between this player and a point */
AActor* AMagicBattleSoccerAIController::GetClosestActorObstructingPoint(const FVector& Point, const TArray<AActor*>& ActorsToIgnore)
{
	if (nullptr == GetPawn())
	{
		return nullptr;
	}
	else
	{
		FHitResult HitResult;
		FCollisionQueryParams CollisionQueryParams(FName(TEXT("GetClosestActorObstructingPoint Trace")), false, this);
		CollisionQueryParams.AddIgnoredActors(ActorsToIgnore);
		FCollisionObjectQueryParams CollisionObjectQueryParams;
		CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
		GetWorld()->LineTraceSingle(
			HitResult
			, GetPawn()->GetActorLocation()
			, Point
			, ECC_Pawn
			, CollisionQueryParams
			);
		AActor* ObstructingActor = HitResult.Actor.Get(false);
		return ObstructingActor;
	}
}

/** Gets the closest enemy to this player that can be pursued */
AMagicBattleSoccerCharacter* AMagicBattleSoccerAIController::GetClosestOpponent()
{
	AMagicBattleSoccerCharacter* MyBot = Cast<AMagicBattleSoccerCharacter>(GetPawn());
	if (nullptr == MyBot)
	{
		return nullptr;
	}
	else
	{
		AMagicBattleSoccerGameState* GameState = GetWorld()->GetGameState<AMagicBattleSoccerGameState>();
		return GameState->GetClosestOpponent(MyBot);
	}
}

/** Gets the ideal teammate to pass the soccer ball to, or NULL if there is none */
AMagicBattleSoccerCharacter* AMagicBattleSoccerAIController::GetIdealPassMate()
{
	AMagicBattleSoccerCharacter* MyBot = Cast<AMagicBattleSoccerCharacter>(GetPawn());
	if (nullptr == MyBot)
	{
		return nullptr;
	}
	else
	{
		AMagicBattleSoccerGameState* GameState = GetWorld()->GetGameState<AMagicBattleSoccerGameState>();
		const TArray<AMagicBattleSoccerCharacter*>& Teammates = GameState->GetTeammates(Cast<AMagicBattleSoccerPlayerState>(PlayerState));
		AMagicBattleSoccerGoal* EnemyGoal = GetEnemyGoal();
		float MyDistanceToGoal = MyBot->GetDistanceTo(EnemyGoal);
		AMagicBattleSoccerCharacter* IdealPassMate = NULL;

		for (TArray<AMagicBattleSoccerCharacter*>::TConstIterator It(Teammates.CreateConstIterator()); It; ++It)
		{
			float DistanceToGoal = (*It)->GetDistanceTo(EnemyGoal);
			float DistanceToSelf = MyBot->GetDistanceTo(*It);

			// Skip this teammate if they're farther from the goal
			if (MyDistanceToGoal < DistanceToGoal)
			{
				continue;
			}
			// Skip this teammate if they're farther from the goal than the ideal pass mate
			else if (NULL != IdealPassMate && IdealPassMate->GetDistanceTo(EnemyGoal) < DistanceToGoal)
			{
				continue;
			}
			// Skip this teammate if they're too close or too far
			else if (DistanceToSelf < 600 || DistanceToSelf > 1600)
			{
				continue;
			}
			// Skip this teammate if there's an opponent close to them
			else
			{
				AMagicBattleSoccerCharacter *ClosestOpponent = GameState->GetClosestOpponent(*It);
				if (NULL != ClosestOpponent && ClosestOpponent->GetDistanceTo(*It) < 300)
				{
					continue;
				}
				else
				{
					// Skip this teammate if there's something in between us and them
					TArray<AActor*> ActorsToIgnore;
					ActorsToIgnore.Add(*It);
					ActorsToIgnore.Add(GameState->SoccerBall);
					ActorsToIgnore.Add(MyBot);
					if (NULL != GetClosestActorObstructingPoint((*It)->GetActorLocation(), ActorsToIgnore))
					{
						continue;
					}

					// Success!
					IdealPassMate = *It;
				}
			}
		}
		return IdealPassMate;
	}
}

/** Gets the ideal object to run to if the player is idle */
AActor* AMagicBattleSoccerAIController::GetIdealPursuitTarget()
{
	// By default we want to pursue the soccer ball
	AMagicBattleSoccerCharacter* MyBot = Cast<AMagicBattleSoccerCharacter>(GetPawn());
	if (nullptr == MyBot)
	{
		return nullptr;
	}
	else
	{
		AMagicBattleSoccerGameState* GameState = GetWorld()->GetGameState<AMagicBattleSoccerGameState>();
		AMagicBattleSoccerGameMode* GameMode = Cast<AMagicBattleSoccerGameMode>(GetWorld()->GetAuthGameMode());
		AActor* PursuitTarget = GameState->SoccerBall;

		// Find out the closest obstructing actor
		TArray<AActor*> ActorsToIgnore;
		ActorsToIgnore.Add(GameState->SoccerBall);
		ActorsToIgnore.Add(MyBot);
		// Ignore players that cannot be pursued
		const TArray<AMagicBattleSoccerCharacter*>& Opponents = GameState->GetOpponents(Cast<AMagicBattleSoccerPlayerState>(PlayerState));
		for (TArray<AMagicBattleSoccerCharacter*>::TConstIterator It(Opponents.CreateConstIterator()); It; ++It)
		{
			AMagicBattleSoccerCharacter *Player = *It;
			if (!GameMode->CanBePursued(Player))
			{
				ActorsToIgnore.Add(*It);
			}
		}

		AActor* ObstructingActor = GetClosestActorObstructingPoint(PursuitTarget->GetActorLocation(), ActorsToIgnore);
		if (nullptr != ObstructingActor)
		{
			// If the obstructing actor is an opponent, then pursue them
			AMagicBattleSoccerCharacter *ObstructingPlayer = Cast<AMagicBattleSoccerCharacter>(ObstructingActor);
			if (nullptr != ObstructingPlayer && Opponents.Contains(ObstructingPlayer) && !ActorsToIgnore.Contains(ObstructingPlayer))
			{
				PursuitTarget = ObstructingPlayer;
			}
		}

		return PursuitTarget;
	}
}

/** Gets the ideal point to run to when not chasing another actor while following the ball possessor */
FVector AMagicBattleSoccerAIController::GetIdealPossessorFollowLocation()
{
	AMagicBattleSoccerGameState* GameState = GetWorld()->GetGameState<AMagicBattleSoccerGameState>();
	AMagicBattleSoccerGoal* EnemyGoal = GetEnemyGoal();
	AMagicBattleSoccerCharacter* MyBot = Cast<AMagicBattleSoccerCharacter>(GetPawn());
	AMagicBattleSoccerCharacter* Possessor = GameState->SoccerBall->Possessor;
	if (nullptr == MyBot)
	{
		return FVector::ZeroVector;
	}
	else if (nullptr == Possessor)
	{
		// The possessor could have been reset before we got to this function from an AI task blueprint.
		return MyBot->GetActorLocation();
	}
	else if (nullptr == EnemyGoal)
	{
		// This should never happen; but if it did, just run to the possessor
		return Possessor->GetActorLocation();
	}
	else
	{
		FVector MyLocation = GetPawn()->GetActorLocation();
		FVector NewLocation = FVector(
			MyLocation.X,
			(Possessor->GetActorLocation().Y + EnemyGoal->GetIdealRunLocation(MyBot).Y) * 0.5f,
			MyLocation.Z
			);
		return NewLocation;
	}
}

/** Attacks a soccer player */
void AMagicBattleSoccerAIController::AttackPlayer(AMagicBattleSoccerCharacter* Target)
{
	AMagicBattleSoccerCharacter* MyBot = Cast<AMagicBattleSoccerCharacter>(GetPawn());
	if (nullptr != MyBot 
		&& nullptr != Target
		&& nullptr != MyBot->PrimaryWeapon
		&& !MyBot->PossessesBall() 
		)
	{
		FRotator faceRot = ((Target->GetActorLocation() + Target->GetVelocity()) - MyBot->GetActorLocation()).Rotation();
		// Face the target
		MyBot->SetActorRotation(faceRot);

		// Start attacking the player if we haven't already
		if (!IsAttacking)
		{
			MyBot->StartPrimaryWeaponFire();
			IsAttacking = true;
		}
	}
}

/** Stops attacking a soccer player */
UFUNCTION(BlueprintCallable, Category = Soccer)
void AMagicBattleSoccerAIController::StopAttackingPlayer()
{
	AMagicBattleSoccerCharacter* MyBot = Cast<AMagicBattleSoccerCharacter>(GetPawn());
	if (nullptr != MyBot)
	{
		if (IsAttacking)
		{
			MyBot->StopPrimaryWeaponFire();
			IsAttacking = false;
		}
	}
}

/** Tries to kick ball into the goal. Returns true if the ball was kicked. */
bool AMagicBattleSoccerAIController::KickBallToGoal()
{
	AMagicBattleSoccerCharacter* MyBot = Cast<AMagicBattleSoccerCharacter>(GetPawn());
	AMagicBattleSoccerGameState* GameState = GetWorld()->GetGameState<AMagicBattleSoccerGameState>();
	AMagicBattleSoccerGoal* EnemyGoal = GetEnemyGoal();

	if (nullptr == MyBot)
	{
		return false;
	}
	else if (nullptr == EnemyGoal)
	{
		return false;
	}
	else if (MyBot != GameState->SoccerBall->Possessor)
	{
		return false;
	}
	else if (MyBot->GetDistanceTo(EnemyGoal) > 1500.0f)
	{
		return false;
	}
	else
	{
		TArray<AActor*> ActorsToIgnore;
		ActorsToIgnore.Add(EnemyGoal);
		ActorsToIgnore.Add(GameState->SoccerBall);
		ActorsToIgnore.Add(MyBot);
		if (NULL != GetClosestActorObstructingPoint(EnemyGoal->GetActorLocation() + FVector(0, 0, 100), ActorsToIgnore))
		{
			return false;
		}
		else
		{
			KickBallToLocation(EnemyGoal->GetActorLocation());
			return true;
		}
	}
}


/** [local] Kicks the ball to the specified location */
void AMagicBattleSoccerAIController::KickBallToLocation(const FVector& Location)
{
	AMagicBattleSoccerCharacter* MyBot = Cast<AMagicBattleSoccerCharacter>(GetPawn());
	AMagicBattleSoccerGameState* GameState = GetWorld()->GetGameState<AMagicBattleSoccerGameState>();
	if (nullptr != MyBot)
	{
		FVector ActorLocation = MyBot->GetActorLocation();
		FVector v = Location - ActorLocation;
		v.Z = 0;
		float distance = v.Size2D();
		v.Normalize();
		MyBot->KickBall(v * distance * 110.f);
	}
}
