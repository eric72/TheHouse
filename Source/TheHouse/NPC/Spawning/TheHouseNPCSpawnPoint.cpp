#include "TheHouse/NPC/Spawning/TheHouseNPCSpawnPoint.h"

#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"

ATheHouseNPCSpawnPoint::ATheHouseNPCSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	UArrowComponent* Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	Arrow->SetupAttachment(Root);
	Arrow->ArrowSize = 1.0f;
}

FVector ATheHouseNPCSpawnPoint::GetSpawnWorldLocation() const
{
	const FVector Origin = GetActorLocation();
	if (RandomRadiusUU <= 0.f)
	{
		return Origin;
	}

	const FVector2D Rand2 = FMath::RandPointInCircle(RandomRadiusUU);
	return Origin + FVector(Rand2.X, Rand2.Y, 0.f);
}

