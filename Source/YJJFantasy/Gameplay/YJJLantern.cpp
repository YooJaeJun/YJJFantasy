#include "Gameplay/YJJLantern.h"

#include "Data/YJJData.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/YJJAdventureSubsystem.h"
#include "Core/YJJAudio.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "World/YJJShapes.h"

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
	UMaterialInterface* StoneMat = YJJShapes::Surface(this, FYJJData::Color(TEXT("LanternStoneTop")), FYJJData::Color(TEXT("LanternStone")));
	UMaterialInterface* BronzeMat = YJJShapes::Solid(this, FYJJData::Color(TEXT("LanternBronze")), 0.f, BronzeRoughness);

	// 받침 + 기둥 + 등 갓 (아래부터 차례로: 위치 z, 지름, 높이)
	for (const FVector& Part : StoneParts)
	{
		YJJShapes::Add(this, R, EYJJShape::Cylinder, FVector(0, 0, Part.X), FVector(Part.Y, Part.Y, Part.Z), StoneMat, true);
	}
	YJJShapes::Add(this, R, EYJJShape::Cylinder, FVector(0, 0, CapBottom.X), FVector(CapBottom.Y, CapBottom.Y, CapBottom.Z), BronzeMat, false);
	for (int32 i = 0; i < CageBars; ++i)
	{
		const float A = i * (TWO_PI / CageBars) + PI / CageBars;
		YJJShapes::Add(this, R, EYJJShape::Cube, FVector(FMath::Cos(A) * CageRadius, FMath::Sin(A) * CageRadius, OrbHeight), CageBarSize, BronzeMat, false);
	}
	YJJShapes::Add(this, R, EYJJShape::Cylinder, FVector(0, 0, CapTop.X), FVector(CapTop.Y, CapTop.Y, CapTop.Z), BronzeMat, false);
	YJJShapes::Add(this, R, EYJJShape::Cone, FVector(0, 0, Roof.X), FVector(Roof.Y, Roof.Y, Roof.Z), BronzeMat, false);
	YJJShapes::Add(this, R, EYJJShape::Sphere, FVector(0, 0, FinialHeight), FVector(FinialSize), YJJShapes::Solid(this, Color, FinialEmissive), false);

	// 꺼진 상태: 희미한 푸른 빛
	OrbMID = YJJShapes::Glow(this, FYJJData::Color(TEXT("LanternUnlit")), OrbGlowUnlit, OrbCore, OrbRim);
	Orb = YJJShapes::Add(this, R, EYJJShape::Sphere, FVector(0, 0, OrbHeight), FVector(OrbSize), OrbMID, false);
	HaloMID = YJJShapes::Glow(this, Color, 0.f, 0.f, 1.f);
	Halo = YJJShapes::Add(this, R, EYJJShape::Sphere, FVector(0, 0, OrbHeight), FVector(HaloSize), HaloMID, false);

	BeamMID = YJJShapes::Glow(this, Color, 0.f, BeamCore, BeamRim);
	Beam = YJJShapes::Add(this, R, EYJJShape::Cylinder, FVector(0, 0, OrbHeight + BeamLength * 0.5f), FVector(BeamWidth, BeamWidth, BeamLength), BeamMID, false);
	Beam->SetVisibility(false);

	BurstMID = YJJShapes::Glow(this, Color, 0.f, 0.f, BurstRim);
	Burst = YJJShapes::Add(this, R, EYJJShape::Sphere, FVector(0, 0, OrbHeight), FVector(YJJShapes::BaseShapeSize), BurstMID, false);
	Burst->SetVisibility(false);

	// 켜지면 등 둘레를 도는 빛 고리
	RingMID = YJJShapes::Glow(this, Color, 0.f, 0.f, 1.f);
	for (int32 i = 0; i < RingMotes; ++i)
	{
		UStaticMeshComponent* Ring = YJJShapes::Add(this, R, EYJJShape::Sphere, FVector(0, 0, OrbHeight), FVector(RingMoteSize), RingMID, false);
		Rings.Add(Ring);
	}

	Light = NewObject<UPointLightComponent>(this);
	Light->SetupAttachment(R);
	Light->SetRelativeLocation(FVector(0, 0, OrbHeight));
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(0.f);
	Light->SetAttenuationRadius(LightRadius);
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
	return !bLit && FVector::Dist(From, GetActorLocation() + FVector(0, 0, InteractHeight)) < InteractRange;
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
	return GetActorLocation() + GetActorForwardVector() * FYJJData::Tune(TEXT("Lantern.RespawnForward")) + FVector(0, 0, FYJJData::Tune(TEXT("Lantern.RespawnUp")));
}

void AYJJLantern::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Time += DeltaTime;

	if (!bLit)
	{
		// 꺼진 등불은 숨 쉬듯 희미하게 깜빡여서 "여기 있다"를 알린다
		const float Pulse = 0.5f + 0.5f * FMath::Sin(Time * UnlitPulseSpeed);
		OrbMID->SetScalarParameterValue(TEXT("Intensity"), UnlitPulseBase + Pulse * UnlitPulseRange);
		return;
	}

	LightAlpha = FMath::Min(1.f, LightAlpha + DeltaTime / LightUpTime);
	const float Ease = 1.f - FMath::Pow(1.f - LightAlpha, 3.f);

	OrbMID->SetVectorParameterValue(TEXT("Color"), Color);
	OrbMID->SetScalarParameterValue(TEXT("Intensity"), OrbGlowLitBase + Ease * OrbGlowLitRange + FMath::Sin(Time * OrbFlickerSpeed) * OrbFlicker);
	HaloMID->SetScalarParameterValue(TEXT("Intensity"), Ease * HaloGlowLit);
	Halo->SetRelativeScale3D(FVector(HaloSize / YJJShapes::BaseShapeSize + HaloBreath * FMath::Sin(Time * HaloBreathSpeed)));
	Light->SetIntensity(Ease * LightLit);

	// 빛기둥: 아래에서 위로 솟구친 뒤 은은하게 유지
	const float BeamGrow = FMath::Clamp(Time / BeamRiseTime, 0.f, 1.f);
	const float BeamThick = BeamWidth / YJJShapes::BaseShapeSize + (1.f - BeamGrow) * BeamStartThicken;
	Beam->SetRelativeScale3D(FVector(BeamThick, BeamThick, BeamLength / YJJShapes::BaseShapeSize * BeamGrow));
	Beam->SetRelativeLocation(FVector(0, 0, OrbHeight + BeamLength * 0.5f * BeamGrow));
	const float BeamFlash = FMath::Exp(-Time * BeamFlashDecay);
	BeamMID->SetScalarParameterValue(TEXT("Intensity"), BeamGlow + BeamFlash * BeamFlashGlow);

	// 터지는 빛 구
	if (Burst->IsVisible())
	{
		const float B = Time / BurstTime;
		Burst->SetRelativeScale3D(FVector(1.f + B * BurstGrow));
		BurstMID->SetScalarParameterValue(TEXT("Intensity"), FMath::Max(0.f, BurstGlow * (1.f - B)));
		if (B >= 1.f)
		{
			Burst->SetVisibility(false);
		}
	}

	RingMID->SetScalarParameterValue(TEXT("Intensity"), Ease * RingGlow);
	for (int32 i = 0; i < Rings.Num(); ++i)
	{
		const float A = Time * RingSpinSpeed + i * (TWO_PI / Rings.Num());
		Rings[i]->SetRelativeLocation(FVector(FMath::Cos(A) * RingRadius, FMath::Sin(A) * RingRadius, OrbHeight + FMath::Sin(Time * RingBobSpeed + i) * RingBob));
	}
}
