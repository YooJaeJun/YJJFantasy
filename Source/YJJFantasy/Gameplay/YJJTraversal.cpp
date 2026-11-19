#include "Gameplay/YJJTraversal.h"

#include "Components/StaticMeshComponent.h"
#include "Core/YJJAudio.h"
#include "Data/YJJData.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "World/YJJShapes.h"

// ============================================================ 상승 기류

AYJJUpdraft::AYJJUpdraft()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);
}

void AYJJUpdraft::Setup(float InRadius, float InHeight)
{
	Radius = InRadius;
	Height = InHeight;
}

void AYJJUpdraft::BeginPlay()
{
	Super::BeginPlay();
	const FLinearColor Wind = FYJJData::Color(TEXT("UpdraftWind"));

	// 바닥의 바람 문양
	YJJShapes::Add(this, RootComponent, EYJJShape::Cylinder, FVector(0, 0, FloorMarkLift), FVector(Radius * 2.f, Radius * 2.f, FloorMarkThickness), YJJShapes::Glow(this, Wind, FloorMarkGlow, FloorMarkCore, FloorMarkRim), false);
	// 은은한 기둥
	YJJShapes::Add(this, RootComponent, EYJJShape::Cylinder, FVector(0, 0, Height * 0.5f), FVector(Radius * 2.f, Radius * 2.f, Height), YJJShapes::Glow(this, Wind, ColumnGlow, 0.f, 1.f), false);

	// 위로 올라가는 바람 고리
	UMaterialInterface* RingMat = YJJShapes::Glow(this, Wind, RingGlow, 0.f, 1.f);
	for (int32 i = 0; i < RingCount; ++i)
	{
		Rings.Add(YJJShapes::Add(this, RootComponent, EYJJShape::Cylinder, FVector::ZeroVector, FVector(Radius * RingWidth, Radius * RingWidth, RingThickness), RingMat, false));
	}
	// 바람 줄기
	UMaterialInterface* StreakMat = YJJShapes::Glow(this, FYJJData::Color(TEXT("UpdraftStreak")), StreakGlow, 1.f, 0.f);
	for (int32 i = 0; i < StreakCount; ++i)
	{
		Streaks.Add(YJJShapes::Add(this, RootComponent, EYJJShape::Cube, FVector::ZeroVector, FVector(StreakThickness, StreakThickness, StreakLength), StreakMat, false));
	}
}

bool AYJJUpdraft::IsInside(const FVector& P) const
{
	const FVector Base = GetActorLocation();
	// 판정은 보이는 기둥보다 조금 넉넉하게
	return FVector::Dist2D(P, Base) < Radius * FYJJData::Tune(TEXT("Updraft.RadiusMargin")) && P.Z > Base.Z - FYJJData::Tune(TEXT("Updraft.BelowMargin")) && P.Z < Base.Z + Height + FYJJData::Tune(TEXT("Updraft.AboveMargin"));
}

void AYJJUpdraft::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Time += DeltaTime;

	for (int32 i = 0; i < Rings.Num(); ++i)
	{
		const float T = FMath::Fmod(Time * RingRiseSpeed + i / static_cast<float>(Rings.Num()), 1.f);
		Rings[i]->SetRelativeLocation(FVector(0, 0, T * Height));
		const float S = (RingStartScale + T * RingGrow) * Radius * RingWidth / YJJShapes::BaseShapeSize;
		Rings[i]->SetRelativeScale3D(FVector(S, S, RingFlat));
	}
	for (int32 i = 0; i < Streaks.Num(); ++i)
	{
		const float Seed = YJJShapes::Hash(i * HashStreak + HashStreakSalt);
		const float T = FMath::Fmod(Time * (StreakRiseMin + Seed * StreakRiseRange) + Seed, 1.f);
		const float A = Seed * TWO_PI + Time * StreakSwirl;
		const float R = Radius * (StreakRadiusMin + StreakRadiusRange * YJJShapes::Hash(i * HashStreakRadius + 1));
		Streaks[i]->SetRelativeLocation(FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, T * Height));
		const float Thin = StreakThickness / YJJShapes::BaseShapeSize;
		Streaks[i]->SetRelativeScale3D(FVector(Thin, Thin, StreakLength / YJJShapes::BaseShapeSize * FMath::Sin(T * PI)));
	}
}

// ============================================================ 튕김 버섯

AYJJBouncePad::AYJJBouncePad()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);
}

void AYJJBouncePad::Setup(float InLaunchSpeed, float InSize, const FLinearColor& InCapColor)
{
	LaunchSpeed = InLaunchSpeed;
	Size = InSize;
	CapColor = InCapColor;
}

void AYJJBouncePad::BeginPlay()
{
	Super::BeginPlay();
	const float S = Size;
	YJJShapes::Add(this, RootComponent, EYJJShape::Cylinder, FVector(0, 0, StemHeight * 0.5f * S), FVector(StemWidth * S, StemWidth * S, StemHeight * S), YJJShapes::Surface(this, FYJJData::Color(TEXT("MushroomStem")), FYJJData::Color(TEXT("MushroomStemSide"))), true);

	CapPivot = NewObject<USceneComponent>(this);
	CapPivot->SetMobility(EComponentMobility::Movable);
	CapPivot->SetupAttachment(RootComponent);
	CapPivot->SetRelativeLocation(FVector(0, 0, CapPivotHeight * S));
	CapPivot->RegisterComponent();
	AddInstanceComponent(CapPivot);

	YJJShapes::Add(this, CapPivot, EYJJShape::Sphere, FVector::ZeroVector, CapSize * S,
		YJJShapes::Surface(this, CapColor, YJJShapes::Shade(CapColor, CapSideShade), CapEmissive), true);
	// 흰 점무늬
	UMaterialInterface* Dot = YJJShapes::Solid(this, FYJJData::Color(TEXT("MushroomDot")), DotEmissive);
	for (int32 i = 0; i < DotCount; ++i)
	{
		const float A = i * GoldenAngle;
		const float R = (i == 0) ? 0.f : DotRingMin + (i % DotRings) * DotRingStep;
		// 갓 곡면을 따라 붙인다 (포물선 근사)
		const float Z = DotTopHeight - R * R / DotCurve;
		YJJShapes::Add(this, CapPivot, EYJJShape::Sphere, FVector(FMath::Cos(A) * R * S, FMath::Sin(A) * R * S, Z * S), DotSize * S, Dot, false);
	}
}

void AYJJBouncePad::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Cooldown = FMath::Max(0.f, Cooldown - DeltaTime);
	Squash = FMath::FInterpTo(Squash, 0.f, DeltaTime, SquashDecay);
	const float Wobble = Squash * FMath::Cos(GetWorld()->GetTimeSeconds() * WobbleSpeed);
	CapPivot->SetRelativeScale3D(FVector(1.f + Wobble * WobbleWiden, 1.f + Wobble * WobbleWiden, 1.f - Wobble * WobbleSquash));

	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	ACharacter* Hero = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
	if (!Hero || Cooldown > 0.f)
	{
		return;
	}

	// 갓 위에 서 있거나 막 떨어졌으면 튕긴다
	const FVector Local = Hero->GetActorLocation() - CapPivot->GetComponentLocation();
	// 갓 윗면 + 캡슐 반 높이
	const float CapTop = CapSize.Z * 0.5f * Size + Hero->GetSimpleCollisionHalfHeight();
	if (FVector(Local.X, Local.Y, 0).Size() < FYJJData::Tune(TEXT("Bounce.Radius")) * Size && Local.Z > 0.f && Local.Z < CapTop + FYJJData::Tune(TEXT("Bounce.HeightMargin")) && Hero->GetVelocity().Z <= FYJJData::Tune(TEXT("Bounce.MaxRiseSpeed")))
	{
		Hero->LaunchCharacter(FVector(0, 0, LaunchSpeed), false, true);
		Squash = 1.f;
		Cooldown = FYJJData::Tune(TEXT("Bounce.Cooldown"));
		YJJAudio::Play(this, EYJJSound::Bounce, FVector::ZeroVector, FYJJData::Tune(TEXT("Bounce.Volume")), FMath::FRandRange(PitchMin, PitchMax));
	}
}

// ============================================================ 움직이는 발판

AYJJMovingPlatform::AYJJMovingPlatform()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);
}

void AYJJMovingPlatform::Setup(const FVector& InOffset, float InPeriod, float InRadius, float InPhase)
{
	Offset = InOffset;
	Period = InPeriod;
	Radius = InRadius;
	Phase = InPhase;
}

void AYJJMovingPlatform::BeginPlay()
{
	Super::BeginPlay();
	Start = GetActorLocation();
	Time = Phase * Period;

	const float D = Radius * 2.f;
	Body = YJJShapes::Add(this, RootComponent, EYJJShape::Cylinder, FVector(0, 0, -SlabThickness * 0.5f), FVector(D, D, SlabThickness), YJJShapes::Surface(this, FYJJData::Color(TEXT("PlatformGrass")), FYJJData::Color(TEXT("PlatformRock"))), true);
	YJJShapes::Add(this, RootComponent, EYJJShape::Cone, FVector(0, 0, -SlabThickness - Radius * 0.5f), FVector(D * RootWidth, D * RootWidth, Radius), YJJShapes::Surface(this, FYJJData::Color(TEXT("PlatformRock")), FYJJData::Color(TEXT("PlatformRockSide"))), false, UpsideDown);
	// 아래에 매달린 작은 빛 결정
	YJJShapes::Add(this, RootComponent, EYJJShape::Cone, FVector(0, 0, -SlabThickness - Radius * GemDrop), GemSize, YJJShapes::Solid(this, FYJJData::Color(TEXT("PlatformGem")), GemGlow), false, UpsideDown);
}

void AYJJMovingPlatform::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Time += DeltaTime;
	const float Alpha = 0.5f - 0.5f * FMath::Cos(Time * TWO_PI / Period);
	SetActorLocation(Start + Offset * Alpha);
}
