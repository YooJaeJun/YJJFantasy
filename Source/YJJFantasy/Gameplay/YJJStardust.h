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
	// ---- 모양·반짝임 (cm, 초)
	static constexpr float PhaseSpread = 10.f;
	static constexpr float CoreSize = 26.f;
	static constexpr float CoreGlow = 5.f;
	static constexpr float CoreRim = 0.5f;
	static constexpr float HaloSize = 70.f;
	static constexpr float HaloGlow = 1.2f;
	static constexpr float StarGlow = 3.f;
	static constexpr float StarThickness = 6.f;
	static constexpr float StarLength = 90.f;
	static constexpr float CollectFlashTime = 0.25f;
	static constexpr float CollectFlashGrow = 1.5f;
	static constexpr float BobSpeed = 2.4f;
	static constexpr float BobHeight = 18.f;
	static constexpr float TwinkleBase = 0.7f;
	static constexpr float TwinkleRange = 0.3f;
	static constexpr float TwinkleSpeed = 6.f;
	static constexpr float TwinklePhaseMul = 3.f;
	static constexpr float SpinSpeed = 90.f;
	static constexpr float HaloPulseBase = 0.9f;
	static constexpr float HaloPulseRange = 0.4f;
	/** 주인공 가슴 쪽으로 빨려 간다 */
	static constexpr float TargetLift = 20.f;
	/** 튀어 오른 별가루가 땅을 찾는 범위 */
	static constexpr float LandProbeUp = 50.f;
	static constexpr float LandProbeDown = 80.f;
	/** 이 아래로 떨어지면 사라진다 */
	static constexpr float LostBelowZ = -100000.f;

	/** 줍는 소리가 연달아 나면 음이 조금씩 올라간다 (모든 별가루가 함께 쓴다) */
	static float LastCollectTime;
	static int32 ComboIndex;
	static constexpr float SemitonesPerOctave = 12.f;

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
