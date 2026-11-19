#include "Gameplay/YJJShrine.h"

#include "Data/YJJData.h"
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

	UMaterialInterface* MarbleMat = YJJShapes::Surface(this, FYJJData::Color(TEXT("ShrineMarble")), FYJJData::Color(TEXT("ShrineMarbleSide")));
	UMaterialInterface* GoldMat = YJJShapes::Solid(this, FYJJData::Color(TEXT("ShrineGold")), 0.f, GoldRoughness);

	// 3단 원형 제단 (한 단 StepHeight: 걸어서 올라갈 수 있다)
	for (int32 i = 0; i < StepCount; ++i)
	{
		const float D = StepBottomDiameter - i * StepShrink;
		YJJShapes::Add(this, R, EYJJShape::Cylinder, FVector(0, 0, StepHeight * (i + 0.5f)), FVector(D, D, StepHeight), MarbleMat, true);
	}
	YJJShapes::Add(this, R, EYJJShape::Cylinder, FVector(0, 0, TopZ + TopInlayThickness * 0.5f), FVector(TopInlayDiameter, TopInlayDiameter, TopInlayThickness), GoldMat, false);

	// 둘레 기둥 (몇 개는 부서짐) + 위를 잇는 들보
	for (int32 i = 0; i < PillarCount; ++i)
	{
		const float A = i * (TWO_PI / PillarCount);
		const FVector P(FMath::Cos(A) * PillarRing, FMath::Sin(A) * PillarRing, 0.f);
		const bool bBroken = IsBrokenPillar(i);
		const float H = bBroken ? BrokenPillarHeight + i * BrokenPillarStep : PillarHeight;
		const FRotator Facing(0, FMath::RadiansToDegrees(A), 0);
		YJJShapes::Add(this, R, EYJJShape::Cylinder, P + FVector(0, 0, PillarBase + H * 0.5f), FVector(PillarDiameter, PillarDiameter, H), MarbleMat, true);
		YJJShapes::Add(this, R, EYJJShape::Cube, P + FVector(0, 0, PlinthHeight), FVector(PlinthWidth, PlinthWidth, PlinthThickness), MarbleMat, true);
		if (!bBroken)
		{
			YJJShapes::Add(this, R, EYJJShape::Cube, P + FVector(0, 0, PillarBase + H + CapitalLift), FVector(PlinthWidth, PlinthWidth, CapitalThickness), MarbleMat, true, Facing);
			YJJShapes::Add(this, R, EYJJShape::Cube, P + FVector(0, 0, PillarBase + H + CapitalTrimLift), FVector(CapitalTrimWidth, CapitalTrimWidth, CapitalTrimThickness), GoldMat, false, Facing);
		}
		else
		{
			// 부서진 조각이 바닥에 굴러 있다
			const FVector Debris = P * DebrisOut + FVector(FMath::Sin(A) * DebrisSide, -FMath::Cos(A) * DebrisSide, DebrisHeight);
			YJJShapes::Add(this, R, EYJJShape::Cylinder, Debris, DebrisSize, MarbleMat, true, FRotator(DebrisPitch, FMath::RadiansToDegrees(A) + DebrisTurn, 0));
		}
	}
	// 들보 (부서지지 않은 이웃 기둥끼리)
	for (int32 i = 0; i < PillarCount; ++i)
	{
		const int32 j = (i + 1) % PillarCount;
		if (IsBrokenPillar(i) || IsBrokenPillar(j))
		{
			continue;
		}
		const float A0 = i * (TWO_PI / PillarCount);
		const float A1 = j * (TWO_PI / PillarCount);
		const FVector P0(FMath::Cos(A0) * PillarRing, FMath::Sin(A0) * PillarRing, 0.f);
		const FVector P1(FMath::Cos(A1) * PillarRing, FMath::Sin(A1) * PillarRing, 0.f);
		const FVector Mid = (P0 + P1) * 0.5f + FVector(0, 0, PillarBase + PillarHeight + LintelLift);
		const float Len = FVector::Dist(P0, P1);
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(P1.Y - P0.Y, P1.X - P0.X));
		YJJShapes::Add(this, R, EYJJShape::Cube, Mid, FVector(Len + LintelOverhang, LintelDepth, LintelThickness), MarbleMat, true, FRotator(0, Yaw, 0));
	}

	// 등불 수만큼 룬 석판. 등불이 켜지면 그 색으로 빛난다
	const FLinearColor RuneDark = FYJJData::Color(TEXT("ShrineRuneDark"));
	for (int32 i = 0; i < RuneCount; ++i)
	{
		const float A = i * (TWO_PI / RuneCount) + HALF_PI;
		UMaterialInstanceDynamic* MID = YJJShapes::Solid(this, RuneDark, 0.f, RuneRoughness);
		RuneMIDs.Add(MID);
		YJJShapes::Add(this, R, EYJJShape::Cube, FVector(FMath::Cos(A) * RuneRadius, FMath::Sin(A) * RuneRadius, TopZ + RuneSize.Z * 0.5f), RuneSize, MID, false,
			FRotator(0, FMath::RadiansToDegrees(A), 0));
	}

	// 가운데 받침대
	YJJShapes::Add(this, R, EYJJShape::Cylinder, FVector(0, 0, TopZ + PedestalHeight * 0.5f), FVector(PedestalDiameter, PedestalDiameter, PedestalHeight), MarbleMat, true);
	YJJShapes::Add(this, R, EYJJShape::Cylinder, FVector(0, 0, TopZ + PedestalTrimLift), FVector(PedestalTrimDiameter, PedestalTrimDiameter, PedestalTrimThickness), GoldMat, false);

	// 떠 있는 수정 (원뿔 두 개 = 팔면체)
	CrystalPivot = NewObject<USceneComponent>(this);
	CrystalPivot->SetMobility(EComponentMobility::Movable);
	CrystalPivot->SetupAttachment(R);
	CrystalPivot->SetRelativeLocation(FVector(0, 0, CrystalHeight));
	CrystalPivot->RegisterComponent();
	AddInstanceComponent(CrystalPivot);

	const FLinearColor Dormant = FYJJData::Color(TEXT("ShrineCrystalDormant"));
	CrystalMID = YJJShapes::Solid(this, Dormant, CrystalEmissiveDormant, CrystalRoughness);
	const FVector Half(CrystalWidth, CrystalWidth, CrystalHalfHeight * 2.f);
	YJJShapes::Add(this, CrystalPivot, EYJJShape::Cone, FVector(0, 0, CrystalHalfHeight), Half, CrystalMID, false);
	YJJShapes::Add(this, CrystalPivot, EYJJShape::Cone, FVector(0, 0, -CrystalHalfHeight), Half, CrystalMID, false, UpsideDown);
	for (int32 i = 0; i < ShardCount; ++i)
	{
		USceneComponent* ShardPivot = YJJShapes::Add(this, CrystalPivot, EYJJShape::Cone, FVector::ZeroVector, ShardSize, CrystalMID, false);
		Shards.Add(Cast<UStaticMeshComponent>(ShardPivot));
	}

	AuraMID = YJJShapes::Glow(this, FYJJData::Color(TEXT("ShrineCrystalColor")), AuraDormant, AuraCore, 1.f);
	Aura = YJJShapes::Add(this, CrystalPivot, EYJJShape::Sphere, FVector::ZeroVector, AuraSize, AuraMID, false);

	BeamMID = YJJShapes::Glow(this, FYJJData::Color(TEXT("ShrineBeam")), 0.f, BeamCore, BeamRim);
	Beam = YJJShapes::Add(this, R, EYJJShape::Cylinder, FVector(0, 0, BeamLength * 0.5f), FVector(BeamBaseWidth, BeamBaseWidth, BeamLength), BeamMID, false);
	Beam->SetVisibility(false);

	ShockMID = YJJShapes::Glow(this, FYJJData::Color(TEXT("ShrineShock")), 0.f, 0.f, ShockRim);
	Shockwave = YJJShapes::Add(this, R, EYJJShape::Sphere, FVector(0, 0, CrystalHeight), ShockSize, ShockMID, false);
	Shockwave->SetVisibility(false);

	SparkMID = YJJShapes::Glow(this, FYJJData::Color(TEXT("ShrineSpark")), SparkGlow, 1.f, 1.f);
	for (int32 i = 0; i < SparkCount; ++i)
	{
		UStaticMeshComponent* Spark = YJJShapes::Add(this, R, EYJJShape::Sphere, FVector(0, 0, CrystalHeight), FVector(SparkSize), SparkMID, false);
		Spark->SetVisibility(false);
		Sparks.Add(Spark);
	}

	Light = NewObject<UPointLightComponent>(this);
	Light->SetupAttachment(CrystalPivot);
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(LightDormant);
	Light->SetAttenuationRadius(LightRadius);
	Light->SetLightColor(FYJJData::Color(TEXT("ShrineCrystalColor")));
	Light->SetCastShadows(false);
	Light->RegisterComponent();
	AddInstanceComponent(Light);

	if (UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(this))
	{
		Adv->RegisterShrine(this);
	}
}

bool AYJJShrine::IsBrokenPillar(int32 Index)
{
	for (const int32 Broken : BrokenPillars)
	{
		if (Broken == Index)
		{
			return true;
		}
	}
	return false;
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
			FinaleCamera->GetCameraComponent()->SetFieldOfView(FYJJData::Tune(TEXT("Shrine.CameraFov")));
			FinaleCamera->GetCameraComponent()->bConstrainAspectRatio = false;
			Tick(0.f);
			const float Blend = FYJJData::Tune(TEXT("Shrine.CameraBlend"));
			PC->SetViewTargetWithBlend(FinaleCamera, Blend, VTBlend_EaseInOut, CameraBlendExp);
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
		const FLinearColor RuneDark = FYJJData::Color(TEXT("ShrineRuneDark"));
		const TArray<TWeakObjectPtr<AYJJLantern>>& Lanterns = Adv->GetLanterns();
		for (int32 i = 0; i < RuneMIDs.Num(); ++i)
		{
			const AYJJLantern* Lantern = Lanterns.IsValidIndex(i) ? Lanterns[i].Get() : nullptr;
			const float A = Lantern ? Lantern->GetLightAlpha() : 0.f;
			const FLinearColor C = Lantern ? Lantern->GetColor() : FLinearColor::White;
			RuneMIDs[i]->SetVectorParameterValue(TEXT("TopColor"), FMath::Lerp(RuneDark, C, A));
			RuneMIDs[i]->SetVectorParameterValue(TEXT("SideColor"), FMath::Lerp(RuneDark, C, A));
			RuneMIDs[i]->SetScalarParameterValue(TEXT("Emissive"), A * RuneGlow);
		}
	}

	// 수정: 평소엔 천천히 돌며 둥실, 깨어나면 밝게 맥동
	float Spin = bAwake ? SpinAwake : SpinDormant;
	float Height = CrystalHeight + FMath::Sin(Time * FloatSpeed) * FloatHeight;
	float Glow = bAwake ? GlowAwakeBase + GlowAwakePulse * FMath::Sin(Time * PulseSpeed) : CrystalEmissiveDormant;

	if (bFinale)
	{
		FinaleTime += DeltaTime;
		const float RiseTime = FYJJData::Tune(TEXT("Shrine.RiseTime"));
		const float Rise = FMath::SmoothStep(0.f, RiseTime, FinaleTime);
		Height = CrystalHeight + Rise * FYJJData::Tune(TEXT("Shrine.RiseHeight"));
		Spin = SpinAwake + Rise * SpinFinale;
		Glow = GlowFinaleBase + Rise * GlowFinaleRise;

		if (FinaleTime > RiseTime && !Beam->IsVisible())
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
			const float T = FinaleTime - RiseTime;
			// 처음엔 굵고 눈부시게, 곧 수정이 비쳐 보일 만큼 가늘고 옅게
			BeamMID->SetScalarParameterValue(TEXT("Intensity"), BeamGlow + BeamFlashGlow * FMath::Exp(-T * BeamFlashDecay));
			const float W = 1.f + BeamStartThicken * FMath::Exp(-T * BeamThinDecay);
			const float BeamScale = BeamWidth / YJJShapes::BaseShapeSize * W;
			Beam->SetRelativeScale3D(FVector(BeamScale, BeamScale, BeamLength / YJJShapes::BaseShapeSize));

			const float S = T / ShockTime;
			Shockwave->SetRelativeLocation(FVector(0, 0, Height));
			Shockwave->SetRelativeScale3D(FVector(1.f + S * ShockGrow, 1.f + S * ShockGrow, ShockFlatStart + S * ShockThicken));
			ShockMID->SetScalarParameterValue(TEXT("Intensity"), FMath::Max(0.f, ShockGlow * (1.f - S)));
			if (S >= 1.f)
			{
				Shockwave->SetVisibility(false);
			}

			// 불꽃: 수정에서 사방으로 솟아올라 천천히 떨어진다 (반복)
			for (int32 i = 0; i < Sparks.Num(); ++i)
			{
				const float Seed = YJJShapes::Hash(i * HashSpark + HashSparkSalt);
				const float Period = SparkPeriodMin + Seed * SparkPeriodRange;
				const float Local = FMath::Fmod(T + Seed * Period, Period);
				const float A = i * GoldenAngle;
				const float Speed = SparkSpeedMin + Seed * SparkSpeedRange;
				const FVector Dir(FMath::Cos(A), FMath::Sin(A), 0.f);
				const FVector P = FVector(0, 0, Height) + Dir * Speed * Local + FVector(0, 0, SparkRise * Local - SparkGravity * Local * Local);
				Sparks[i]->SetRelativeLocation(P);
				Sparks[i]->SetRelativeScale3D(FVector(SparkStartScale * (1.f - Local / Period) + SparkEndScale));
			}
		}

		// 환경: 노을빛으로
		for (TActorIterator<AYJJEnvironment> It(GetWorld()); It; ++It)
		{
			It->SetFinaleAlpha(FMath::SmoothStep(SunsetFrom, FYJJData::Tune(TEXT("Shrine.SunsetTime")), FinaleTime));
		}
	}

	CrystalPivot->SetRelativeLocation(FVector(0, 0, Height));

	// 엔딩 카메라: 천천히 돌며 솟아오르는 수정을 올려다본다. 엔딩이 끝나면 영웅에게 돌아간다
	if (FinaleCamera)
	{
		CameraAngle += DeltaTime * FMath::DegreesToRadians(FYJJData::Tune(TEXT("Shrine.CameraOrbitSpeed")));
		const float Approach = FMath::Min(FinaleTime, FYJJData::Tune(TEXT("Shrine.CameraApproachTime")));
		const float Dist = FYJJData::Tune(TEXT("Shrine.CameraDistance")) - Approach * FYJJData::Tune(TEXT("Shrine.CameraApproachSpeed"));
		const FVector Eye = GetActorLocation() + FVector(FMath::Cos(CameraAngle) * Dist, FMath::Sin(CameraAngle) * Dist,
			FYJJData::Tune(TEXT("Shrine.CameraHeight")) + Approach * FYJJData::Tune(TEXT("Shrine.CameraRiseSpeed")));
		const FVector Focus = GetActorLocation() + FVector(0, 0, Height + FYJJData::Tune(TEXT("Shrine.CameraLookAbove")));
		FinaleCamera->SetActorLocationAndRotation(Eye, (Focus - Eye).Rotation());

		const UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(this);
		if (!bCameraReturned && Adv && Adv->GetPhase() == EYJJPhase::Ended && Adv->GetPhaseTime() > FYJJData::Tune(TEXT("Shrine.CameraReturnDelay")))
		{
			bCameraReturned = true;
			if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
			{
				PC->SetViewTargetWithBlend(PC->GetPawn(), FYJJData::Tune(TEXT("Shrine.CameraBlend")), VTBlend_EaseInOut, CameraBlendExp);
			}
		}
	}
	CrystalPivot->AddLocalRotation(FRotator(0, Spin * DeltaTime, 0));
	const FLinearColor Body = bAwake ? FYJJData::Color(TEXT("ShrineCrystalColor")) : FYJJData::Color(TEXT("ShrineCrystalDormant"));
	CrystalMID->SetVectorParameterValue(TEXT("TopColor"), Body);
	CrystalMID->SetVectorParameterValue(TEXT("SideColor"), Body);
	CrystalMID->SetScalarParameterValue(TEXT("Emissive"), Glow);
	AuraMID->SetScalarParameterValue(TEXT("Intensity"), bAwake ? AuraAwakeBase + AuraAwakePulse * FMath::Sin(Time * PulseSpeed) : AuraDormant);
	Light->SetIntensity(bAwake ? LightAwake + Glow * LightPerGlow : LightDormantIdle);

	for (int32 i = 0; i < Shards.Num(); ++i)
	{
		const float A = Time * (bAwake ? ShardSpinAwake : ShardSpinDormant) + i * (TWO_PI / Shards.Num());
		const float Radius = bFinale ? ShardOrbit + FMath::Min(FinaleTime, ShardSpreadTime) * ShardSpreadSpeed : ShardOrbit;
		Shards[i]->SetRelativeLocation(FVector(FMath::Cos(A) * Radius, FMath::Sin(A) * Radius, FMath::Sin(Time * ShardBobSpeed + i) * ShardBob));
		Shards[i]->SetRelativeRotation(FRotator(Time * ShardTumblePitch + i * ShardTumbleOffset, Time * ShardTumbleYaw, 0));
	}
}
