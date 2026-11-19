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
	Phase = YJJShapes::Hash(GetUniqueID()) * 10.f;

	const FLinearColor Gold(1.f, 0.82f, 0.4f);
	Core = YJJShapes::Add(this, RootComponent, EYJJShape::Sphere, FVector::ZeroVector, FVector(26, 26, 26), YJJShapes::Glow(this, FLinearColor(1.f, 0.95f, 0.8f), 5.f, 1.f, 0.5f), false);
	HaloMID = YJJShapes::Glow(this, Gold, 1.2f, 0.f, 1.f);
	Halo = YJJShapes::Add(this, RootComponent, EYJJShape::Sphere, FVector::ZeroVector, FVector(70, 70, 70), HaloMID, false);
	// 반짝이는 십자 별
	UMaterialInterface* StarMat = YJJShapes::Glow(this, Gold, 3.f, 1.f, 0.f);
	Star = YJJShapes::Add(this, RootComponent, EYJJShape::Cube, FVector::ZeroVector, FVector(6, 6, 90), StarMat, false);
	StarB = YJJShapes::Add(this, RootComponent, EYJJShape::Cube, FVector::ZeroVector, FVector(90, 6, 6), StarMat, false);

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
		const float A = CollectTime / 0.25f;
		SetActorScale3D(FVector(1.f + A * 1.5f));
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
		PopVelocity.Z -= 2400.f * DeltaTime;
		Home += PopVelocity * DeltaTime;
		if (PopVelocity.Z < 0.f)
		{
			FHitResult Hit;
			if (GetWorld()->LineTraceSingleByObjectType(Hit, Home + FVector(0, 0, 50), Home - FVector(0, 0, 80), FCollisionObjectQueryParams(ECC_WorldStatic)) && Home.Z < Hit.ImpactPoint.Z + 70.f)
			{
				Home.Z = Hit.ImpactPoint.Z + 70.f;
				PopVelocity = FVector::ZeroVector;
			}
			else if (Home.Z < -100000.f)
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
		const FVector Target = Pawn->GetActorLocation() + FVector(0, 0, 20);
		const float Dist = FVector::Dist(GetActorLocation(), Target);
		if (!bMagnet && Dist < FYJJData::Tune(TEXT("Stardust.MagnetRange")) && PopVelocity.IsNearlyZero())
		{
			bMagnet = true;
		}
		if (bMagnet)
		{
			// 가속하며 빨려 들어간다
			const FVector Dir = (Target - GetActorLocation()).GetSafeNormal();
			MagnetVelocity = FMath::VInterpTo(MagnetVelocity, Dir * 2200.f, DeltaTime, 8.f);
			SetActorLocation(GetActorLocation() + MagnetVelocity * DeltaTime);
			if (FVector::Dist(GetActorLocation(), Target) < FYJJData::Tune(TEXT("Stardust.CollectRange")))
			{
				Collect();
			}
			return;
		}
	}

	const float Bob = FMath::Sin(Time * 2.4f + Phase) * 18.f;
	SetActorLocation(Home + FVector(0, 0, Bob));
	const float Twinkle = 0.7f + 0.3f * FMath::Sin(Time * 6.f + Phase * 3.f);
	const FRotator Spin(0, Time * 90.f, 0);
	Star->SetRelativeRotation(Spin);
	StarB->SetRelativeRotation(Spin);
	Star->SetRelativeScale3D(FVector(0.06f, 0.06f, 0.9f * Twinkle));
	StarB->SetRelativeScale3D(FVector(0.9f * Twinkle, 0.06f, 0.06f));
	HaloMID->SetScalarParameterValue(TEXT("Intensity"), 0.9f + 0.4f * Twinkle);
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
