#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YJJStardust.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/** 별가루. 가까이 가면 영웅에게 빨려 들어온다. 15개마다 기력 상한 +20 */
UCLASS()
class YJJFANTASY_API AYJJStardust : public AActor
{
	GENERATED_BODY()

public:
	AYJJStardust();

	virtual void Tick(float DeltaTime) override;

	/** 몹이 떨군 별가루는 전체 개수에 넣지 않는다 */
	bool bCountsTowardTotal = true;

	/** 떨궈질 때 튀어 오르는 속도 */
	FVector PopVelocity = FVector::ZeroVector;

protected:
	virtual void BeginPlay() override;

private:
	void Collect();

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Core;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Halo;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Star;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> StarB;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> HaloMID;

	FVector Home = FVector::ZeroVector;
	float Time = 0.f;
	float Phase = 0.f;
	bool bMagnet = false;
	bool bCollected = false;
	float CollectTime = 0.f;
	FVector MagnetVelocity = FVector::ZeroVector;
};
