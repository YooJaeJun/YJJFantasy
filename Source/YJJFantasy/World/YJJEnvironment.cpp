#include "World/YJJEnvironment.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "YJJAssetPaths.h"

AYJJEnvironment::AYJJEnvironment()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(RootComponent);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->SetAtmosphereSunLight(true);
	Sun->SetIntensity(9.f);
	Sun->SetLightColor(FLinearColor(1.f, 0.86f, 0.68f));
	// 볼류메트릭 포그 안에서 빛줄기가 보이도록 산란을 키운다
	Sun->VolumetricScatteringIntensity = 3.f;
	Sun->bEnableLightShaftBloom = true;
	Sun->BloomScale = 0.25f;
	Sun->BloomThreshold = 4.f;

	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
	Atmosphere->SetupAttachment(RootComponent);

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(RootComponent);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->bRealTimeCapture = true;
	SkyLight->SetIntensity(1.2f);

	Clouds = CreateDefaultSubobject<UVolumetricCloudComponent>(TEXT("Clouds"));
	Clouds->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CloudMaterial(
		TEXT("/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst.m_SimpleVolumetricCloud_Inst"));
	if (CloudMaterial.Succeeded())
	{
		Clouds->SetMaterial(CloudMaterial.Object);
	}
	// 하늘섬은 고도 3km. 구름층(0.6~2.2km)이 발밑에 바다처럼 깔린다
	Clouds->SetLayerBottomAltitude(0.6f);
	Clouds->SetLayerHeight(1.6f);

	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
	Fog->SetupAttachment(RootComponent);
	// 가까운 건 또렷하게, 먼 섬만 푸르게 바래도록 옅게
	Fog->SetFogDensity(0.004f);
	Fog->SetFogHeightFalloff(0.06f);
	Fog->SetStartDistance(2500.f);
	Fog->SetFogInscatteringColor(FLinearColor(0.28f, 0.48f, 0.9f));
	Fog->SetVolumetricFog(true);
	Fog->SetVolumetricFogScatteringDistribution(0.75f);
	Fog->SetVolumetricFogExtinctionScale(0.6f);
	Fog->SetVolumetricFogAlbedo(FColor(255, 240, 220));

	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcess->SetupAttachment(RootComponent);
	PostProcess->bUnbound = true;

	FPostProcessSettings& PP = PostProcess->Settings;
	// 노출 고정: 밝기가 출렁이지 않게
	PP.bOverride_AutoExposureMethod = true;
	PP.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	PP.bOverride_AutoExposureBias = true;
	// 물리 카메라 노출을 끈 수동 노출: 장면 밝기 x 2^Bias. 해 9lux 기준 0 근처가 적당
	PP.AutoExposureBias = -0.6f;
	PP.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	PP.AutoExposureApplyPhysicalCameraExposure = false;
	// 동화 같은 색감: 채도·대비 살짝 올리고, 그림자는 푸르게, 하이라이트는 따뜻하게
	PP.bOverride_ColorSaturation = true;
	PP.ColorSaturation = FVector4(1.25f, 1.25f, 1.25f, 1.f);
	PP.bOverride_ColorContrast = true;
	PP.ColorContrast = FVector4(1.05f, 1.05f, 1.05f, 1.f);
	PP.bOverride_ColorGainShadows = true;
	PP.ColorGainShadows = FVector4(0.92f, 0.98f, 1.12f, 1.f);
	PP.bOverride_ColorGainHighlights = true;
	PP.ColorGainHighlights = FVector4(1.06f, 1.0f, 0.92f, 1.f);
	PP.bOverride_BloomIntensity = true;
	PP.BloomIntensity = 0.9f;
	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = 0.35f;
}

void AYJJEnvironment::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Sun->SetWorldRotation(FRotator(-SunElevation, SunAzimuth, 0.f));
}

void AYJJEnvironment::BeginPlay()
{
	Super::BeginPlay();
	Sun->SetWorldRotation(FRotator(-SunElevation, SunAzimuth, 0.f));

	// 셀 셰이딩 + 외곽선 (머티리얼이 없으면 그냥 넘어간다)
	if (UMaterialInterface* Toon = LoadObject<UMaterialInterface>(nullptr, YJJAssetPaths::ToonPostProcess, nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		ToonMID = UMaterialInstanceDynamic::Create(Toon, this);
		PostProcess->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.f, ToonMID));
	}
}

void AYJJEnvironment::SetToonParams(float Ref, float Strength, float Outline)
{
	if (ToonMID)
	{
		ToonMID->SetScalarParameterValue(TEXT("Ref"), Ref);
		ToonMID->SetScalarParameterValue(TEXT("Strength"), Strength);
		ToonMID->SetScalarParameterValue(TEXT("OutlineStrength"), Outline);
	}
}

void AYJJEnvironment::SetFinaleAlpha(float Alpha)
{
	// 해가 낮아지며 금빛, 안개는 분홍빛
	Sun->SetWorldRotation(FRotator(-FMath::Lerp(SunElevation, 12.f, Alpha), SunAzimuth + Alpha * 25.f, 0.f));
	Sun->SetLightColor(FMath::Lerp(FLinearColor(1.f, 0.86f, 0.68f), FLinearColor(1.f, 0.62f, 0.38f), Alpha));
	Fog->SetFogInscatteringColor(FMath::Lerp(FLinearColor(0.35f, 0.5f, 0.8f), FLinearColor(0.85f, 0.5f, 0.55f), Alpha));
	PostProcess->Settings.BloomIntensity = FMath::Lerp(0.9f, 1.6f, Alpha);
}
