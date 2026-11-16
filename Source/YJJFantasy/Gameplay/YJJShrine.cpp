#include "Gameplay/YJJShrine.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/YJJAdventureSubsystem.h"
#include "Core/YJJAudio.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Gameplay/YJJLantern.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "World/YJJEnvironment.h"
#include "World/YJJShapes.h"

namespace
{
	namespace ShrineLook
	{
		const FLinearColor Marble(0.82f, 0.79f, 0.74f);
		const FLinearColor MarbleSide(0.66f, 0.66f, 0.7f);
		const FLinearColor Gold(0.85f, 0.62f, 0.22f);
		const FLinearColor CrystalColor(0.35f, 0.85f, 1.f);
		constexpr int32 RuneCount = 5;
		constexpr float RuneRadius = 380.f;
		constexpr float TopZ = 120.f;
	}
}

AYJJShrine::AYJJShrine()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);
}

void AYJJShrine::BeginPlay()
{
	Super::BeginPlay();
	USceneComponent* R = RootComponent;

	UMaterialInterface* MarbleMat = YJJShapes::Surface(this, ShrineLook::Marble, ShrineLook::MarbleSide);
	UMaterialInterface* GoldMat = YJJShapes::Solid(this, ShrineLook::Gold, 0.f, 0.35f);

	// 3단 원형 제단 (한 단 40cm: 걸어서 올라갈 수 있다)
	YJJShapes::Add(this, R, EYJJShape::Cylinder, FVector(0, 0, 20), FVector(1900, 1900, 40), MarbleMat, true);
	YJJShapes::Add(this, R, EYJJShape::Cylinder, FVector(0, 0, 60), FVector(1500, 1500, 40), MarbleMat, true);
	YJJShapes::Add(this, R, EYJJShape::Cylinder, FVector(0, 0, 100), FVector(1100, 1100, 40), MarbleMat, true);
	YJJShapes::Add(this, R, EYJJShape::Cylinder, FVector(0, 0, ShrineLook::TopZ + 2.f), FVector(1060, 1060, 4), GoldMat, false);

	// 둘레 기둥 8개 (두 개는 부서짐) + 위를 잇는 들보
	for (int32 i = 0; i < 8; ++i)
	{
		const float A = i * (TWO_PI / 8.f);
		const FVector P(FMath::Cos(A) * 820.f, FMath::Sin(A) * 820.f, 0.f);
		const bool bBroken = (i == 2 || i == 5);
		const float H = bBroken ? 260.f + i * 20.f : 720.f;
		YJJShapes::Add(this, R, EYJJShape::Cylinder, P + FVector(0, 0, 40.f + H * 0.5f), FVector(120, 120, H), MarbleMat, true);
		YJJShapes::Add(this, R, EYJJShape::Cube, P + FVector(0, 0, 60.f), FVector(170, 170, 40), MarbleMat, true);
		if (!bBroken)
		{
			YJJShapes::Add(this, R, EYJJShape::Cube, P + FVector(0, 0, 40.f + H + 25.f), FVector(170, 170, 50), MarbleMat, true, FRotator(0, FMath::RadiansToDegrees(A), 0));
			YJJShapes::Add(this, R, EYJJShape::Cube, P + FVector(0, 0, 40.f + H + 60.f), FVector(150, 150, 14), GoldMat, false, FRotator(0, FMath::RadiansToDegrees(A), 0));
		}
		else
		{
			// 부서진 조각이 바닥에 굴러 있다
			const FVector Debris = P * 1.12f + FVector(FMath::Sin(A) * 120.f, -FMath::Cos(A) * 120.f, 70.f);
			YJJShapes::Add(this, R, EYJJShape::Cylinder, Debris, FVector(110, 110, 220), MarbleMat, true, FRotator(85, FMath::RadiansToDegrees(A) + 30.f, 0));
		}
	}
	// 들보 (부서지지 않은 이웃 기둥끼리)
	for (int32 i = 0; i < 8; ++i)
	{
		const int32 j = (i + 1) % 8;
		if (i == 2 || i == 5 || j == 2 || j == 5)
		{
			continue;
		}
		const float A0 = i * (TWO_PI / 8.f);
		const float A1 = j * (TWO_PI / 8.f);
		const FVector P0(FMath::Cos(A0) * 820.f, FMath::Sin(A0) * 820.f, 0.f);
		const FVector P1(FMath::Cos(A1) * 820.f, FMath::Sin(A1) * 820.f, 0.f);
		const FVector Mid = (P0 + P1) * 0.5f + FVector(0, 0, 40.f + 720.f + 70.f);
		const float Len = FVector::Dist(P0, P1);
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(P1.Y - P0.Y, P1.X - P0.X));
		YJJShapes::Add(this, R, EYJJShape::Cube, Mid, FVector(Len + 60.f, 90, 50), MarbleMat, true, FRotator(0, Yaw, 0));
	}

	// 등불 수만큼 룬 석판. 등불이 켜지면 그 색으로 빛난다
	for (int32 i = 0; i < ShrineLook::RuneCount; ++i)
	{
		const float A = i * (TWO_PI / ShrineLook::RuneCount) + HALF_PI;
		UMaterialInstanceDynamic* MID = YJJShapes::Solid(this, FLinearColor(0.3f, 0.32f, 0.38f), 0.f, 0.4f);
		RuneMIDs.Add(MID);
		YJJShapes::Add(this, R, EYJJShape::Cube, FVector(FMath::Cos(A) * ShrineLook::RuneRadius, FMath::Sin(A) * ShrineLook::RuneRadius, ShrineLook::TopZ + 8.f), FVector(150, 90, 16), MID, false,
			FRotator(0, FMath::RadiansToDegrees(A), 0));
	}

	// 가운데 받침대
	YJJShapes::Add(this, R, EYJJShape::Cylinder, FVector(0, 0, ShrineLook::TopZ + 60.f), FVector(300, 300, 120), MarbleMat, true);
	YJJShapes::Add(this, R, EYJJShape::Cylinder, FVector(0, 0, ShrineLook::TopZ + 125.f), FVector(340, 340, 14), GoldMat, false);

	// 떠 있는 수정 (원뿔 두 개 = 팔면체)
	CrystalPivot = NewObject<USceneComponent>(this);
	CrystalPivot->SetMobility(EComponentMobility::Movable);
	CrystalPivot->SetupAttachment(R);
	CrystalPivot->SetRelativeLocation(FVector(0, 0, CrystalHeight));
	CrystalPivot->RegisterComponent();
	AddInstanceComponent(CrystalPivot);

	CrystalMID = YJJShapes::Solid(this, FLinearColor(0.42f, 0.5f, 0.6f), 0.15f, 0.2f);
	YJJShapes::Add(this, CrystalPivot, EYJJShape::Cone, FVector(0, 0, 130), FVector(200, 200, 260), CrystalMID, false);
	YJJShapes::Add(this, CrystalPivot, EYJJShape::Cone, FVector(0, 0, -130), FVector(200, 200, 260), CrystalMID, false, FRotator(180, 0, 0));
	for (int32 i = 0; i < 5; ++i)
	{
		USceneComponent* ShardPivot = YJJShapes::Add(this, CrystalPivot, EYJJShape::Cone, FVector::ZeroVector, FVector(50, 50, 90), CrystalMID, false);
		Shards.Add(Cast<UStaticMeshComponent>(ShardPivot));
	}

	AuraMID = YJJShapes::Glow(this, ShrineLook::CrystalColor, 0.2f, 0.05f, 1.f);
	Aura = YJJShapes::Add(this, CrystalPivot, EYJJShape::Sphere, FVector::ZeroVector, FVector(520, 520, 620), AuraMID, false);

	BeamMID = YJJShapes::Glow(this, FLinearColor(0.7f, 0.95f, 1.f), 0.f, 0.3f, 1.5f);
	Beam = YJJShapes::Add(this, R, EYJJShape::Cylinder, FVector(0, 0, 20000.f), FVector(360, 360, 40000), BeamMID, false);
	Beam->SetVisibility(false);

	ShockMID = YJJShapes::Glow(this, FLinearColor(0.8f, 0.95f, 1.f), 0.f, 0.f, 2.f);
	Shockwave = YJJShapes::Add(this, R, EYJJShape::Sphere, FVector(0, 0, CrystalHeight), FVector(100, 100, 30), ShockMID, false);
	Shockwave->SetVisibility(false);

	SparkMID = YJJShapes::Glow(this, FLinearColor(1.f, 0.85f, 0.5f), 6.f, 1.f, 1.f);
	for (int32 i = 0; i < 28; ++i)
	{
		UStaticMeshComponent* Spark = YJJShapes::Add(this, R, EYJJShape::Sphere, FVector(0, 0, CrystalHeight), FVector(40, 40, 40), SparkMID, false);
		Spark->SetVisibility(false);
		Sparks.Add(Spark);
	}

	Light = NewObject<UPointLightComponent>(this);
	Light->SetupAttachment(CrystalPivot);
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(80.f);
	Light->SetAttenuationRadius(3500.f);
	Light->SetLightColor(ShrineLook::CrystalColor);
	Light->SetCastShadows(false);
	Light->RegisterComponent();
	AddInstanceComponent(Light);

	if (UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(this))
	{
		Adv->RegisterShrine(this);
	}
}

void AYJJShrine::SetAwake(bool bInAwake)
{
	if (bAwake == bInAwake)
	{
		return;
	}
	bAwake = bInAwake;
	if (bAwake)
	{
		YJJAudio::Play(this, EYJJSound::ShrineAwake);
	}
}

bool AYJJShrine::CanInteract(const FVector& From) const
{
	return bAwake && !bFinale && FVector::Dist2D(From, GetActorLocation()) < InteractRange && From.Z > GetActorLocation().Z;
}

void AYJJShrine::Interact()
{
	if (!bAwake || bFinale)
	{
		return;
	}
	bFinale = true;
	FinaleTime = 0.f;
	if (UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(this))
	{
		Adv->StartFinale();
	}
	YJJAudio::Play(this, EYJJSound::Finale);

	// 영웅이 서 있는 쪽에서 성소를 비스듬히 올려다보는 카메라로 넘어간다
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (const APawn* Hero = PC->GetPawn())
		{
			const FVector ToHero = Hero->GetActorLocation() - GetActorLocation();
			CameraAngle = FMath::Atan2(ToHero.Y, ToHero.X);
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		FinaleCamera = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform::Identity, Params);
		if (FinaleCamera)
		{
			FinaleCamera->GetCameraComponent()->SetFieldOfView(72.f);
			FinaleCamera->GetCameraComponent()->bConstrainAspectRatio = false;
			Tick(0.f);
			PC->SetViewTargetWithBlend(FinaleCamera, 2.f, VTBlend_EaseInOut, 2.f);
		}
	}
}

FVector AYJJShrine::GetCrystalLocation() const
{
	return CrystalPivot ? CrystalPivot->GetComponentLocation() : GetActorLocation();
}

void AYJJShrine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Time += DeltaTime;

	// 룬 석판: 켜진 등불 색으로
	if (const UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(this))
	{
		const TArray<TWeakObjectPtr<AYJJLantern>>& Lanterns = Adv->GetLanterns();
		for (int32 i = 0; i < RuneMIDs.Num(); ++i)
		{
			const AYJJLantern* Lantern = Lanterns.IsValidIndex(i) ? Lanterns[i].Get() : nullptr;
			const float A = Lantern ? Lantern->GetLightAlpha() : 0.f;
			const FLinearColor C = Lantern ? Lantern->GetColor() : FLinearColor::White;
			RuneMIDs[i]->SetVectorParameterValue(TEXT("TopColor"), FMath::Lerp(FLinearColor(0.3f, 0.32f, 0.38f), C, A));
			RuneMIDs[i]->SetVectorParameterValue(TEXT("SideColor"), FMath::Lerp(FLinearColor(0.3f, 0.32f, 0.38f), C, A));
			RuneMIDs[i]->SetScalarParameterValue(TEXT("Emissive"), A * 3.f);
		}
	}

	// 수정: 평소엔 천천히 돌며 둥실, 깨어나면 밝게 맥동
	float Spin = bAwake ? 40.f : 12.f;
	float Height = CrystalHeight + FMath::Sin(Time * 1.2f) * 30.f;
	float Glow = bAwake ? 2.5f + 1.5f * FMath::Sin(Time * 3.f) : 0.15f;

	if (bFinale)
	{
		FinaleTime += DeltaTime;
		const float Rise = FMath::SmoothStep(0.f, 2.2f, FinaleTime);
		Height = CrystalHeight + Rise * 700.f;
		Spin = 40.f + Rise * 400.f;
		Glow = 4.f + Rise * 10.f;

		if (FinaleTime > 2.2f && !Beam->IsVisible())
		{
			Beam->SetVisibility(true);
			Shockwave->SetVisibility(true);
			for (UStaticMeshComponent* Spark : Sparks)
			{
				Spark->SetVisibility(true);
			}
		}
		if (Beam->IsVisible())
		{
			const float T = FinaleTime - 2.2f;
			// 처음엔 굵고 눈부시게, 곧 수정이 비쳐 보일 만큼 가늘고 옅게
			BeamMID->SetScalarParameterValue(TEXT("Intensity"), 0.45f + 5.f * FMath::Exp(-T * 1.1f));
			const float W = 1.f + 2.5f * FMath::Exp(-T * 1.2f);
			Beam->SetRelativeScale3D(FVector(2.2f * W, 2.2f * W, 400.f));

			const float S = T / 2.5f;
			Shockwave->SetRelativeLocation(FVector(0, 0, Height));
			Shockwave->SetRelativeScale3D(FVector(1.f + S * 80.f, 1.f + S * 80.f, 0.3f + S * 4.f));
			ShockMID->SetScalarParameterValue(TEXT("Intensity"), FMath::Max(0.f, 3.f * (1.f - S)));
			if (S >= 1.f)
			{
				Shockwave->SetVisibility(false);
			}

			// 불꽃: 수정에서 사방으로 솟아올라 천천히 떨어진다 (반복)
			for (int32 i = 0; i < Sparks.Num(); ++i)
			{
				const float Seed = YJJShapes::Hash(i * 31 + 7);
				const float Period = 2.6f + Seed * 1.4f;
				const float Local = FMath::Fmod(T + Seed * Period, Period);
				const float A = i * 2.399963f; // 황금각으로 고르게
				const float Speed = 900.f + Seed * 700.f;
				const FVector Dir(FMath::Cos(A), FMath::Sin(A), 0.f);
				const FVector P = FVector(0, 0, Height) + Dir * Speed * Local + FVector(0, 0, 1800.f * Local - 700.f * Local * Local);
				Sparks[i]->SetRelativeLocation(P);
				Sparks[i]->SetRelativeScale3D(FVector(0.5f * (1.f - Local / Period) + 0.1f));
			}
		}

		// 환경: 노을빛으로
		for (TActorIterator<AYJJEnvironment> It(GetWorld()); It; ++It)
		{
			It->SetFinaleAlpha(FMath::SmoothStep(1.f, 8.f, FinaleTime));
		}
	}

	CrystalPivot->SetRelativeLocation(FVector(0, 0, Height));

	// 엔딩 카메라: 천천히 돌며 솟아오르는 수정을 올려다본다. 엔딩이 끝나면 영웅에게 돌아간다
	if (FinaleCamera)
	{
		CameraAngle += DeltaTime * FMath::DegreesToRadians(7.f);
		const float Dist = 2900.f - FMath::Min(FinaleTime, 10.f) * 60.f;
		const FVector Eye = GetActorLocation() + FVector(FMath::Cos(CameraAngle) * Dist, FMath::Sin(CameraAngle) * Dist, 500.f + FMath::Min(FinaleTime, 10.f) * 40.f);
		const FVector Focus = GetActorLocation() + FVector(0, 0, Height + 250.f);
		FinaleCamera->SetActorLocationAndRotation(Eye, (Focus - Eye).Rotation());

		const UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(this);
		if (!bCameraReturned && Adv && Adv->GetPhase() == EYJJPhase::Ended && Adv->GetPhaseTime() > 9.f)
		{
			bCameraReturned = true;
			if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
			{
				PC->SetViewTargetWithBlend(PC->GetPawn(), 2.f, VTBlend_EaseInOut, 2.f);
			}
		}
	}
	CrystalPivot->AddLocalRotation(FRotator(0, Spin * DeltaTime, 0));
	const FLinearColor Body = bAwake ? ShrineLook::CrystalColor : FLinearColor(0.42f, 0.5f, 0.6f);
	CrystalMID->SetVectorParameterValue(TEXT("TopColor"), Body);
	CrystalMID->SetVectorParameterValue(TEXT("SideColor"), Body);
	CrystalMID->SetScalarParameterValue(TEXT("Emissive"), Glow);
	AuraMID->SetScalarParameterValue(TEXT("Intensity"), bAwake ? 0.8f + 0.4f * FMath::Sin(Time * 3.f) : 0.15f);
	Light->SetIntensity(bAwake ? 600.f + Glow * 80.f : 60.f);

	for (int32 i = 0; i < Shards.Num(); ++i)
	{
		const float A = Time * (bAwake ? 1.4f : 0.5f) + i * (TWO_PI / Shards.Num());
		const float Radius = bFinale ? 340.f + FMath::Min(FinaleTime, 3.f) * 200.f : 340.f;
		Shards[i]->SetRelativeLocation(FVector(FMath::Cos(A) * Radius, FMath::Sin(A) * Radius, FMath::Sin(Time * 2.f + i) * 60.f));
		Shards[i]->SetRelativeRotation(FRotator(Time * 50.f + i * 40.f, Time * 30.f, 0));
	}
}
