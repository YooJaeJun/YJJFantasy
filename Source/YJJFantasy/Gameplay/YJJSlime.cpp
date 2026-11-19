#include "Gameplay/YJJSlime.h"

#include "Data/YJJData.h"
#include "YJJFantasy.h"
#include "Components/StaticMeshComponent.h"
#include "Core/YJJAudio.h"
#include "Engine/World.h"
#include "Gameplay/YJJStardust.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Player/YJJHero.h"
#include "World/YJJShapes.h"

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
	HopTime = YJJShapes::Hash(GetUniqueID()) * FYJJData::Tune(TEXT("Mob.HopPeriod"));

	BodyPivot = NewObject<USceneComponent>(this);
	BodyPivot->SetMobility(EComponentMobility::Movable);
	BodyPivot->SetupAttachment(RootComponent);
	BodyPivot->RegisterComponent();
	AddInstanceComponent(BodyPivot);

	const float S = Size;
	BodyMID = YJJShapes::Surface(this, YJJShapes::Shade(Color, TopShade), Color, BodyEmissive, BodyRoughness);
	// 떡무지 (Blender: Tools/Character/make_mob.py). 몸은 섬마다 색을 입히고, 얼굴의 눈꺼풀도 몸 색으로
	UStaticMesh* MobBody = YJJShapes::CharacterMesh(TEXT("SM_Mob_Body"));
	UStaticMesh* MobFace = YJJShapes::CharacterMesh(TEXT("SM_Mob_Face"));
	if (MobBody && MobFace)
	{
		const FRotator Front(0.f, MeshYaw, 0.f);
		Body = YJJShapes::AddAI(this, BodyPivot, MobBody, FVector::ZeroVector, MeshWidth * S, true, Front);
		Body->SetMaterial(0, BodyMID);
		const float FaceScale = Body->GetRelativeScale3D().X;
		UStaticMeshComponent* Face = NewObject<UStaticMeshComponent>(this);
		Face->SetStaticMesh(MobFace);
		Face->SetupAttachment(BodyPivot);
		Face->SetRelativeScale3D(FVector(FaceScale));
		Face->SetRelativeRotation(Front);
		// 얼굴은 몸과 같은 원점으로 내보냈으니 몸 컴포넌트와 같은 위치에 둔다
		Face->SetRelativeLocation(Body->GetRelativeLocation());
		Face->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Face->SetCastShadow(false);
		Face->RegisterComponent();
		AddInstanceComponent(Face);
		const TArray<FName> Slots = Face->GetMaterialSlotNames();
		for (int32 i = 0; i < Slots.Num(); ++i)
		{
			if (Slots[i].ToString().Contains(TEXT("MobBody")))
			{
				Face->SetMaterial(i, BodyMID);
			}
		}
	}
	else
	{
		// 모델이 아직 임포트되지 않았으면 둥근 덩어리로 대신한다
		UE_LOG(LogYJJFantasy, Warning, TEXT("[Mob] 떡무지 모델 없음: BuildContent.ps1로 임포트 필요"));
		Body = YJJShapes::Add(this, BodyPivot, EYJJShape::Sphere, FVector(0, 0, FallbackSize.Z * 0.5f * S), FallbackSize * S, BodyMID, false);
	}
	// 바닥 그림자 (점프할 때 위치를 알기 쉽게)
	Shadow = YJJShapes::Add(this, RootComponent, EYJJShape::Cylinder, FVector(0, 0, ShadowLift), FVector(ShadowSize * S, ShadowSize * S, ShadowLift), YJJShapes::Solid(this, FYJJData::Color(TEXT("MobShadow")), 0.f), false);
	BuildPuffs();
}

void AYJJSlime::BuildPuffs()
{
	PuffMID = YJJShapes::Glow(this, YJJShapes::Shade(Color, PuffShade), 0.f, PuffCore, 1.f);
	for (int32 i = 0; i < PuffCount; ++i)
	{
		UStaticMeshComponent* Puff = YJJShapes::Add(this, RootComponent, EYJJShape::Sphere, FVector::ZeroVector, FVector(PuffSize), PuffMID, false);
		Puff->SetVisibility(false);
		Puffs.Add(Puff);
	}
}

bool AYJJSlime::FindGround(const FVector& At, float& OutZ) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(TEXT("SlimeGround"), false, this);
	if (GetWorld()->LineTraceSingleByObjectType(Hit, At + FVector(0, 0, GroundProbeUp), At - FVector(0, 0, GroundProbeDown), FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		if (Hit.ImpactNormal.Z > WalkableNormalZ)
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
	KnockVelocity = Away.GetSafeNormal() * FYJJData::Tune(TEXT("Mob.Knockback"));
	YJJAudio::Play(this, EYJJSound::SlimeHit, GetActorLocation(), FYJJData::Tune(TEXT("Mob.HitVolume")), FMath::FRandRange(HitPitchMin, HitPitchMax));
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

	// 별가루 몇 개를 튕겨 낸다
	const int32 MinDrops = FMath::RoundToInt(FYJJData::Tune(TEXT("Mob.DropsMin")));
	const int32 Count = MinDrops + (GetUniqueID() % (FMath::RoundToInt(FYJJData::Tune(TEXT("Mob.DropsMax"))) - MinDrops + 1));
	const FTransform Spawn(GetActorLocation() + FVector(0, 0, FYJJData::Tune(TEXT("Mob.DropHeight"))));
	for (int32 i = 0; i < Count; ++i)
	{
		AYJJStardust* Dust = GetWorld()->SpawnActorDeferred<AYJJStardust>(AYJJStardust::StaticClass(), Spawn);
		if (Dust)
		{
			Dust->bCountsTowardTotal = false;
			const float A = (i / static_cast<float>(Count)) * TWO_PI + FMath::FRand();
			const float Side = FYJJData::Tune(TEXT("Mob.DropSide"));
			Dust->PopVelocity = FVector(FMath::Cos(A) * Side, FMath::Sin(A) * Side, FYJJData::Tune(TEXT("Mob.DropUp")));
			Dust->FinishSpawning(Spawn);
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
		const float A = DeathTime / PuffTime;
		for (int32 i = 0; i < Puffs.Num(); ++i)
		{
			const float Ang = i * GoldenAngle;
			const FVector Dir(FMath::Cos(Ang), FMath::Sin(Ang), PuffRiseMin + YJJShapes::Hash(i) * PuffRiseRange);
			Puffs[i]->SetRelativeLocation(FVector(0, 0, PuffCenterHeight) + Dir * PuffSpread * FMath::Sqrt(A));
			Puffs[i]->SetRelativeScale3D(FVector(PuffStartScale * (1.f - A) + PuffEndScale));
		}
		PuffMID->SetScalarParameterValue(TEXT("Intensity"), FMath::Max(0.f, PuffGlow * (1.f - A)));
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
		if (FVector::Dist2D(HP, P) < FYJJData::Tune(TEXT("Mob.SightRange")) && FMath::Abs(HP.Z - P.Z) < FYJJData::Tune(TEXT("Mob.SightHeight"))
			&& FVector::Dist2D(HP, Home) < HomeRadius + FYJJData::Tune(TEXT("Mob.Leash")))
		{
			bChasing = true;
			Target = HP;
		}
	}
	if (!bChasing)
	{
		WanderTimer -= DeltaTime;
		if (WanderTimer <= 0.f || FVector::Dist2D(P, Target) < FYJJData::Tune(TEXT("Mob.WanderArrive")))
		{
			const float A = FMath::FRand() * TWO_PI;
			const float R = FMath::Sqrt(FMath::FRand()) * HomeRadius;
			Target = Home + FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 0);
			WanderTimer = FYJJData::Tune(TEXT("Mob.WanderPauseMin")) + FMath::FRand() * FYJJData::Tune(TEXT("Mob.WanderPauseRange"));
		}
	}

	// 통통 뛰는 동안에만 움직인다
	const float HopPeriod = FYJJData::Tune(TEXT("Mob.HopPeriod"));
	HopTime += DeltaTime * (bChasing ? FYJJData::Tune(TEXT("Mob.ChaseHopRate")) : 1.f);
	const float Hop = FMath::Fmod(HopTime, HopPeriod) / HopPeriod;
	const bool bAirborne = Hop < AirFraction;
	const float Air = bAirborne ? FMath::Sin(Hop / AirFraction * PI) : 0.f;

	FVector Move = FVector::ZeroVector;
	FVector ToTarget = Target - P;
	ToTarget.Z = 0.f;
	if (bAirborne && ToTarget.SizeSquared() > FMath::Square(ArriveDistance))
	{
		Move = ToTarget.GetSafeNormal() * (bChasing ? FYJJData::Tune(TEXT("Mob.ChaseSpeed")) : FYJJData::Tune(TEXT("Mob.WanderSpeed"))) * DeltaTime;
	}
	Move += KnockVelocity * DeltaTime;
	KnockVelocity = FMath::VInterpTo(KnockVelocity, FVector::ZeroVector, DeltaTime, FYJJData::Tune(TEXT("Mob.KnockbackDecay")));

	// 절벽으로는 안 간다
	FVector Next = P + Move;
	float NextGround = GroundZ;
	if (FindGround(Next, NextGround) && FMath::Abs(NextGround - GroundZ) < FYJJData::Tune(TEXT("Mob.MaxStep")))
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
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0, Move.Rotation().Yaw, 0), DeltaTime, TurnSpeed));
	}

	// 말랑한 움직임: 뜰 때 길쭉, 착지할 때 납작
	const float Land = bAirborne ? 0.f : FMath::Sin((Hop - AirFraction) / (1.f - AirFraction) * PI);
	const float Breath = BreathAmount * FMath::Sin(Time * BreathSpeed);
	const float Wide = 1.f - Air * AirThin + Land * LandWiden + Breath;
	BodyPivot->SetRelativeLocation(FVector(0, 0, Air * HopHeight * Size));
	BodyPivot->SetRelativeScale3D(FVector(Wide, Wide, 1.f + Air * AirStretch - Land * LandSquash - Breath));
	const float ShadowScale = ShadowSize / YJJShapes::BaseShapeSize * Size * (1.f - Air * ShadowShrink);
	Shadow->SetRelativeScale3D(FVector(ShadowScale, ShadowScale, ShadowFlat));

	Flash = FMath::FInterpTo(Flash, 0.f, DeltaTime, FlashDecay);
	BodyMID->SetScalarParameterValue(TEXT("Emissive"), BodyEmissive + Flash * FlashGlow + (bChasing ? ChaseGlow : 0.f));

	// 영웅과 부딪히면 다치게 한다
	if (Hero && !Hero->IsDead())
	{
		const FVector BodyP = GetActorLocation() + FVector(0, 0, (BodyCenterHeight + Air * HopHeight) * Size);
		const FVector HeroP = Hero->GetActorLocation();
		if (FVector::Dist2D(BodyP, HeroP) < GetRadius() + FYJJData::Tune(TEXT("Mob.ContactReach")) && FMath::Abs(BodyP.Z - HeroP.Z) < FYJJData::Tune(TEXT("Mob.ContactHeight")))
		{
			Hero->TakeHit(GetActorLocation());
		}
	}
}
