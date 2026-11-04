#include "Gameplay/YJJChest.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/YJJAdventureSubsystem.h"
#include "Core/YJJAudio.h"
#include "Engine/World.h"
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
	UMaterialInterface* Wood = YJJShapes::Surface(this, FLinearColor(0.72f, 0.45f, 0.25f), FLinearColor(0.55f, 0.32f, 0.18f), 0.f, 0.7f);
	UMaterialInterface* Gold = YJJShapes::Solid(this, FLinearColor(1.f, 0.78f, 0.3f), 0.25f, 0.3f);

	// 몸통 + 금테
	YJJShapes::Add(this, R, EYJJShape::Cube, FVector(0, 0, 40), FVector(150, 100, 80), Wood, true);
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		YJJShapes::Add(this, R, EYJJShape::Cube, FVector(0, Side * 51.f, 40), FVector(156, 4, 84), Gold, false);
		YJJShapes::Add(this, R, EYJJShape::Cube, FVector(Side * 76.f, 0, 40), FVector(4, 104, 84), Gold, false);
	}

	// 뚜껑 (뒤쪽 경첩 축)
	LidPivot = NewObject<USceneComponent>(this);
	LidPivot->SetMobility(EComponentMobility::Movable);
	LidPivot->SetupAttachment(R);
	LidPivot->SetRelativeLocation(FVector(-50, 0, 80));
	LidPivot->RegisterComponent();
	AddInstanceComponent(LidPivot);
	YJJShapes::Add(this, LidPivot, EYJJShape::Cylinder, FVector(50, 0, 0), FVector(100, 150, 70), Wood, false, FRotator(0, 0, 90));
	YJJShapes::Add(this, LidPivot, EYJJShape::Cylinder, FVector(50, 0, 0), FVector(104, 20, 74), Gold, false, FRotator(0, 0, 90));
	YJJShapes::Add(this, LidPivot, EYJJShape::Cube, FVector(102, 0, -6), FVector(8, 26, 30), Gold, false);

	// 틈새로 새어 나오는 빛
	GlowMID = YJJShapes::Glow(this, FLinearColor(1.f, 0.85f, 0.45f), 1.2f, 0.f, 1.f);
	Glow = YJJShapes::Add(this, R, EYJJShape::Sphere, FVector(0, 0, 70), FVector(260, 220, 180), GlowMID, false);

	Light = NewObject<UPointLightComponent>(this);
	Light->SetupAttachment(R);
	Light->SetRelativeLocation(FVector(0, 0, 120));
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(60.f);
	Light->SetAttenuationRadius(700.f);
	Light->SetLightColor(FLinearColor(1.f, 0.8f, 0.45f));
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
	return !bOpened && FVector::Dist(From, GetActorLocation() + FVector(0, 0, 60)) < InteractRange;
}

void AYJJChest::Interact()
{
	if (bOpened)
	{
		return;
	}
	bOpened = true;
	OpenTime = 0.f;
	YJJAudio::Play(this, EYJJSound::LanternLit, FVector::ZeroVector, 0.6f, 1.3f);

	// 별가루를 위로 튕겨 낸다 (총 개수에 이미 포함)
	for (int32 i = 0; i < StardustInside; ++i)
	{
		const FTransform Spawn(GetActorLocation() + FVector(0, 0, 110));
		AYJJStardust* Dust = GetWorld()->SpawnActorDeferred<AYJJStardust>(AYJJStardust::StaticClass(), Spawn, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Dust)
		{
			Dust->bCountsTowardTotal = false;
			const float A = i * (TWO_PI / StardustInside);
			Dust->PopVelocity = FVector(FMath::Cos(A) * 260.f, FMath::Sin(A) * 260.f, 1100.f + i * 60.f);
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
		const float Wiggle = FMath::Max(0.f, FMath::Sin(Time * 2.5f)) * FMath::Max(0.f, FMath::Sin(Time * 17.f));
		LidPivot->SetRelativeRotation(FRotator(Wiggle * 6.f, 0, 0));
		GlowMID->SetScalarParameterValue(TEXT("Intensity"), 0.7f + 0.5f * FMath::Sin(Time * 3.f));
		return;
	}
	OpenTime += DeltaTime;
	const float A = FMath::Clamp(OpenTime / 0.5f, 0.f, 1.f);
	const float Ease = 1.f - FMath::Pow(1.f - A, 3.f);
	// 뚜껑을 뒤로 젖힌다 (조금 튕기며)
	const float Overshoot = FMath::Sin(A * PI) * 12.f;
	LidPivot->SetRelativeRotation(FRotator(Ease * 105.f + Overshoot, 0, 0));
	GlowMID->SetScalarParameterValue(TEXT("Intensity"), FMath::Max(0.f, 3.f * (1.f - OpenTime / 1.5f)));
	Light->SetIntensity(FMath::Max(0.f, 300.f * (1.f - OpenTime / 2.f)));
}
