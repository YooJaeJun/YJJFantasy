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
	// ---- 모양 (cm). StoneParts/CapBottom/CapTop/Roof = (위치 z, 지름, 높이)
	/** 등 갓(빛 구슬)의 높이 */
	static constexpr float OrbHeight = 330.f;
	inline static const FVector StoneParts[] = { FVector(20.f, 180.f, 40.f), FVector(55.f, 130.f, 30.f), FVector(170.f, 34.f, 210.f) };
	inline static const FVector CapBottom = FVector(272.f, 110.f, 14.f);
	inline static const FVector CapTop = FVector(388.f, 120.f, 12.f);
	inline static const FVector Roof = FVector(430.f, 100.f, 75.f);
	static constexpr int32 CageBars = 4;
	static constexpr float CageRadius = 46.f;
	inline static const FVector CageBarSize = FVector(9.f, 9.f, 110.f);
	static constexpr float FinialHeight = 478.f;
	static constexpr float FinialSize = 22.f;
	static constexpr float FinialEmissive = 1.5f;
	static constexpr float BronzeRoughness = 0.45f;
	static constexpr float OrbSize = 62.f;
	static constexpr float HaloSize = 170.f;
	static constexpr float BeamWidth = 70.f;
	static constexpr float BeamLength = 12000.f;
	static constexpr int32 RingMotes = 3;
	static constexpr float RingMoteSize = 14.f;
	static constexpr float RingRadius = 120.f;
	static constexpr float LightRadius = 1800.f;
	/** 상호작용 거리를 재는 기준 높이 */
	static constexpr float InteractHeight = 100.f;

	// ---- 빛 (세기, 초)
	static constexpr float OrbGlowUnlit = 0.6f;
	static constexpr float OrbCore = 0.6f;
	static constexpr float OrbRim = 0.6f;
	static constexpr float UnlitPulseSpeed = 2.2f;
	static constexpr float UnlitPulseBase = 0.4f;
	static constexpr float UnlitPulseRange = 0.5f;
	static constexpr float LightUpTime = 1.6f;
	static constexpr float OrbGlowLitBase = 2.f;
	static constexpr float OrbGlowLitRange = 6.f;
	static constexpr float OrbFlickerSpeed = 4.f;
	static constexpr float OrbFlicker = 0.6f;
	static constexpr float HaloGlowLit = 1.4f;
	static constexpr float HaloBreath = 0.1f;
	static constexpr float HaloBreathSpeed = 2.f;
	static constexpr float LightLit = 900.f;
	static constexpr float BeamCore = 0.15f;
	static constexpr float BeamRim = 1.2f;
	static constexpr float BeamRiseTime = 0.9f;
	static constexpr float BeamStartThicken = 1.6f;
	static constexpr float BeamFlashDecay = 1.5f;
	static constexpr float BeamGlow = 0.6f;
	static constexpr float BeamFlashGlow = 5.f;
	static constexpr float BurstRim = 1.5f;
	static constexpr float BurstTime = 0.8f;
	static constexpr float BurstGrow = 9.f;
	static constexpr float BurstGlow = 3.f;
	static constexpr float RingGlow = 6.f;
	static constexpr float RingSpinSpeed = 1.6f;
	static constexpr float RingBobSpeed = 2.f;
	static constexpr float RingBob = 50.f;

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
