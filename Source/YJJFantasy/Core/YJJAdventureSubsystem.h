#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "YJJAdventureSubsystem.generated.h"

class AYJJLantern;
class AYJJShrine;

/** 섬 하나의 이름표. 영웅이 들어오면 화면 가운데에 이름이 크게 뜬다 */
USTRUCT()
struct FYJJZone
{
	GENERATED_BODY()

	FString Name;
	FString Subtitle;
	FVector Center = FVector::ZeroVector;
	float Radius = 2000.f;
	FLinearColor Tint = FLinearColor::White;
};

UENUM()
enum class EYJJPhase : uint8
{
	Explore,
	Finale,
	Ended,
};

/**
 * 모험 진행 상태: 등불, 별가루, 체크포인트, 섬 이름, 알림 메시지.
 * HUD와 게임 액터가 모두 이걸 본다.
 */
UCLASS()
class YJJFANTASY_API UYJJAdventureSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UYJJAdventureSubsystem* Get(const UObject* WorldContext);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	// ---- 등록 (월드 생성 시)
	void RegisterLantern(AYJJLantern* Lantern);
	void RegisterShrine(AYJJShrine* Shrine);
	void RegisterStardust() { StardustTotal++; }
	void AddZone(const FYJJZone& Zone) { Zones.Add(Zone); }

	// ---- 진행
	void OnLanternLit(AYJJLantern* Lantern);
	void OnStardustCollected();
	void SetCheckpoint(const FVector& Location, float Yaw);
	void StartFinale();

	/** 화면 아래쪽 알림 */
	void Toast(const FString& Text, const FLinearColor& Color = FLinearColor::White, float Duration = 3.f);

	// ---- 조회
	int32 GetLanternsLit() const;
	int32 GetLanternCount() const { return Lanterns.Num(); }
	int32 GetStardust() const { return Stardust; }
	int32 GetStardustTotal() const { return StardustTotal; }
	EYJJPhase GetPhase() const { return Phase; }
	float GetPlayTime() const { return PlayTime; }
	float GetPhaseTime() const { return PhaseTime; }
	FVector GetCheckpoint() const { return Checkpoint; }
	float GetCheckpointYaw() const { return CheckpointYaw; }
	const TArray<TWeakObjectPtr<AYJJLantern>>& GetLanterns() const { return Lanterns; }
	AYJJShrine* GetShrine() const { return Shrine_.Get(); }

	/** 지금 가야 할 곳 (가장 가까운 꺼진 등불, 다 켰으면 성소). 없으면 false */
	bool GetObjectiveLocation(const FVector& From, FVector& OutLocation) const;
	FString GetObjectiveText() const;

	/** 별가루 15개마다 기력 상한 +20 */
	float GetStaminaBonus() const { return (Stardust / 15) * 20.f; }

	struct FToast
	{
		FString Text;
		FLinearColor Color;
		float Age = 0.f;
		float Duration = 3.f;
	};
	const TArray<FToast>& GetToasts() const { return Toasts; }

	/** 섬 이름 카드 (Age < 0 이면 없음) */
	const FYJJZone* GetZoneCard(float& OutAge) const;

private:
	void UpdateZones(float DeltaTime);

	TArray<TWeakObjectPtr<AYJJLantern>> Lanterns;
	TWeakObjectPtr<AYJJShrine> Shrine_;
	TArray<FYJJZone> Zones;
	TSet<int32> VisitedZones;
	int32 CurrentZone = INDEX_NONE;
	int32 CardZone = INDEX_NONE;
	float CardAge = -1.f;

	TArray<FToast> Toasts;

	int32 Stardust = 0;
	int32 StardustTotal = 0;
	FVector Checkpoint = FVector::ZeroVector;
	float CheckpointYaw = 0.f;
	EYJJPhase Phase = EYJJPhase::Explore;
	float PlayTime = 0.f;
	float PhaseTime = 0.f;
};
