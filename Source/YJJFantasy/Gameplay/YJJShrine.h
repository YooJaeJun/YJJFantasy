#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YJJShrine.generated.h"

class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * 바람의 성소. 중앙 섬의 큰 제단과 떠 있는 수정.
 * 등불이 모두 켜지면 깨어나고, 상호작용하면 엔딩 연출이 시작된다.
 */
UCLASS()
class YJJFANTASY_API AYJJShrine : public AActor
{
	GENERATED_BODY()

public:
	AYJJShrine();

	virtual void Tick(float DeltaTime) override;

	void SetAwake(bool bInAwake);
	bool IsAwake() const { return bAwake; }

	bool CanInteract(const FVector& From) const;
	void Interact();

	/** 엔딩 중 카메라가 바라볼 곳 */
	FVector GetCrystalLocation() const;

	static constexpr float InteractRange = 520.f;

protected:
	virtual void BeginPlay() override;

private:
	static bool IsBrokenPillar(int32 Index);

	// ---- 제단 (cm). 한 단 StepHeight 라 걸어서 오른다
	static constexpr int32 StepCount = 3;
	static constexpr float StepHeight = 40.f;
	static constexpr float StepBottomDiameter = 1900.f;
	static constexpr float StepShrink = 400.f;
	/** 제단 윗면 높이 = StepCount x StepHeight */
	static constexpr float TopZ = StepCount * StepHeight;
	static constexpr float TopInlayDiameter = 1060.f;
	static constexpr float TopInlayThickness = 4.f;
	static constexpr float GoldRoughness = 0.35f;
	// ---- 둘레 기둥
	static constexpr int32 PillarCount = 8;
	inline static const int32 BrokenPillars[] = { 2, 5 };
	static constexpr float PillarRing = 820.f;
	static constexpr float PillarBase = 40.f;
	static constexpr float PillarHeight = 720.f;
	static constexpr float PillarDiameter = 120.f;
	static constexpr float BrokenPillarHeight = 260.f;
	static constexpr float BrokenPillarStep = 20.f;
	static constexpr float PlinthHeight = 60.f;
	static constexpr float PlinthWidth = 170.f;
	static constexpr float PlinthThickness = 40.f;
	static constexpr float CapitalLift = 25.f;
	static constexpr float CapitalThickness = 50.f;
	static constexpr float CapitalTrimLift = 60.f;
	static constexpr float CapitalTrimWidth = 150.f;
	static constexpr float CapitalTrimThickness = 14.f;
	static constexpr float DebrisOut = 1.12f;
	static constexpr float DebrisSide = 120.f;
	static constexpr float DebrisHeight = 70.f;
	inline static const FVector DebrisSize = FVector(110.f, 110.f, 220.f);
	static constexpr float DebrisPitch = 85.f;
	static constexpr float DebrisTurn = 30.f;
	static constexpr float LintelLift = 70.f;
	static constexpr float LintelOverhang = 60.f;
	static constexpr float LintelDepth = 90.f;
	static constexpr float LintelThickness = 50.f;
	// ---- 룬 석판 (등불 수만큼)
	static constexpr int32 RuneCount = 5;
	static constexpr float RuneRadius = 380.f;
	inline static const FVector RuneSize = FVector(150.f, 90.f, 16.f);
	static constexpr float RuneRoughness = 0.4f;
	static constexpr float RuneGlow = 3.f;
	// ---- 받침대
	static constexpr float PedestalDiameter = 300.f;
	static constexpr float PedestalHeight = 120.f;
	static constexpr float PedestalTrimLift = 125.f;
	static constexpr float PedestalTrimDiameter = 340.f;
	static constexpr float PedestalTrimThickness = 14.f;
	// ---- 떠 있는 수정
	static constexpr float CrystalWidth = 200.f;
	static constexpr float CrystalHalfHeight = 130.f;
	/** 아래 원뿔은 뒤집어 팔면체를 만든다 */
	inline static const FRotator UpsideDown = FRotator(180.f, 0.f, 0.f);
	static constexpr float CrystalRoughness = 0.2f;
	static constexpr float CrystalEmissiveDormant = 0.15f;
	static constexpr int32 ShardCount = 5;
	inline static const FVector ShardSize = FVector(50.f, 50.f, 90.f);
	inline static const FVector AuraSize = FVector(520.f, 520.f, 620.f);
	static constexpr float AuraCore = 0.05f;
	static constexpr float AuraDormant = 0.2f;
	static constexpr float SpinDormant = 12.f;
	static constexpr float SpinAwake = 40.f;
	static constexpr float SpinFinale = 400.f;
	static constexpr float FloatSpeed = 1.2f;
	static constexpr float FloatHeight = 30.f;
	static constexpr float PulseSpeed = 3.f;
	static constexpr float GlowAwakeBase = 2.5f;
	static constexpr float GlowAwakePulse = 1.5f;
	static constexpr float GlowFinaleBase = 4.f;
	static constexpr float GlowFinaleRise = 10.f;
	static constexpr float AuraAwakeBase = 0.8f;
	static constexpr float AuraAwakePulse = 0.4f;
	static constexpr float LightDormant = 80.f;
	static constexpr float LightDormantIdle = 60.f;
	static constexpr float LightAwake = 600.f;
	static constexpr float LightPerGlow = 80.f;
	static constexpr float LightRadius = 3500.f;
	static constexpr float ShardOrbit = 340.f;
	static constexpr float ShardSpinDormant = 0.5f;
	static constexpr float ShardSpinAwake = 1.4f;
	static constexpr float ShardSpreadTime = 3.f;
	static constexpr float ShardSpreadSpeed = 200.f;
	static constexpr float ShardBobSpeed = 2.f;
	static constexpr float ShardBob = 60.f;
	static constexpr float ShardTumblePitch = 50.f;
	static constexpr float ShardTumbleOffset = 40.f;
	static constexpr float ShardTumbleYaw = 30.f;
	// ---- 엔딩: 빛기둥·충격파·불꽃
	static constexpr float BeamBaseWidth = 360.f;
	static constexpr float BeamWidth = 220.f;
	static constexpr float BeamLength = 40000.f;
	static constexpr float BeamCore = 0.3f;
	static constexpr float BeamRim = 1.5f;
	static constexpr float BeamGlow = 0.45f;
	static constexpr float BeamFlashGlow = 5.f;
	static constexpr float BeamFlashDecay = 1.1f;
	static constexpr float BeamStartThicken = 2.5f;
	static constexpr float BeamThinDecay = 1.2f;
	inline static const FVector ShockSize = FVector(100.f, 100.f, 30.f);
	static constexpr float ShockRim = 2.f;
	static constexpr float ShockTime = 2.5f;
	static constexpr float ShockGrow = 80.f;
	static constexpr float ShockFlatStart = 0.3f;
	static constexpr float ShockThicken = 4.f;
	static constexpr float ShockGlow = 3.f;
	static constexpr int32 SparkCount = 28;
	static constexpr float SparkSize = 40.f;
	static constexpr float SparkGlow = 6.f;
	static constexpr float SparkPeriodMin = 2.6f;
	static constexpr float SparkPeriodRange = 1.4f;
	static constexpr float SparkSpeedMin = 900.f;
	static constexpr float SparkSpeedRange = 700.f;
	static constexpr float SparkRise = 1800.f;
	static constexpr float SparkGravity = 700.f;
	static constexpr float SparkStartScale = 0.5f;
	static constexpr float SparkEndScale = 0.1f;
	static constexpr int32 HashSpark = 31;
	static constexpr int32 HashSparkSalt = 7;
	static constexpr float GoldenAngle = 2.399963f;
	/** 노을이 지기 시작하는 엔딩 시각 */
	static constexpr float SunsetFrom = 1.f;
	static constexpr float CameraBlendExp = 2.f;

	bool bAwake = false;
	bool bFinale = false;
	float Time = 0.f;
	float FinaleTime = 0.f;
	float CrystalHeight = 650.f;

	UPROPERTY()
	TObjectPtr<USceneComponent> CrystalPivot;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Aura;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Beam;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Shockwave;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Shards;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Sparks;

	UPROPERTY()
	TObjectPtr<UPointLightComponent> Light;

	/** 엔딩 연출용 카메라 (성소 둘레를 천천히 돈다) */
	UPROPERTY()
	TObjectPtr<class ACameraActor> FinaleCamera;

	float CameraAngle = 0.f;
	bool bCameraReturned = false;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> CrystalMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> AuraMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BeamMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> ShockMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> SparkMID;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> RuneMIDs;
};
