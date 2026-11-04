#include "Gameplay/YJJTraversal.h"

#include "Components/StaticMeshComponent.h"
#include "Core/YJJAudio.h"
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
	const FLinearColor Wind(0.65f, 0.95f, 1.f);

	// 바닥의 바람 문양
	YJJShapes::Add(this, RootComponent, EYJJShape::Cylinder, FVector(0, 0, 5), FVector(Radius * 2.f, Radius * 2.f, 6), YJJShapes::Glow(this, Wind, 1.2f, 0.6f, 0.4f), false);
	// 은은한 기둥
	YJJShapes::Add(this, RootComponent, EYJJShape::Cylinder, FVector(0, 0, Height * 0.5f), FVector(Radius * 2.f, Radius * 2.f, Height), YJJShapes::Glow(this, Wind, 0.22f, 0.f, 1.f), false);

	// 위로 올라가는 바람 고리
	UMaterialInterface* RingMat = YJJShapes::Glow(this, Wind, 1.4f, 0.f, 1.f);
	for (int32 i = 0; i < 7; ++i)
	{
		Rings.Add(YJJShapes::Add(this, RootComponent, EYJJShape::Cylinder, FVector::ZeroVector, FVector(Radius * 1.6f, Radius * 1.6f, 10), RingMat, false));
	}
	// 바람 줄기
	UMaterialInterface* StreakMat = YJJShapes::Glow(this, FLinearColor(0.9f, 1.f, 1.f), 3.f, 1.f, 0.f);
	for (int32 i = 0; i < 14; ++i)
	{
		Streaks.Add(YJJShapes::Add(this, RootComponent, EYJJShape::Cube, FVector::ZeroVector, FVector(6, 6, 220), StreakMat, false));
	}
}

bool AYJJUpdraft::IsInside(const FVector& P) const
{
	const FVector Base = GetActorLocation();
	// 판정은 보이는 기둥보다 조금 넉넉하게
	return FVector::Dist2D(P, Base) < Radius * 1.25f && P.Z > Base.Z - 200.f && P.Z < Base.Z + Height + 300.f;
}

void AYJJUpdraft::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Time += DeltaTime;

	for (int32 i = 0; i < Rings.Num(); ++i)
	{
		const float T = FMath::Fmod(Time * 0.35f + i / static_cast<float>(Rings.Num()), 1.f);
		Rings[i]->SetRelativeLocation(FVector(0, 0, T * Height));
		const float S = (0.5f + T * 0.7f) * Radius * 1.6f / 100.f;
		Rings[i]->SetRelativeScale3D(FVector(S, S, 0.06f));
	}
	for (int32 i = 0; i < Streaks.Num(); ++i)
	{
		const float Seed = YJJShapes::Hash(i * 13 + 5);
		const float T = FMath::Fmod(Time * (0.5f + Seed * 0.4f) + Seed, 1.f);
		const float A = Seed * TWO_PI + Time * 0.8f;
		const float R = Radius * (0.3f + 0.6f * YJJShapes::Hash(i * 7 + 1));
		Streaks[i]->SetRelativeLocation(FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, T * Height));
		Streaks[i]->SetRelativeScale3D(FVector(0.06f, 0.06f, 2.2f * FMath::Sin(T * PI)));
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
	YJJShapes::Add(this, RootComponent, EYJJShape::Cylinder, FVector(0, 0, 70 * S), FVector(80 * S, 80 * S, 140 * S), YJJShapes::Surface(this, FLinearColor(0.98f, 0.94f, 0.85f), FLinearColor(0.92f, 0.86f, 0.74f)), true);

	CapPivot = NewObject<USceneComponent>(this);
	CapPivot->SetMobility(EComponentMobility::Movable);
	CapPivot->SetupAttachment(RootComponent);
	CapPivot->SetRelativeLocation(FVector(0, 0, 150 * S));
	CapPivot->RegisterComponent();
	AddInstanceComponent(CapPivot);

	YJJShapes::Add(this, CapPivot, EYJJShape::Sphere, FVector::ZeroVector, FVector(300 * S, 300 * S, 130 * S),
		YJJShapes::Surface(this, CapColor, YJJShapes::Shade(CapColor, 0.6f), 0.25f), true);
	// 흰 점무늬
	UMaterialInterface* Dot = YJJShapes::Solid(this, FLinearColor(1.f, 0.97f, 0.9f), 0.2f);
	for (int32 i = 0; i < 7; ++i)
	{
		const float A = i * 2.399963f;
		const float R = (i == 0) ? 0.f : 70.f + (i % 3) * 25.f;
		const float Z = 58.f - R * R / 2400.f;
		YJJShapes::Add(this, CapPivot, EYJJShape::Sphere, FVector(FMath::Cos(A) * R * S, FMath::Sin(A) * R * S, Z * S), FVector(48 * S, 48 * S, 22 * S), Dot, false);
	}
}

void AYJJBouncePad::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Cooldown = FMath::Max(0.f, Cooldown - DeltaTime);
	Squash = FMath::FInterpTo(Squash, 0.f, DeltaTime, 6.f);
	const float Wobble = Squash * FMath::Cos(GetWorld()->GetTimeSeconds() * 30.f);
	CapPivot->SetRelativeScale3D(FVector(1.f + Wobble * 0.25f, 1.f + Wobble * 0.25f, 1.f - Wobble * 0.35f));

	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	ACharacter* Hero = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
	if (!Hero || Cooldown > 0.f)
	{
		return;
	}

	// 갓 위에 서 있거나 막 떨어졌으면 튕긴다
	const FVector Local = Hero->GetActorLocation() - CapPivot->GetComponentLocation();
	const float CapTop = 65.f * Size + 96.f;
	if (FVector(Local.X, Local.Y, 0).Size() < 140.f * Size && Local.Z > 0.f && Local.Z < CapTop + 40.f && Hero->GetVelocity().Z <= 50.f)
	{
		Hero->LaunchCharacter(FVector(0, 0, LaunchSpeed), false, true);
		Squash = 1.f;
		Cooldown = 0.35f;
		YJJAudio::Play(this, EYJJSound::Bounce, FVector::ZeroVector, 0.9f, FMath::FRandRange(0.95f, 1.08f));
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
	Body = YJJShapes::Add(this, RootComponent, EYJJShape::Cylinder, FVector(0, 0, -30), FVector(D, D, 60), YJJShapes::Surface(this, FLinearColor(0.5f, 0.78f, 0.36f), FLinearColor(0.6f, 0.55f, 0.5f)), true);
	YJJShapes::Add(this, RootComponent, EYJJShape::Cone, FVector(0, 0, -60 - Radius * 0.5f), FVector(D * 0.9f, D * 0.9f, Radius), YJJShapes::Surface(this, FLinearColor(0.6f, 0.55f, 0.5f), FLinearColor(0.55f, 0.5f, 0.48f)), false, FRotator(180, 0, 0));
	// 아래에 매달린 작은 빛 결정
	YJJShapes::Add(this, RootComponent, EYJJShape::Cone, FVector(0, 0, -60 - Radius * 1.05f), FVector(40, 40, 80), YJJShapes::Solid(this, FLinearColor(0.4f, 0.9f, 1.f), 3.f), false, FRotator(180, 0, 0));
}

void AYJJMovingPlatform::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Time += DeltaTime;
	const float Alpha = 0.5f - 0.5f * FMath::Cos(Time * TWO_PI / Period);
	SetActorLocation(Start + Offset * Alpha);
}
