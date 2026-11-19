#include "Gameplay/YJJStardust.h"

#include "Data/YJJData.h"
#include "Components/StaticMeshComponent.h"
#include "Core/YJJAdventureSubsystem.h"
#include "Core/YJJAudio.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "World/YJJShapes.h"

float AYJJStardust::LastCollectTime = TNumericLimits<float>::Lowest();
int32 AYJJStardust::ComboIndex = 0;

AYJJStardust::AYJJStardust()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);
}

void AYJJStardust::BeginPlay()
{
	Super::BeginPlay();
	Home = GetActorLocation();
	Phase = YJJShapes::Hash(GetUniqueID()) * PhaseSpread;

	const FLinearColor Gold = FYJJData::Color(TEXT("StardustGold"));
	Core = YJJShapes::Add(this, RootComponent, EYJJShape::Sphere, FVector::ZeroVector, FVector(CoreSize), YJJShapes::Glow(this, FYJJData::Color(TEXT("StardustCore")), CoreGlow, 1.f, CoreRim), false);
	HaloMID = YJJShapes::Glow(this, Gold, HaloGlow, 0.f, 1.f);
	Halo = YJJShapes::Add(this, RootComponent, EYJJShape::Sphere, FVector::ZeroVector, FVector(HaloSize), HaloMID, false);
	// 반짝이는 십자 별
	UMaterialInterface* StarMat = YJJShapes::Glow(this, Gold, StarGlow, 1.f, 0.f);
	Star = YJJShapes::Add(this, RootComponent, EYJJShape::Cube, FVector::ZeroVector, FVector(StarThickness, StarThickness, StarLength), StarMat, false);
	StarB = YJJShapes::Add(this, RootComponent, EYJJShape::Cube, FVector::ZeroVector, FVector(StarLength, StarThickness, StarThickness), StarMat, false);

	if (bCountsTowardTotal)
	{
		if (UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(this))
		{
			Adv->RegisterStardust();
		}
	}
}

void AYJJStardust::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Time += DeltaTime;

	if (bCollected)
	{
		// 주운 뒤: 반짝 커졌다가 사라진다
		CollectTime += DeltaTime;
		const float A = CollectTime / CollectFlashTime;
		SetActorScale3D(FVector(1.f + A * CollectFlashGrow));
		HaloMID->SetScalarParameterValue(TEXT("Fade"), FMath::Max(0.f, 1.f - A));
		if (A >= 1.f)
		{
			Destroy();
		}
		return;
	}

	// 몹이 떨군 별가루는 튀어 올랐다가 떨어져 자리 잡는다
	if (!PopVelocity.IsNearlyZero())
	{
		PopVelocity.Z -= FYJJData::Tune(TEXT("Stardust.PopGravity")) * DeltaTime;
		Home += PopVelocity * DeltaTime;
		if (PopVelocity.Z < 0.f)
		{
			const float Hover = FYJJData::Tune(TEXT("Stardust.SettleHeight"));
			FHitResult Hit;
			if (GetWorld()->LineTraceSingleByObjectType(Hit, Home + FVector(0, 0, LandProbeUp), Home - FVector(0, 0, LandProbeDown), FCollisionObjectQueryParams(ECC_WorldStatic)) && Home.Z < Hit.ImpactPoint.Z + Hover)
			{
				Home.Z = Hit.ImpactPoint.Z + Hover;
				PopVelocity = FVector::ZeroVector;
			}
			else if (Home.Z < LostBelowZ)
			{
				Destroy();
				return;
			}
		}
	}

	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (Pawn)
	{
		const FVector Target = Pawn->GetActorLocation() + FVector(0, 0, TargetLift);
		const float Dist = FVector::Dist(GetActorLocation(), Target);
		if (!bMagnet && Dist < FYJJData::Tune(TEXT("Stardust.MagnetRange")) && PopVelocity.IsNearlyZero())
		{
			bMagnet = true;
		}
		if (bMagnet)
		{
			// 가속하며 빨려 들어간다
			const FVector Dir = (Target - GetActorLocation()).GetSafeNormal();
			MagnetVelocity = FMath::VInterpTo(MagnetVelocity, Dir * FYJJData::Tune(TEXT("Stardust.MagnetSpeed")), DeltaTime, FYJJData::Tune(TEXT("Stardust.MagnetAccel")));
			SetActorLocation(GetActorLocation() + MagnetVelocity * DeltaTime);
			if (FVector::Dist(GetActorLocation(), Target) < FYJJData::Tune(TEXT("Stardust.CollectRange")))
			{
				Collect();
			}
			return;
		}
	}

	const float Bob = FMath::Sin(Time * BobSpeed + Phase) * BobHeight;
	SetActorLocation(Home + FVector(0, 0, Bob));
	const float Twinkle = TwinkleBase + TwinkleRange * FMath::Sin(Time * TwinkleSpeed + Phase * TwinklePhaseMul);
	const FRotator Spin(0, Time * SpinSpeed, 0);
	Star->SetRelativeRotation(Spin);
	StarB->SetRelativeRotation(Spin);
	const float Thin = StarThickness / YJJShapes::BaseShapeSize;
	const float Long = StarLength / YJJShapes::BaseShapeSize * Twinkle;
	Star->SetRelativeScale3D(FVector(Thin, Thin, Long));
	StarB->SetRelativeScale3D(FVector(Long, Thin, Thin));
	HaloMID->SetScalarParameterValue(TEXT("Intensity"), HaloPulseBase + HaloPulseRange * Twinkle);
}

void AYJJStardust::Collect()
{
	bCollected = true;
	const float Now = GetWorld()->GetTimeSeconds();
	ComboIndex = (Now - LastCollectTime < FYJJData::Tune(TEXT("Stardust.ComboWindow"))) ? FMath::Min(ComboIndex + 1, FMath::RoundToInt(FYJJData::Tune(TEXT("Stardust.ComboMax")))) : 0;
	LastCollectTime = Now;
	YJJAudio::Play(this, EYJJSound::Stardust, FVector::ZeroVector, FYJJData::Tune(TEXT("Stardust.Volume")), FMath::Pow(2.f, ComboIndex * FYJJData::Tune(TEXT("Stardust.ComboSemitones")) / SemitonesPerOctave));
	if (UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(this))
	{
		Adv->OnStardustCollected();
	}
}
