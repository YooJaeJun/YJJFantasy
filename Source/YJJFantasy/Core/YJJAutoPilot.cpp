#include "Core/YJJAutoPilot.h"

#include "Core/YJJAdventureSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Gameplay/YJJChest.h"
#include "Gameplay/YJJLantern.h"
#include "Gameplay/YJJShrine.h"
#include "Gameplay/YJJSlime.h"
#include "Gameplay/YJJTraversal.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Player/YJJHero.h"
#include "UnrealClient.h"
#include "World/YJJEnvironment.h"
#include "World/YJJWorldBuilder.h"
#include "YJJFantasy.h"

namespace
{
	FVector W(float X, float Y, float Z)
	{
		return FVector(X, Y, AYJJWorldBuilder::BaseZ + Z);
	}

	FAutoConsoleCommandWithWorld TourCommand(TEXT("yjj.Tour"), TEXT("명소 스크린샷 투어"), FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (UYJJAutoPilot* Pilot = World ? World->GetSubsystem<UYJJAutoPilot>() : nullptr)
		{
			Pilot->StartTour();
		}
	}));

	FAutoConsoleCommandWithWorld PlayCommand(TEXT("yjj.AutoPlay"), TEXT("자동 플레이 검증"), FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (UYJJAutoPilot* Pilot = World ? World->GetSubsystem<UYJJAutoPilot>() : nullptr)
		{
			Pilot->StartAutoPlay();
		}
	}));

	FAutoConsoleCommandWithWorldAndArgs ToonCommand(TEXT("yjj.Toon"), TEXT("셀 셰이딩 조절: Ref Strength Outline"), FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() < 3 || !World)
		{
			return;
		}
		for (TActorIterator<AYJJEnvironment> It(World); It; ++It)
		{
			It->SetToonParams(FCString::Atof(*Args[0]), FCString::Atof(*Args[1]), FCString::Atof(*Args[2]));
		}
	}));

	FAutoConsoleCommandWithWorld LightAllCommand(TEXT("yjj.LightAll"), TEXT("모든 등불 켜기"), FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		for (TActorIterator<AYJJLantern> It(World); It; ++It)
		{
			It->Interact();
		}
	}));
}

TStatId UYJJAutoPilot::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UYJJAutoPilot, STATGROUP_Tickables);
}

bool UYJJAutoPilot::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UYJJAutoPilot::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	bQuitWhenDone = FParse::Param(FCommandLine::Get(), TEXT("yjjquit"));
	if (FParse::Param(FCommandLine::Get(), TEXT("yjjtour")))
	{
		StartTour();
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("yjjplay")))
	{
		StartAutoPlay();
	}
}

AYJJHero* UYJJAutoPilot::Hero() const
{
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	return PC ? Cast<AYJJHero>(PC->GetPawn()) : nullptr;
}

void UYJJAutoPilot::Shot(const FString& Name)
{
	const FString Path = FPaths::ProjectSavedDir() / TEXT("Screenshots") / (Name + TEXT(".png"));
	FScreenshotRequest::RequestScreenshot(Path, true, false);
	UE_LOG(LogYJJFantasy, Display, TEXT("[AutoPilot] screenshot %s"), *Path);
}

void UYJJAutoPilot::View(const FVector& HeroLocation, float Yaw, float Pitch)
{
	if (AYJJHero* H = Hero())
	{
		H->WarpTo(HeroLocation, Yaw);
		if (AController* C = H->GetController())
		{
			C->SetControlRotation(FRotator(Pitch, Yaw, 0));
		}
	}
}

void UYJJAutoPilot::Check(bool bOk, const FString& What)
{
	(bOk ? Passed : Failed)++;
	UE_LOG(LogYJJFantasy, Display, TEXT("[AutoPilot] %s %s"), bOk ? TEXT("PASS") : TEXT("FAIL"), *What);
}

void UYJJAutoPilot::StartTour()
{
	Steps.Reset();
	StepIndex = -1;
	StepTimer = 0.f;

	struct FShot { const TCHAR* Name; FVector P; float Yaw; float Pitch; };
	const FShot Shots[] = {
		{ TEXT("Tour_0_Spawn"), AYJJWorldBuilder::GetSpawnLocation(), 45.f, -10.f },
		{ TEXT("Tour_1_Shrine"), W(-700, -1500, 140), 65.f, 8.f },
		{ TEXT("Tour_2_Stones"), W(2400, -300, 140), 15.f, -12.f },
		{ TEXT("Tour_3_Meadow"), W(5400, 300, 500), 30.f, -8.f },
		{ TEXT("Tour_4_Crystal"), W(-1800, 6300, 1520), 115.f, -6.f },
		{ TEXT("Tour_5_Waterfall"), W(-6100, -2100, 2930), 175.f, -2.f },
		{ TEXT("Tour_6_Forest"), W(4000, -9600, -780), 135.f, 22.f },
		{ TEXT("Tour_7_Ruins"), W(8300, -6200, 4420), 35.f, 14.f },
		{ TEXT("Tour_8_SkyView"), W(-2350, -500, 3900), 0.f, -22.f },
		{ TEXT("Tour_9_Vista"), W(1500, -2200, 140), 160.f, 4.f },
	};
	Add(3.f, [] {});
	for (const FShot& S : Shots)
	{
		const FShot Copy = S;
		Add(0.1f, [this, Copy] { View(Copy.P, Copy.Yaw, Copy.Pitch); });
		// 텔레포트 후 Lumen·그림자가 자리 잡을 때까지 기다린다
		Add(2.5f, [this, Copy] {
			if (AYJJHero* H = Hero())
			{
				// 공중 샷은 떨어지지 않게 활공
				if (H->GetCharacterMovement()->IsFalling())
				{
					H->SetGliding(true);
				}
			}
		});
		Add(0.6f, [this, Copy] { Shot(Copy.Name); });
	}
	UE_LOG(LogYJJFantasy, Display, TEXT("[AutoPilot] tour start (%d steps)"), Steps.Num());
}

void UYJJAutoPilot::StartAutoPlay()
{
	Steps.Reset();
	StepIndex = -1;
	StepTimer = 0.f;
	Passed = Failed = 0;

	Add(4.f, [] {});
	Add(0.5f, [this] {
		Check(Hero() != nullptr, TEXT("hero spawned"));
		const UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(GetWorld());
		Check(Adv && Adv->GetLanternCount() == 5, FString::Printf(TEXT("5 lanterns registered (%d)"), Adv ? Adv->GetLanternCount() : -1));
		Check(Adv && Adv->GetStardustTotal() >= 50, FString::Printf(TEXT("stardust placed (%d)"), Adv ? Adv->GetStardustTotal() : -1));
		if (AYJJHero* H = Hero())
		{
			Check(!H->GetCharacterMovement()->IsFalling(), FString::Printf(TEXT("hero standing on hub (z=%.0f)"), H->GetActorLocation().Z - AYJJWorldBuilder::BaseZ));
		}
		Shot(TEXT("Play_0_Start"));
	});

	// 점프 높이: 고목 나선 발판(한 단 165)을 오를 수 있어야 한다
	Add(0.3f, [this] {
		View(W(-1600, -1200, 120), 45.f, -10.f);
	});
	Add(0.6f, [this] {
		if (AYJJHero* H = Hero())
		{
			TrackStartZ = H->GetActorLocation().Z;
			TrackMaxZ = TrackStartZ;
			H->Jump();
		}
	});
	for (int32 i = 0; i < 12; ++i)
	{
		Add(0.05f, [this] { if (AYJJHero* H = Hero()) { TrackMaxZ = FMath::Max(TrackMaxZ, H->GetActorLocation().Z); } });
	}
	Add(0.5f, [this] { Check(TrackMaxZ - TrackStartZ > 180.f, FString::Printf(TEXT("jump height %.0f (need > 180)"), TrackMaxZ - TrackStartZ)); });

	// 활공 + 상승 기류: 중앙섬 서쪽 기류 위에서 활공하면 높이 올라가야 한다
	Add(0.2f, [this] {
		View(W(-2350, -500, 400), 180.f, -10.f);
		if (AYJJHero* H = Hero())
		{
			TrackStartZ = H->GetActorLocation().Z;
			TrackMaxZ = TrackStartZ;
			H->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		}
	});
	Add(0.3f, [this] { if (AYJJHero* H = Hero()) { H->SetGliding(true); } });
	for (int32 i = 0; i < 8; ++i)
	{
		Add(0.5f, [this] { if (AYJJHero* H = Hero()) { TrackMaxZ = FMath::Max(TrackMaxZ, H->GetActorLocation().Z); } });
	}
	Add(0.1f, [this] {
		Check(TrackMaxZ - TrackStartZ > 1500.f, FString::Printf(TEXT("updraft lifts glider (+%.0f)"), TrackMaxZ - TrackStartZ));
		Shot(TEXT("Play_1_Updraft"));
	});

	// 활공 거리: 기류 꼭대기에서 폭포 정원까지 날아갈 수 있는지
	Add(0.1f, [this] {
		View(W(-2350, -500, 3600), 190.f, -10.f);
		if (AYJJHero* H = Hero())
		{
			H->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		}
	});
	Add(0.1f, [this] { if (AYJJHero* H = Hero()) { H->SetGliding(true); } });
	for (int32 i = 0; i < 12; ++i)
	{
		Add(0.5f, [this] {
			if (AYJJHero* H = Hero())
			{
				// 폭포 정원 쪽으로 방향 유지
				const FVector To = (W(-7700, -1900, 2800) - H->GetActorLocation()).GetSafeNormal2D();
				H->SetActorRotation(To.Rotation());
				H->AddMovementInput(To, 1.f);
			}
		});
	}
	Add(2.f, [this] {
		if (AYJJHero* H = Hero())
		{
			const float Dist = FVector::Dist2D(H->GetActorLocation(), W(-7700, -1900, 0));
			Check(H->GetActorLocation().Z > AYJJWorldBuilder::BaseZ + 2500.f && Dist < 3000.f, FString::Printf(TEXT("glide reaches waterfall garden (dist %.0f z=%.0f)"), Dist, H->GetActorLocation().Z - AYJJWorldBuilder::BaseZ));
		}
		Shot(TEXT("Play_2_GlideLanding"));
	});

	// 버섯: 밟으면 높이 튀어야 한다
	Add(0.1f, [this] {
		View(W(-700, 2250, 700), 90.f, -20.f);
		if (AYJJHero* H = Hero())
		{
			TrackStartZ = H->GetActorLocation().Z;
			TrackMaxZ = -1e9f;
			H->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		}
	});
	for (int32 i = 0; i < 10; ++i)
	{
		Add(0.15f, [this] { if (AYJJHero* H = Hero()) { TrackMaxZ = FMath::Max(TrackMaxZ, H->GetActorLocation().Z); } });
	}
	Add(0.1f, [this] { Check(TrackMaxZ > TrackStartZ + 300.f, FString::Printf(TEXT("mushroom bounces (+%.0f)"), TrackMaxZ - TrackStartZ)); });

	// 전투: 들꽃 언덕 슬라임 옆으로 가서 휘두르기
	Add(0.1f, [this] {
		for (TActorIterator<AYJJSlime> It(GetWorld()); It; ++It)
		{
			const FVector P = It->GetActorLocation();
			const FVector Dir = FVector(1, 0, 0);
			View(P - Dir * 180.f + FVector(0, 0, 110), 0.f, -15.f);
			break;
		}
	});
	for (int32 i = 0; i < 8; ++i)
	{
		Add(0.4f, [this] {
			if (AYJJHero* H = Hero())
			{
				// 가장 가까운 슬라임을 바라보고 공격
				AYJJSlime* Best = nullptr;
				float BestD = 1e9f;
				for (TActorIterator<AYJJSlime> It(GetWorld()); It; ++It)
				{
					const float D = FVector::Dist(It->GetActorLocation(), H->GetActorLocation());
					if (It->IsAlive() && D < BestD)
					{
						BestD = D;
						Best = *It;
					}
				}
				if (Best)
				{
					H->SetActorRotation((Best->GetActorLocation() - H->GetActorLocation()).GetSafeNormal2D().Rotation());
				}
				H->DoAttack();
			}
		});
	}
	Add(0.8f, [this] {
		int32 Alive = 0;
		for (TActorIterator<AYJJSlime> It(GetWorld()); It; ++It)
		{
			Alive++;
		}
		Check(Alive < 13, FString::Printf(TEXT("slime defeated (alive %d)"), Alive));
		Shot(TEXT("Play_3_Combat"));
	});

	// 보물 상자: 각 상자 앞에서 열기
	for (int32 i = 0; i < 6; ++i)
	{
		Add(0.2f, [this, i] {
			int32 Index = 0;
			for (TActorIterator<AYJJChest> It(GetWorld()); It; ++It)
			{
				if (Index++ == i)
				{
					View(It->GetActorLocation() + It->GetActorForwardVector() * 170.f + FVector(0, 0, 110), It->GetActorRotation().Yaw + 180.f, -10.f);
					break;
				}
			}
		});
		Add(1.5f, [this, i] {
			if (AYJJHero* H = Hero())
			{
				Check(!H->GetInteractPrompt().IsEmpty(), FString::Printf(TEXT("chest %d prompt '%s'"), i, *H->GetInteractPrompt()));
				H->DoInteract();
			}
		});
		Add(1.2f, [this, i] { if (i == 0) { Shot(TEXT("Play_3b_Chest")); } });
	}
	Add(0.1f, [this] {
		const UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(GetWorld());
		Check(Adv && Adv->GetChestsOpened() == Adv->GetChestTotal() && Adv->GetChestTotal() == 6, FString::Printf(TEXT("chests opened %d/%d"), Adv ? Adv->GetChestsOpened() : -1, Adv ? Adv->GetChestTotal() : -1));
	});

	// 등불: 각 등불 앞에서 상호작용
	for (int32 i = 0; i < 5; ++i)
	{
		Add(0.2f, [this, i] {
			int32 Index = 0;
			for (TActorIterator<AYJJLantern> It(GetWorld()); It; ++It)
			{
				if (Index++ == i)
				{
					View(It->GetRespawnLocation() - It->GetActorForwardVector() * 140.f, (It->GetActorLocation() - It->GetRespawnLocation()).Rotation().Yaw, -5.f);
					break;
				}
			}
		});
		Add(1.8f, [this, i] {
			if (AYJJHero* H = Hero())
			{
				Check(!H->GetInteractPrompt().IsEmpty(), FString::Printf(TEXT("lantern %d prompt '%s'"), i, *H->GetInteractPrompt()));
				H->DoInteract();
			}
		});
		Add(2.f, [this, i] { Shot(FString::Printf(TEXT("Play_4_Lantern%d"), i)); });
	}
	Add(0.5f, [this] {
		const UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(GetWorld());
		Check(Adv && Adv->GetLanternsLit() == 5, FString::Printf(TEXT("all lanterns lit (%d)"), Adv ? Adv->GetLanternsLit() : -1));
	});

	// 엔딩: 성소에서 상호작용
	Add(0.2f, [this] { View(W(0, -420, 260), 90.f, 10.f); });
	Add(1.5f, [this] {
		if (AYJJHero* H = Hero())
		{
			Check(!H->GetInteractPrompt().IsEmpty(), TEXT("shrine prompt"));
			H->DoInteract();
		}
	});
	Add(4.f, [this] { Shot(TEXT("Play_5_FinaleBeam")); });
	Add(9.f, [this] { Shot(TEXT("Play_6_FinaleSky")); });
	Add(8.f, [this] {
		const UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(GetWorld());
		Check(Adv && Adv->GetPhase() == EYJJPhase::Ended, TEXT("ending reached"));
		Shot(TEXT("Play_7_Ending"));
	});

	// 낙사 -> 부활
	Add(2.5f, [this] { View(W(0, 0, -4800), 0.f, 0.f); });
	Add(3.5f, [this] {
		if (AYJJHero* H = Hero())
		{
			Check(H->GetActorLocation().Z > AYJJWorldBuilder::BaseZ - 1000.f, FString::Printf(TEXT("fall respawn (z=%.0f)"), H->GetActorLocation().Z - AYJJWorldBuilder::BaseZ));
		}
	});
	Add(0.1f, [this] { UE_LOG(LogYJJFantasy, Display, TEXT("[AutoPilot] result passed=%d failed=%d avgfps=%.0f"), Passed, Failed, FrameCount / FMath::Max(FrameTime, 0.001f)); });
	UE_LOG(LogYJJFantasy, Display, TEXT("[AutoPilot] autoplay start (%d steps)"), Steps.Num());
}

void UYJJAutoPilot::Tick(float DeltaTime)
{
	// 평균 프레임 (처음 몇 초의 셰이더 컴파일 구간은 뺀다)
	if (StepIndex > 2)
	{
		FrameCount++;
		FrameTime += DeltaTime;
	}
	if (StepIndex >= Steps.Num() || Steps.Num() == 0)
	{
		return;
	}
	StepTimer -= DeltaTime;
	if (StepTimer > 0.f)
	{
		return;
	}
	StepIndex++;
	if (StepIndex >= Steps.Num())
	{
		UE_LOG(LogYJJFantasy, Display, TEXT("[AutoPilot] done"));
		if (bQuitWhenDone)
		{
			UKismetSystemLibrary::QuitGame(GetWorld(), GetWorld()->GetFirstPlayerController(), EQuitPreference::Quit, false);
		}
		return;
	}
	Steps[StepIndex].Action();
	StepTimer = Steps[StepIndex].Wait;
}
