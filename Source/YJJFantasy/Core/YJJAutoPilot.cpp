#include "Core/YJJAutoPilot.h"

#include "Core/YJJAdventureSubsystem.h"
#include "Data/YJJData.h"
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
#include "World/YJJWorldLayout.h"
#include "YJJFantasy.h"

FAutoConsoleCommandWithWorld UYJJAutoPilot::TourCommand(TEXT("yjj.Tour"), TEXT("명소 스크린샷 투어"), FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
{
	if (UYJJAutoPilot* Pilot = World ? World->GetSubsystem<UYJJAutoPilot>() : nullptr)
	{
		Pilot->StartTour();
	}
}));

FAutoConsoleCommandWithWorld UYJJAutoPilot::PlayCommand(TEXT("yjj.AutoPlay"), TEXT("자동 플레이 검증"), FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
{
	if (UYJJAutoPilot* Pilot = World ? World->GetSubsystem<UYJJAutoPilot>() : nullptr)
	{
		Pilot->StartAutoPlay();
	}
}));

FAutoConsoleCommandWithWorldAndArgs UYJJAutoPilot::ToonCommand(TEXT("yjj.Toon"), TEXT("셀 셰이딩 조절: Ref Strength Outline"), FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
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

FAutoConsoleCommandWithWorld UYJJAutoPilot::LightAllCommand(TEXT("yjj.LightAll"), TEXT("모든 등불 켜기"), FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
{
	for (TActorIterator<AYJJLantern> It(World); It; ++It)
	{
		It->Interact();
	}
}));

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

	Add(Pace.TourStart, [] {});
	for (const TPair<FName, const FYJJTourRow*>& Spot : FYJJData::Tour())
	{
		const FString Name = Spot.Key.ToString();
		const FVector P = AYJJWorldBuilder::W(Spot.Value->X, Spot.Value->Y, Spot.Value->Z);
		const float Yaw = Spot.Value->Yaw;
		const float Pitch = Spot.Value->Pitch;
		Add(Pace.Teleport, [this, P, Yaw, Pitch] { View(P, Yaw, Pitch); });
		// 텔레포트 후 Lumen·그림자가 자리 잡을 때까지 기다린다
		Add(Pace.LightSettle, [this] {
			if (AYJJHero* H = Hero())
			{
				// 공중 샷은 떨어지지 않게 활공
				if (H->GetCharacterMovement()->IsFalling())
				{
					H->SetGliding(true);
				}
			}
		});
		Add(Pace.Shot, [this, Name] { Shot(Name); });
	}
	UE_LOG(LogYJJFantasy, Display, TEXT("[AutoPilot] tour start (%d steps)"), Steps.Num());
}

void UYJJAutoPilot::ViewPoint(const TCHAR* Name)
{
	if (const FYJJTourRow* Row = FYJJData::TestPoint(FName(Name)))
	{
		View(AYJJWorldBuilder::W(Row->X, Row->Y, Row->Z), Row->Yaw, Row->Pitch);
	}
}

void UYJJAutoPilot::TrackHeight()
{
	if (AYJJHero* H = Hero())
	{
		TrackMaxZ = FMath::Max(TrackMaxZ, H->GetActorLocation().Z);
	}
}

float UYJJAutoPilot::Test(const TCHAR* Name)
{
	return FYJJData::Tune(*FString::Printf(TEXT("Test.%s"), Name));
}

void UYJJAutoPilot::StartAutoPlay()
{
	Steps.Reset();
	StepIndex = -1;
	StepTimer = 0.f;
	Passed = Failed = 0;
	// 기대 개수는 Placements.csv 에서 센다 (배치를 바꾸면 테스트도 따라간다)
	const int32 LanternCount = FYJJData::CountPlacements(TEXT("Lantern"));
	const int32 ChestCount = FYJJData::CountPlacements(TEXT("Chest"));
	const int32 MobCount = FYJJData::CountPlacements(TEXT("Mob"));

	Add(Pace.Boot, [] {});
	Add(Pace.Check, [this, LanternCount] {
		Check(Hero() != nullptr, TEXT("hero spawned"));
		const UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(GetWorld());
		Check(Adv && Adv->GetLanternCount() == LanternCount, FString::Printf(TEXT("%d lanterns registered (%d)"), LanternCount, Adv ? Adv->GetLanternCount() : INDEX_NONE));
		Check(Adv && Adv->GetStardustTotal() >= Test(TEXT("MinStardust")), FString::Printf(TEXT("stardust placed (%d)"), Adv ? Adv->GetStardustTotal() : INDEX_NONE));
		if (AYJJHero* H = Hero())
		{
			Check(!H->GetCharacterMovement()->IsFalling(), FString::Printf(TEXT("hero standing on hub (z=%.0f)"), H->GetActorLocation().Z - AYJJWorldBuilder::BaseZ));
		}
		Shot(TEXT("Play_0_Start"));
	});

	// 점프 높이: 고목 나선 발판을 오를 수 있어야 한다
	Add(Pace.Short, [this] { ViewPoint(TEXT("JumpSpot")); });
	Add(Pace.Land, [this] {
		if (AYJJHero* H = Hero())
		{
			TrackStartZ = H->GetActorLocation().Z;
			TrackMaxZ = TrackStartZ;
			H->Jump();
		}
	});
	for (int32 i = 0; i < Pace.JumpSamples; ++i)
	{
		Add(Pace.JumpSample, [this] { TrackHeight(); });
	}
	Add(Pace.Check, [this] {
		const float Need = Test(TEXT("JumpMinHeight"));
		Check(TrackMaxZ - TrackStartZ > Need, FString::Printf(TEXT("jump height %.0f (need > %.0f)"), TrackMaxZ - TrackStartZ, Need));
	});

	// 활공 + 상승 기류: 중앙섬 서쪽 기류 위에서 활공하면 높이 올라가야 한다
	Add(Pace.Short, [this] {
		ViewPoint(TEXT("UpdraftSpot"));
		if (AYJJHero* H = Hero())
		{
			TrackStartZ = H->GetActorLocation().Z;
			TrackMaxZ = TrackStartZ;
			H->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		}
	});
	Add(Pace.Short, [this] { if (AYJJHero* H = Hero()) { H->SetGliding(true); } });
	for (int32 i = 0; i < Pace.UpdraftSamples; ++i)
	{
		Add(Pace.FlightSample, [this] { TrackHeight(); });
	}
	Add(Pace.Teleport, [this] {
		Check(TrackMaxZ - TrackStartZ > Test(TEXT("UpdraftMinLift")), FString::Printf(TEXT("updraft lifts glider (+%.0f)"), TrackMaxZ - TrackStartZ));
		Shot(TEXT("Play_1_Updraft"));
	});

	// 활공 거리: 기류 꼭대기에서 폭포 정원까지 날아갈 수 있는지
	const FYJJIslandRow* Garden = FYJJData::Island(FName(TEXT("Waterfall")));
	const FVector GardenCenter = Garden ? AYJJWorldBuilder::W(Garden->X, Garden->Y, Garden->Z) : FVector::ZeroVector;
	Add(Pace.Teleport, [this] {
		ViewPoint(TEXT("GlideStart"));
		if (AYJJHero* H = Hero())
		{
			H->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		}
	});
	Add(Pace.Teleport, [this] { if (AYJJHero* H = Hero()) { H->SetGliding(true); } });
	for (int32 i = 0; i < Pace.GlideSteps; ++i)
	{
		Add(Pace.FlightSample, [this, GardenCenter] {
			if (AYJJHero* H = Hero())
			{
				// 폭포 정원 쪽으로 방향 유지
				const FVector To = (GardenCenter - H->GetActorLocation()).GetSafeNormal2D();
				H->SetActorRotation(To.Rotation());
				H->AddMovementInput(To, 1.f);
			}
		});
	}
	Add(Pace.GlideLand, [this, GardenCenter] {
		if (AYJJHero* H = Hero())
		{
			const float Dist = FVector::Dist2D(H->GetActorLocation(), GardenCenter);
			const float Above = H->GetActorLocation().Z - AYJJWorldBuilder::BaseZ;
			Check(Above > Test(TEXT("GlideArriveAbove")) && Dist < Test(TEXT("GlideArriveRadius")), FString::Printf(TEXT("glide reaches waterfall garden (dist %.0f z=%.0f)"), Dist, Above));
		}
		Shot(TEXT("Play_2_GlideLanding"));
	});

	// 버섯: 밟으면 높이 튀어야 한다
	Add(Pace.Teleport, [this] {
		ViewPoint(TEXT("MushroomSpot"));
		if (AYJJHero* H = Hero())
		{
			TrackStartZ = H->GetActorLocation().Z;
			TrackMaxZ = TNumericLimits<float>::Lowest();
			H->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		}
	});
	for (int32 i = 0; i < Pace.BounceSamples; ++i)
	{
		Add(Pace.BounceSample, [this] { TrackHeight(); });
	}
	Add(Pace.Teleport, [this] { Check(TrackMaxZ > TrackStartZ + Test(TEXT("BounceMinRise")), FString::Printf(TEXT("mushroom bounces (+%.0f)"), TrackMaxZ - TrackStartZ)); });

	// 전투: 첫 떡무지 옆으로 가서 휘두르기
	Add(Pace.Teleport, [this] {
		for (TActorIterator<AYJJSlime> It(GetWorld()); It; ++It)
		{
			View(It->GetActorLocation() - FVector::ForwardVector * Test(TEXT("CombatStandOff")) + FVector(0, 0, Test(TEXT("StandLift"))), 0.f, -Pace.CombatLookDown);
			break;
		}
	});
	for (int32 i = 0; i < Pace.Swings; ++i)
	{
		Add(Pace.Swing, [this] {
			if (AYJJHero* H = Hero())
			{
				// 가장 가까운 떡무지를 바라보고 공격
				AYJJSlime* Best = nullptr;
				float BestD = TNumericLimits<float>::Max();
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
	Add(Pace.AfterCombat, [this, MobCount] {
		int32 Alive = 0;
		for (TActorIterator<AYJJSlime> It(GetWorld()); It; ++It)
		{
			Alive++;
		}
		Check(Alive < MobCount, FString::Printf(TEXT("slime defeated (alive %d)"), Alive));
		Shot(TEXT("Play_3_Combat"));
	});

	// 보물 상자: 각 상자 앞에서 열기
	for (int32 i = 0; i < ChestCount; ++i)
	{
		Add(Pace.Short, [this, i] {
			int32 Index = 0;
			for (TActorIterator<AYJJChest> It(GetWorld()); It; ++It)
			{
				if (Index++ == i)
				{
					View(It->GetActorLocation() + It->GetActorForwardVector() * Test(TEXT("ChestStandOff")) + FVector(0, 0, Test(TEXT("StandLift"))), It->GetActorRotation().Yaw + YJJWorldLayout::HalfTurn, -Pace.ChestLookDown);
					break;
				}
			}
		});
		Add(Pace.Interact, [this, i] {
			if (AYJJHero* H = Hero())
			{
				Check(!H->GetInteractPrompt().IsEmpty(), FString::Printf(TEXT("chest %d prompt '%s'"), i, *H->GetInteractPrompt()));
				H->DoInteract();
			}
		});
		Add(Pace.ChestOpen, [this, i] { if (i == 0) { Shot(TEXT("Play_3b_Chest")); } });
	}
	Add(Pace.Teleport, [this, ChestCount] {
		const UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(GetWorld());
		Check(Adv && Adv->GetChestsOpened() == Adv->GetChestTotal() && Adv->GetChestTotal() == ChestCount, FString::Printf(TEXT("chests opened %d/%d"), Adv ? Adv->GetChestsOpened() : INDEX_NONE, Adv ? Adv->GetChestTotal() : INDEX_NONE));
	});

	// 등불: 각 등불 앞에서 상호작용
	for (int32 i = 0; i < LanternCount; ++i)
	{
		Add(Pace.Short, [this, i] {
			int32 Index = 0;
			for (TActorIterator<AYJJLantern> It(GetWorld()); It; ++It)
			{
				if (Index++ == i)
				{
					View(It->GetRespawnLocation() - It->GetActorForwardVector() * Test(TEXT("LanternStandOff")), (It->GetActorLocation() - It->GetRespawnLocation()).Rotation().Yaw, -Pace.LanternLookDown);
					break;
				}
			}
		});
		Add(Pace.LanternInteract, [this, i] {
			if (AYJJHero* H = Hero())
			{
				Check(!H->GetInteractPrompt().IsEmpty(), FString::Printf(TEXT("lantern %d prompt '%s'"), i, *H->GetInteractPrompt()));
				H->DoInteract();
			}
		});
		Add(Pace.LanternGlow, [this, i] { Shot(FString::Printf(TEXT("Play_4_Lantern%d"), i)); });
	}
	Add(Pace.Check, [this, LanternCount] {
		const UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(GetWorld());
		Check(Adv && Adv->GetLanternsLit() == LanternCount, FString::Printf(TEXT("all lanterns lit (%d)"), Adv ? Adv->GetLanternsLit() : INDEX_NONE));
	});

	// 엔딩: 성소에서 상호작용
	Add(Pace.Short, [this] { ViewPoint(TEXT("ShrineSpot")); });
	Add(Pace.Interact, [this] {
		if (AYJJHero* H = Hero())
		{
			Check(!H->GetInteractPrompt().IsEmpty(), TEXT("shrine prompt"));
			H->DoInteract();
		}
	});
	Add(Pace.FinaleBeam, [this] { Shot(TEXT("Play_5_FinaleBeam")); });
	Add(Pace.FinaleSky, [this] { Shot(TEXT("Play_6_FinaleSky")); });
	Add(Pace.FinaleEnd, [this] {
		const UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(GetWorld());
		Check(Adv && Adv->GetPhase() == EYJJPhase::Ended, TEXT("ending reached"));
		Shot(TEXT("Play_7_Ending"));
	});

	// 낙사 -> 부활
	Add(Pace.Fall, [this] { ViewPoint(TEXT("AbyssSpot")); });
	Add(Pace.Respawn, [this] {
		if (AYJJHero* H = Hero())
		{
			Check(H->GetActorLocation().Z > AYJJWorldBuilder::BaseZ - Test(TEXT("RespawnAbove")), FString::Printf(TEXT("fall respawn (z=%.0f)"), H->GetActorLocation().Z - AYJJWorldBuilder::BaseZ));
		}
	});
	Add(Pace.Teleport, [this] { UE_LOG(LogYJJFantasy, Display, TEXT("[AutoPilot] result passed=%d failed=%d avgfps=%.0f"), Passed, Failed, FrameCount / FMath::Max(FrameTime, Pace.MinFrameTime)); });
	UE_LOG(LogYJJFantasy, Display, TEXT("[AutoPilot] autoplay start (%d steps)"), Steps.Num());
}

void UYJJAutoPilot::Tick(float DeltaTime)
{
	// 평균 프레임 (처음 몇 초의 셰이더 컴파일 구간은 뺀다)
	if (StepIndex > Pace.SkipFirstSteps)
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
