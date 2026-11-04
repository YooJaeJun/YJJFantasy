#include "Gameplay/YJJSlime.h"

#include "Components/StaticMeshComponent.h"
#include "Core/YJJAudio.h"
#include "Engine/World.h"
#include "Gameplay/YJJStardust.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Player/YJJHero.h"
#include "World/YJJShapes.h"

namespace
{
	constexpr float SightRange = 1300.f;
	constexpr float ChaseSpeed = 330.f;
	constexpr float WanderSpeed = 140.f;
	constexpr float HopPeriod = 0.55f;
}

AYJJSlime::AYJJSlime()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);
}

void AYJJSlime::Setup(const FLinearColor& InColor, float InHomeRadius, float InSize)
{
	Color = InColor;
	HomeRadius = InHomeRadius;
	Size = InSize;
}

void AYJJSlime::BeginPlay()
{
	Super::BeginPlay();
	Home = GetActorLocation();
	Target = Home;
	// 지어지는 같은 프레임엔 물리 검색이 땅을 못 볼 수 있다. 스폰 위치가 곧 땅 위다
	if (!FindGround(Home, GroundZ))
	{
		GroundZ = Home.Z;
	}
	HopTime = YJJShapes::Hash(GetUniqueID()) * HopPeriod;

	BodyPivot = NewObject<USceneComponent>(this);
	BodyPivot->SetMobility(EComponentMobility::Movable);
	BodyPivot->SetupAttachment(RootComponent);
	BodyPivot->RegisterComponent();
	AddInstanceComponent(BodyPivot);

	const float S = Size;
	BodyMID = YJJShapes::Surface(this, YJJShapes::Shade(Color, 1.15f), Color, 0.35f, 0.25f);
	Body = YJJShapes::Add(this, BodyPivot, EYJJShape::Sphere, FVector(0, 0, 55 * S), FVector(130 * S, 130 * S, 110 * S), BodyMID, false);
	// 반짝이는 하이라이트
	YJJShapes::Add(this, BodyPivot, EYJJShape::Sphere, FVector(-20 * S, -28 * S, 92 * S), FVector(26 * S, 20 * S, 14 * S), YJJShapes::Solid(this, FLinearColor::White, 2.f), false);
	// 눈 (멍한 점 두 개) + 볼터치
	UMaterialInterface* Eye = YJJShapes::Solid(this, FLinearColor(0.05f, 0.05f, 0.08f), 0.f, 0.3f);
	UMaterialInterface* Blush = YJJShapes::Solid(this, FLinearColor(1.f, 0.55f, 0.6f), 0.3f);
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		YJJShapes::Add(this, BodyPivot, EYJJShape::Sphere, FVector(58 * S, Side * 24 * S, 66 * S), FVector(14 * S, 14 * S, 22 * S), Eye, false);
		YJJShapes::Add(this, BodyPivot, EYJJShape::Sphere, FVector(54 * S, Side * 42 * S, 50 * S), FVector(8 * S, 18 * S, 10 * S), Blush, false);
	}
	// 바닥 그림자 (점프할 때 위치를 알기 쉽게)
	Shadow = YJJShapes::Add(this, RootComponent, EYJJShape::Cylinder, FVector(0, 0, 2), FVector(120 * S, 120 * S, 2), YJJShapes::Solid(this, FLinearColor(0.05f, 0.08f, 0.06f), 0.f), false);

	PuffMID = YJJShapes::Glow(this, YJJShapes::Shade(Color, 1.3f), 0.f, 0.6f, 1.f);
	for (int32 i = 0; i < 10; ++i)
	{
		UStaticMeshComponent* Puff = YJJShapes::Add(this, RootComponent, EYJJShape::Sphere, FVector::ZeroVector, FVector(50, 50, 50), PuffMID, false);
		Puff->SetVisibility(false);
		Puffs.Add(Puff);
	}
}

bool AYJJSlime::FindGround(const FVector& At, float& OutZ) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(TEXT("SlimeGround"), false, this);
	if (GetWorld()->LineTraceSingleByObjectType(Hit, At + FVector(0, 0, 300), At - FVector(0, 0, 600), FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		if (Hit.ImpactNormal.Z > 0.7f)
		{
			OutZ = Hit.ImpactPoint.Z;
			return true;
		}
	}
	return false;
}

bool AYJJSlime::TakeHit(const FVector& From)
{
	if (Health <= 0)
	{
		return false;
	}
	Health--;
	Flash = 1.f;
	FVector Away = GetActorLocation() - From;
	Away.Z = 0.f;
	KnockVelocity = Away.GetSafeNormal() * 900.f;
	YJJAudio::Play(this, EYJJSound::SlimeHit, GetActorLocation(), 0.9f, FMath::FRandRange(0.9f, 1.15f));
	if (Health <= 0)
	{
		Die();
		return true;
	}
	return false;
}

void AYJJSlime::Die()
{
	DeathTime = 0.f;
	BodyPivot->SetVisibility(false, true);
	Shadow->SetVisibility(false);
	for (UStaticMeshComponent* Puff : Puffs)
	{
		Puff->SetVisibility(true);
	}
	YJJAudio::Play(this, EYJJSound::SlimePop, GetActorLocation());

	// 별가루 두세 개를 튕겨 낸다
	const int32 Count = 2 + (GetUniqueID() % 2);
	for (int32 i = 0; i < Count; ++i)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AYJJStardust* Dust = GetWorld()->SpawnActorDeferred<AYJJStardust>(AYJJStardust::StaticClass(), FTransform(GetActorLocation() + FVector(0, 0, 80)));
		if (Dust)
		{
			Dust->bCountsTowardTotal = false;
			const float A = (i / static_cast<float>(Count)) * TWO_PI + FMath::FRand();
			Dust->PopVelocity = FVector(FMath::Cos(A) * 300.f, FMath::Sin(A) * 300.f, 900.f);
			Dust->FinishSpawning(FTransform(GetActorLocation() + FVector(0, 0, 80)));
		}
	}
}

void AYJJSlime::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Time += DeltaTime;

	if (DeathTime >= 0.f)
	{
		// 말랑하게 터지는 연기
		DeathTime += DeltaTime;
		const float A = DeathTime / 0.6f;
		for (int32 i = 0; i < Puffs.Num(); ++i)
		{
			const float Ang = i * 2.399963f;
			const FVector Dir(FMath::Cos(Ang), FMath::Sin(Ang), 0.4f + YJJShapes::Hash(i) * 0.8f);
			Puffs[i]->SetRelativeLocation(FVector(0, 0, 60) + Dir * 220.f * FMath::Sqrt(A));
			Puffs[i]->SetRelativeScale3D(FVector(0.8f * (1.f - A) + 0.1f));
		}
		PuffMID->SetScalarParameterValue(TEXT("Intensity"), FMath::Max(0.f, 2.5f * (1.f - A)));
		if (A >= 1.f)
		{
			Destroy();
		}
		return;
	}

	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	AYJJHero* Hero = PC ? Cast<AYJJHero>(PC->GetPawn()) : nullptr;
	const FVector P = GetActorLocation();

	// 쫓을지, 돌아다닐지
	bChasing = false;
	if (Hero && !Hero->IsDead())
	{
		const FVector HP = Hero->GetActorLocation();
		if (FVector::Dist2D(HP, P) < SightRange && FMath::Abs(HP.Z - P.Z) < 500.f && FVector::Dist2D(HP, Home) < HomeRadius + 400.f)
		{
			bChasing = true;
			Target = HP;
		}
	}
	if (!bChasing)
	{
		WanderTimer -= DeltaTime;
		if (WanderTimer <= 0.f || FVector::Dist2D(P, Target) < 60.f)
		{
			const float A = FMath::FRand() * TWO_PI;
			const float R = FMath::Sqrt(FMath::FRand()) * HomeRadius;
			Target = Home + FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 0);
			WanderTimer = 2.f + FMath::FRand() * 3.f;
		}
	}

	// 통통 뛰는 동안에만 움직인다
	HopTime += DeltaTime * (bChasing ? 1.4f : 1.f);
	const float Hop = FMath::Fmod(HopTime, HopPeriod) / HopPeriod;
	const bool bAirborne = Hop < 0.6f;
	const float Air = bAirborne ? FMath::Sin(Hop / 0.6f * PI) : 0.f;

	FVector Move = FVector::ZeroVector;
	FVector ToTarget = Target - P;
	ToTarget.Z = 0.f;
	if (bAirborne && ToTarget.SizeSquared() > 100.f)
	{
		Move = ToTarget.GetSafeNormal() * (bChasing ? ChaseSpeed : WanderSpeed) * DeltaTime;
	}
	Move += KnockVelocity * DeltaTime;
	KnockVelocity = FMath::VInterpTo(KnockVelocity, FVector::ZeroVector, DeltaTime, 5.f);

	// 절벽으로는 안 간다
	FVector Next = P + Move;
	float NextGround = GroundZ;
	if (FindGround(Next, NextGround) && FMath::Abs(NextGround - GroundZ) < 120.f)
	{
		GroundZ = NextGround;
	}
	else
	{
		Next = P;
		KnockVelocity = FVector::ZeroVector;
		Target = Home;
	}
	SetActorLocation(FVector(Next.X, Next.Y, GroundZ));
	if (!Move.IsNearlyZero())
	{
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0, Move.Rotation().Yaw, 0), DeltaTime, 8.f));
	}

	// 말랑한 움직임: 뜰 때 길쭉, 착지할 때 납작
	const float Land = bAirborne ? 0.f : FMath::Sin((Hop - 0.6f) / 0.4f * PI);
	const float Breath = 0.03f * FMath::Sin(Time * 5.f);
	BodyPivot->SetRelativeLocation(FVector(0, 0, Air * 90.f * Size));
	BodyPivot->SetRelativeScale3D(FVector(1.f - Air * 0.12f + Land * 0.25f + Breath, 1.f - Air * 0.12f + Land * 0.25f + Breath, 1.f + Air * 0.22f - Land * 0.3f - Breath));
	Shadow->SetRelativeScale3D(FVector(1.2f * Size * (1.f - Air * 0.3f), 1.2f * Size * (1.f - Air * 0.3f), 0.02f));

	Flash = FMath::FInterpTo(Flash, 0.f, DeltaTime, 8.f);
	BodyMID->SetScalarParameterValue(TEXT("Emissive"), 0.35f + Flash * 4.f + (bChasing ? 0.15f : 0.f));

	// 영웅과 부딪히면 다치게 한다
	if (Hero && !Hero->IsDead())
	{
		const FVector BodyP = GetActorLocation() + FVector(0, 0, 55 * Size + Air * 90.f * Size);
		const FVector HeroP = Hero->GetActorLocation();
		if (FVector::Dist2D(BodyP, HeroP) < GetRadius() + 40.f && FMath::Abs(BodyP.Z - HeroP.Z) < 130.f)
		{
			Hero->TakeHit(GetActorLocation());
		}
	}
}
