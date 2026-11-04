#include "Core/YJJFantasyGameMode.h"

#include "Core/YJJAudio.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Player/YJJHero.h"
#include "UI/YJJHUD.h"
#include "World/YJJWorldBuilder.h"

AYJJFantasyGameMode::AYJJFantasyGameMode()
{
	DefaultPawnClass = AYJJHero::StaticClass();
	HUDClass = AYJJHUD::StaticClass();
}

void AYJJFantasyGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	bool bHasBuilder = false;
	for (TActorIterator<AYJJWorldBuilder> It(GetWorld()); It; ++It)
	{
		bHasBuilder = true;
	}
	if (!bHasBuilder)
	{
		GetWorld()->SpawnActor<AYJJWorldBuilder>(AYJJWorldBuilder::StaticClass(), FTransform::Identity, Params);
	}

	// 섬은 BeginPlay에 지어지므로 영웅은 그 전에 시작 위치(공중)에 서 있다가 첫 틱부터 섬 위에 선다
	SpawnPoint = GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(),
		FTransform(FRotator(0, AYJJWorldBuilder::GetSpawnYaw(), 0), AYJJWorldBuilder::GetSpawnLocation()), Params);
}

AActor* AYJJFantasyGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	return SpawnPoint ? SpawnPoint.Get() : Super::ChoosePlayerStart_Implementation(Player);
}

void AYJJFantasyGameMode::StartPlay()
{
	Super::StartPlay();
	YJJAudio::StartAmbience(this);
}
