#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YJJLantern.generated.h"

class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * 바람 등불. 가까이 가서 상호작용하면 불이 켜지고 하늘로 빛기둥이 솟는다.
 * 켜진 등불은 체크포인트가 된다.
 */
UCLASS()
class YJJFANTASY_API AYJJLantern : public AActor
{
	GENERATED_BODY()

public:
	AYJJLantern();

	void Setup(const FString& InName, const FLinearColor& InColor);

	virtual void Tick(float DeltaTime) override;

	bool IsLit() const { return bLit; }
	const FString& GetLanternName() const { return LanternName; }
	FLinearColor GetColor() const { return Color; }

	/** 상호작용 가능한 거리 안인지 */
	bool CanInteract(const FVector& From) const;
	void Interact();

	/** 불이 켜진 정도 0~1 (연출용) */
	float GetLightAlpha() const { return LightAlpha; }

	/** 등불 앞 부활 위치 */
	FVector GetRespawnLocation() const;

	static constexpr float InteractRange = 260.f;

protected:
	virtual void BeginPlay() override;

private:
	/** 등 갓(빛 구슬)의 높이 */
	static constexpr float OrbHeight = 330.f;

	FString LanternName;
	FLinearColor Color = FLinearColor(1.f, 0.8f, 0.4f);
	bool bLit = false;
	float LightAlpha = 0.f;
	float Time = 0.f;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Orb;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Halo;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Beam;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Burst;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Rings;

	UPROPERTY()
	TObjectPtr<UPointLightComponent> Light;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> OrbMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> HaloMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BeamMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BurstMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> RingMID;
};
