#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "YJJAutoPilot.generated.h"

/**
 * 자동 테스트. 명령줄 -yjjtour / -yjjplay 또는 콘솔 yjj.Tour / yjj.AutoPlay.
 *  Tour     : 명소마다 카메라를 세우고 스크린샷 (Saved/Screenshots/Tour_*.png)
 *  AutoPlay : 등불·활공·버섯·전투·엔딩을 차례로 실행하며 로그로 검증 ([AutoPilot] PASS/FAIL)
 * -yjjquit 이 있으면 끝나고 게임을 끈다.
 */
UCLASS()
class YJJFANTASY_API UYJJAutoPilot : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	void StartTour();
	void StartAutoPlay();

private:
	struct FStep
	{
		float Wait = 1.f;
		TFunction<void()> Action;
	};

	void Add(float Wait, TFunction<void()> Action) { Steps.Add({ Wait, MoveTemp(Action) }); }
	void Shot(const FString& Name);
	void View(const FVector& HeroLocation, float Yaw, float Pitch);
	void Check(bool bOk, const FString& What);
	class AYJJHero* Hero() const;

	TArray<FStep> Steps;
	int32 StepIndex = -1;
	float StepTimer = 0.f;
	bool bQuitWhenDone = false;
	int32 Passed = 0;
	int32 Failed = 0;
	float TrackMaxZ = 0.f;
	float TrackStartZ = 0.f;
	int32 FrameCount = 0;
	float FrameTime = 0.f;
};
