#include "Gameplay/YJJLantern.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/YJJAdventureSubsystem.h"
#include "Core/YJJAudio.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "World/YJJShapes.h"

namespace
{
	namespace LanternLook
	{
		const FLinearColor Stone(0.62f, 0.6f, 0.58f);
		const FLinearColor StoneTop(0.78f, 0.76f, 0.72f);
		const FLinearColor Bronze(0.55f, 0.38f, 0.2f);
		constexpr float OrbHeight = 330.f;
	}
}

AYJJLantern::AYJJLantern()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);
}

void AYJJLantern::Setup(const FString& InName, const FLinearColor& InColor)
{
	LanternName = InName;
	Color = InColor;
}

void AYJJLantern::BeginPlay()
{
	Super::BeginPlay();

	USceneComponent* R = RootComponent;
	UMaterialInterface* StoneMat = YJJShapes::Surface(this, LanternLook::StoneTop, LanternLook::Stone);
	UMaterialInterface* BronzeMat = YJJShapes::Solid(this, LanternLook::Bronze, 0.f, 0.45f);

	// 받침 + 기둥 + 등 갓
	YJJShapes::Add(this, R, EYJJShape::Cylinder, FVector(0, 0, 20), FVector(180, 180, 40), StoneMat, true);
	YJJShapes::Add(this, R, EYJJShape::Cylinder, FVector(0, 0, 55), FVector(130, 130, 30), StoneMat, true);
	YJJShapes::Add(this, R, EYJJShape::Cylinder, FVector(0, 0, 170), FVector(34, 34, 210), StoneMat, true);
	YJJShapes::Add(this, R, EYJJShape::Cylinder, FVector(0, 0, 272), FVector(110, 110, 14), BronzeMat, false);
	for (int32 i = 0; i < 4; ++i)
	{
		const float A = i * HALF_PI + PI / 4.f;
		YJJShapes::Add(this, R, EYJJShape::Cube, FVector(FMath::Cos(A) * 46.f, FMath::Sin(A) * 46.f, LanternLook::OrbHeight), FVector(9, 9, 110), BronzeMat, false);
	}
	YJJShapes::Add(this, R, EYJJShape::Cylinder, FVector(0, 0, 388), FVector(120, 120, 12), BronzeMat, false);
	YJJShapes::Add(this, R, EYJJShape::Cone, FVector(0, 0, 430), FVector(100, 100, 75), BronzeMat, false);
	YJJShapes::Add(this, R, EYJJShape::Sphere, FVector(0, 0, 478), FVector(22, 22, 22), YJJShapes::Solid(this, Color, 1.5f), false);

	// 꺼진 상태: 희미한 푸른 빛
	OrbMID = YJJShapes::Glow(this, FLinearColor(0.4f, 0.5f, 0.7f), 0.6f, 0.6f, 0.6f);
	Orb = YJJShapes::Add(this, R, EYJJShape::Sphere, FVector(0, 0, LanternLook::OrbHeight), FVector(62, 62, 62), OrbMID, false);
	HaloMID = YJJShapes::Glow(this, Color, 0.f, 0.f, 1.f);
	Halo = YJJShapes::Add(this, R, EYJJShape::Sphere, FVector(0, 0, LanternLook::OrbHeight), FVector(170, 170, 170), HaloMID, false);

	BeamMID = YJJShapes::Glow(this, Color, 0.f, 0.15f, 1.2f);
	Beam = YJJShapes::Add(this, R, EYJJShape::Cylinder, FVector(0, 0, LanternLook::OrbHeight + 6000.f), FVector(70, 70, 12000), BeamMID, false);
	Beam->SetVisibility(false);

	BurstMID = YJJShapes::Glow(this, Color, 0.f, 0.f, 1.5f);
	Burst = YJJShapes::Add(this, R, EYJJShape::Sphere, FVector(0, 0, LanternLook::OrbHeight), FVector(100, 100, 100), BurstMID, false);
	Burst->SetVisibility(false);

	// 켜지면 등 둘레를 도는 빛 고리
	RingMID = YJJShapes::Glow(this, Color, 0.f, 0.f, 1.f);
	for (int32 i = 0; i < 3; ++i)
	{
		UStaticMeshComponent* Ring = YJJShapes::Add(this, R, EYJJShape::Sphere, FVector(0, 0, LanternLook::OrbHeight), FVector(14, 14, 14), RingMID, false);
		Rings.Add(Ring);
	}

	Light = NewObject<UPointLightComponent>(this);
	Light->SetupAttachment(R);
	Light->SetRelativeLocation(FVector(0, 0, LanternLook::OrbHeight));
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(0.f);
	Light->SetAttenuationRadius(1800.f);
	Light->SetLightColor(Color);
	Light->SetCastShadows(false);
	Light->RegisterComponent();
	AddInstanceComponent(Light);

	if (UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(this))
	{
		Adv->RegisterLantern(this);
	}
}

bool AYJJLantern::CanInteract(const FVector& From) const
{
	return !bLit && FVector::Dist(From, GetActorLocation() + FVector(0, 0, 100)) < InteractRange;
}

void AYJJLantern::Interact()
{
	if (bLit)
	{
		return;
	}
	bLit = true;
	Time = 0.f;
	Beam->SetVisibility(true);
	Burst->SetVisibility(true);
	YJJAudio::Play(this, EYJJSound::LanternLit, GetActorLocation());

	if (UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(this))
	{
		Adv->SetCheckpoint(GetRespawnLocation(), GetActorRotation().Yaw);
		Adv->OnLanternLit(this);
	}
}

FVector AYJJLantern::GetRespawnLocation() const
{
	return GetActorLocation() + GetActorForwardVector() * 300.f + FVector(0, 0, 120.f);
}

void AYJJLantern::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Time += DeltaTime;

	if (!bLit)
	{
		// 꺼진 등불은 숨 쉬듯 희미하게 깜빡여서 "여기 있다"를 알린다
		const float Pulse = 0.5f + 0.5f * FMath::Sin(Time * 2.2f);
		OrbMID->SetScalarParameterValue(TEXT("Intensity"), 0.4f + Pulse * 0.5f);
		return;
	}

	LightAlpha = FMath::Min(1.f, LightAlpha + DeltaTime / 1.6f);
	const float Ease = 1.f - FMath::Pow(1.f - LightAlpha, 3.f);

	OrbMID->SetVectorParameterValue(TEXT("Color"), Color);
	OrbMID->SetScalarParameterValue(TEXT("Intensity"), 2.f + Ease * 6.f + FMath::Sin(Time * 4.f) * 0.6f);
	HaloMID->SetScalarParameterValue(TEXT("Intensity"), Ease * 1.4f);
	Halo->SetRelativeScale3D(FVector(1.7f + 0.1f * FMath::Sin(Time * 2.f)));
	Light->SetIntensity(Ease * 900.f);

	// 빛기둥: 아래에서 위로 솟구친 뒤 은은하게 유지
	const float BeamGrow = FMath::Clamp(Time / 0.9f, 0.f, 1.f);
	Beam->SetRelativeScale3D(FVector(0.7f + (1.f - BeamGrow) * 1.6f, 0.7f + (1.f - BeamGrow) * 1.6f, 120.f * BeamGrow));
	Beam->SetRelativeLocation(FVector(0, 0, LanternLook::OrbHeight + 6000.f * BeamGrow));
	const float BeamFlash = FMath::Exp(-Time * 1.5f);
	BeamMID->SetScalarParameterValue(TEXT("Intensity"), 0.6f + BeamFlash * 5.f);

	// 터지는 빛 구
	if (Burst->IsVisible())
	{
		const float B = Time / 0.8f;
		Burst->SetRelativeScale3D(FVector(1.f + B * 9.f));
		BurstMID->SetScalarParameterValue(TEXT("Intensity"), FMath::Max(0.f, 3.f * (1.f - B)));
		if (B >= 1.f)
		{
			Burst->SetVisibility(false);
		}
	}

	RingMID->SetScalarParameterValue(TEXT("Intensity"), Ease * 6.f);
	for (int32 i = 0; i < Rings.Num(); ++i)
	{
		const float A = Time * 1.6f + i * (TWO_PI / Rings.Num());
		Rings[i]->SetRelativeLocation(FVector(FMath::Cos(A) * 120.f, FMath::Sin(A) * 120.f, LanternLook::OrbHeight + FMath::Sin(Time * 2.f + i) * 50.f));
	}
}
