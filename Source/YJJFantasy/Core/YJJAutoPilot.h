#pragma once

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
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
	// 콘솔 명령: yjj.Tour, yjj.AutoPlay, yjj.Toon <Ref> <Strength> <Outline>, yjj.LightAll
	static FAutoConsoleCommandWithWorld TourCommand;
	static FAutoConsoleCommandWithWorld PlayCommand;
	static FAutoConsoleCommandWithWorldAndArgs ToonCommand;
	static FAutoConsoleCommandWithWorld LightAllCommand;

	struct FStep
	{
		float Wait = 1.f;
		TFunction<void()> Action;
	};

	void Add(float Wait, TFunction<void()> Action) { Steps.Add({ Wait, MoveTemp(Action) }); }
	void Shot(const FString& Name);
	void View(const FVector& HeroLocation, float Yaw, float Pitch);
	/** Content/YJJFantasy/Data/TestPoints.csv 의 지점으로 */
	void ViewPoint(const TCHAR* Name);
	void TrackHeight();
	/** Tuning.csv 의 Test.<Name> (합격 기준) */
	static float Test(const TCHAR* Name);

	/** 단계 사이 기다리는 시간(초)과 반복 횟수 */
	struct FPace
	{
		float TourStart;
		float Boot;
		float Teleport;
		float LightSettle;
		float Shot;
		float Check;
		float Short;
		float Land;
		int32 JumpSamples;
		float JumpSample;
		int32 UpdraftSamples;
		int32 GlideSteps;
		float FlightSample;
		float GlideLand;
		int32 BounceSamples;
		float BounceSample;
		int32 Swings;
		float Swing;
		float AfterCombat;
		float Interact;
		float ChestOpen;
		float LanternInteract;
		float LanternGlow;
		float FinaleBeam;
		float FinaleSky;
		float FinaleEnd;
		float Fall;
		float Respawn;
		float CombatLookDown;
		float ChestLookDown;
		float LanternLookDown;
		/** 평균 FPS 에서 뺄 처음 단계 수 (셰이더 컴파일 구간) */
		int32 SkipFirstSteps;
		float MinFrameTime;
	};
	inline static const FPace Pace = {
		3.f, 4.f, 0.1f, 2.5f, 0.6f, 0.5f, 0.2f, 0.6f,
		12, 0.05f, 8, 12, 0.5f, 2.f, 10, 0.15f, 8, 0.4f, 0.8f,
		1.5f, 1.2f, 1.8f, 2.f, 4.f, 9.f, 8.f, 2.5f, 3.5f,
		15.f, 10.f, 5.f, 2, 0.001f,
	};
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
