#include "Gameplay/YJJChest.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/YJJAdventureSubsystem.h"
#include "Core/YJJAudio.h"
#include "Engine/World.h"
#include "Data/YJJData.h"
#include "Gameplay/YJJStardust.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "World/YJJShapes.h"

AYJJChest::AYJJChest()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);
}

void AYJJChest::BeginPlay()
{
	Super::BeginPlay();
	USceneComponent* R = RootComponent;
	UMaterialInterface* Wood = YJJShapes::Surface(this, FYJJData::Color(TEXT("ChestWood")), FYJJData::Color(TEXT("ChestWoodSide")), 0.f, WoodRoughness);
	UMaterialInterface* Gold = YJJShapes::Solid(this, FYJJData::Color(TEXT("ChestGold")), GoldEmissive, GoldRoughness);

	// 몸통 + 금테
	YJJShapes::Add(this, R, EYJJShape::Cube, FVector(0, 0, BodySize.Z * 0.5f), BodySize, Wood, true);
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		YJJShapes::Add(this, R, EYJJShape::Cube, FVector(0, Side * (BodySize.Y * 0.5f + TrimInset), BodySize.Z * 0.5f), FVector(BodySize.X + TrimOver, TrimThickness, BodySize.Z + TrimThickness), Gold, false);
		YJJShapes::Add(this, R, EYJJShape::Cube, FVector(Side * (BodySize.X * 0.5f + TrimInset), 0, BodySize.Z * 0.5f), FVector(TrimThickness, BodySize.Y + TrimThickness, BodySize.Z + TrimThickness), Gold, false);
	}

	// 뚜껑 (뒤쪽 경첩 축)
	LidPivot = NewObject<USceneComponent>(this);
	LidPivot->SetMobility(EComponentMobility::Movable);
	LidPivot->SetupAttachment(R);
	LidPivot->SetRelativeLocation(FVector(-BodySize.Y * 0.5f, 0, BodySize.Z));
	LidPivot->RegisterComponent();
	AddInstanceComponent(LidPivot);
	const FRotator LidTurn(0, 0, LidRoll);
	YJJShapes::Add(this, LidPivot, EYJJShape::Cylinder, FVector(BodySize.Y * 0.5f, 0, 0), FVector(BodySize.Y, BodySize.X, LidHeight), Wood, false, LidTurn);
	YJJShapes::Add(this, LidPivot, EYJJShape::Cylinder, FVector(BodySize.Y * 0.5f, 0, 0), FVector(BodySize.Y + TrimThickness, LidBandWidth, LidHeight + TrimThickness), Gold, false, LidTurn);
	YJJShapes::Add(this, LidPivot, EYJJShape::Cube, LockOffset, LockSize, Gold, false);

	// 틈새로 새어 나오는 빛
	GlowMID = YJJShapes::Glow(this, FYJJData::Color(TEXT("ChestGlow")), GlowIntensity, 0.f, 1.f);
	Glow = YJJShapes::Add(this, R, EYJJShape::Sphere, FVector(0, 0, GlowHeight), GlowSize, GlowMID, false);

	Light = NewObject<UPointLightComponent>(this);
	Light->SetupAttachment(R);
	Light->SetRelativeLocation(FVector(0, 0, LightHeight));
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(LightIdle);
	Light->SetAttenuationRadius(LightRadius);
	Light->SetLightColor(FYJJData::Color(TEXT("ChestLight")));
	Light->SetCastShadows(false);
	Light->RegisterComponent();
	AddInstanceComponent(Light);

	if (UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(this))
	{
		Adv->RegisterChest(StardustInside);
	}
}

bool AYJJChest::CanInteract(const FVector& From) const
{
	return !bOpened && FVector::Dist(From, GetActorLocation() + FVector(0, 0, InteractHeight)) < InteractRange;
}

void AYJJChest::Interact()
{
	if (bOpened)
	{
		return;
	}
	bOpened = true;
	OpenTime = 0.f;
	YJJAudio::Play(this, EYJJSound::LanternLit, FVector::ZeroVector, FYJJData::Tune(TEXT("Chest.SoundVolume")), FYJJData::Tune(TEXT("Chest.SoundPitch")));

	// 별가루를 위로 튕겨 낸다 (총 개수에 이미 포함)
	for (int32 i = 0; i < StardustInside; ++i)
	{
		const FTransform Spawn(GetActorLocation() + FVector(0, 0, PopHeight));
		AYJJStardust* Dust = GetWorld()->SpawnActorDeferred<AYJJStardust>(AYJJStardust::StaticClass(), Spawn, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Dust)
		{
			Dust->bCountsTowardTotal = false;
			const float A = i * (TWO_PI / StardustInside);
			Dust->PopVelocity = FVector(FMath::Cos(A) * FYJJData::Tune(TEXT("Chest.PopSide")), FMath::Sin(A) * FYJJData::Tune(TEXT("Chest.PopSide")), FYJJData::Tune(TEXT("Chest.PopUp")) + i * FYJJData::Tune(TEXT("Chest.PopUpStep")));
			Dust->FinishSpawning(Spawn);
		}
	}

	if (UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(this))
	{
		Adv->OnChestOpened();
	}
}

void AYJJChest::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Time += DeltaTime;
	if (!bOpened)
	{
		// 닫힌 상자는 뚜껑이 들썩이며 반짝인다
		const float Wiggle = FMath::Max(0.f, FMath::Sin(Time * WiggleBeat)) * FMath::Max(0.f, FMath::Sin(Time * WiggleShake));
		LidPivot->SetRelativeRotation(FRotator(Wiggle * WiggleAngle, 0, 0));
		GlowMID->SetScalarParameterValue(TEXT("Intensity"), GlowIdleBase + GlowIdleRange * FMath::Sin(Time * GlowIdleSpeed));
		return;
	}
	OpenTime += DeltaTime;
	const float A = FMath::Clamp(OpenTime / OpenDuration, 0.f, 1.f);
	const float Ease = 1.f - FMath::Pow(1.f - A, 3.f);
	// 뚜껑을 뒤로 젖힌다 (조금 튕기며)
	const float Overshoot = FMath::Sin(A * PI) * OpenOvershoot;
	LidPivot->SetRelativeRotation(FRotator(Ease * OpenAngle + Overshoot, 0, 0));
	GlowMID->SetScalarParameterValue(TEXT("Intensity"), FMath::Max(0.f, GlowBurst * (1.f - OpenTime / GlowBurstTime)));
	Light->SetIntensity(FMath::Max(0.f, LightBurst * (1.f - OpenTime / LightBurstTime)));
}
