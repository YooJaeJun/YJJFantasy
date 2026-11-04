#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "YJJFantasyGameMode.generated.h"

/** 기본 게임모드: 하늘섬 월드를 짓고 3인칭 영웅을 시작 위치에 세운다. */
UCLASS()
class YJJFANTASY_API AYJJFantasyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AYJJFantasyGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual void StartPlay() override;

private:
	UPROPERTY()
	TObjectPtr<AActor> SpawnPoint;
};
