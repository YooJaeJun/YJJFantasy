#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "YJJFantasyGameMode.generated.h"

/** 기본 게임모드: 3인칭 영웅을 스폰한다. */
UCLASS()
class YJJFANTASY_API AYJJFantasyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AYJJFantasyGameMode();
};
