#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YJJSkyWhale.generated.h"

class UStaticMeshComponent;

/**
 * 섬들 주위를 느긋하게 헤엄치는 하늘 고래 (AI 모델 SM_SkyWhale).
 * 큰 원을 그리며 위아래로 출렁이고, 도는 방향으로 몸을 기울인다. 충돌 없음.
 */
UCLASS()
class YJJFANTASY_API AYJJSkyWhale : public AActor
{
	GENERATED_BODY()

public:
	AYJJSkyWhale();

	virtual void Tick(float DeltaTime) override;

	/** 도는 중심, 반지름, 한 바퀴 시간(초), 몸길이(cm), 시작 각도(도) */
	void Setup(const FVector& InCenter, float InRadius, float InPeriod, float InLength, float InStartAngle);

	/** 모델 머리 방향 보정 (AI 모델마다 앞쪽이 다르다) */
	float MeshYawOffset = 0.f;

protected:
	virtual void BeginPlay() override;

private:
	// ---- 헤엄 모양
	static constexpr float BobSpeed = 0.35f;
	static constexpr float BobHeight = 450.f;
	static constexpr float BobPitch = 6.f;
	/** 진행 방향 = 원의 접선 (반시계) */
	static constexpr float TangentTurn = 90.f;
	/** 원 안쪽으로 기울기 */
	static constexpr float BankRoll = -8.f;
	static constexpr float SwimPitchSpeed = 1.1f;
	static constexpr float SwimPitch = 3.f;
	static constexpr float SwimYawSpeed = 0.55f;
	static constexpr float SwimYaw = 2.f;

	FVector Center = FVector::ZeroVector;
	float Radius = 12000.f;
	float Period = 160.f;
	float Length = 4000.f;
	float Angle = 0.f;
	float Time = 0.f;

	UPROPERTY()
	TObjectPtr<USceneComponent> Body;
};
