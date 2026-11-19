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
	Sun->SetIntensity(SunIntensityLux);
	Sun->SetLightColor(SunColor);
	// 볼류메트릭 포그 안에서 빛줄기가 보이도록 산란을 키운다
	Sun->VolumetricScatteringIntensity = SunVolumetricScattering;
	// 만화풍: 빛줄기 블룸은 끈다
	Sun->bEnableLightShaftBloom = false;
	Sun->BloomScale = SunBloomScale;
	Sun->BloomThreshold = SunBloomThreshold;

	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
	Atmosphere->SetupAttachment(RootComponent);
	// 만화풍 하늘: 고도 3km라 머리 위가 진한 남색이 되는데, 밝은 하늘색으로 끌어올린다
	Atmosphere->SkyLuminanceFactor = SkyLuminance;
	Atmosphere->MultiScatteringFactor = SkyMultiScattering;

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(RootComponent);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->bRealTimeCapture = true;
	SkyLight->SetIntensity(SkyLightIntensity);

	Clouds = CreateDefaultSubobject<UVolumetricCloudComponent>(TEXT("Clouds"));
	Clouds->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CloudMaterial(YJJAssetPaths::CloudMaterial);
	if (CloudMaterial.Succeeded())
	{
		Clouds->SetMaterial(CloudMaterial.Object);
	}
	// 하늘섬은 고도 3km. 구름층(0.6~2.2km)이 발밑에 바다처럼 깔린다
	Clouds->SetLayerBottomAltitude(CloudLayerBottomKm);
	Clouds->SetLayerHeight(CloudLayerHeightKm);
	// 만화풍 배경: 사실적인 볼류메트릭 구름 대신 월드 빌더가 몽실한 만화 구름 바다를 깐다
	Clouds->SetVisibility(false);

	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
	Fog->SetupAttachment(RootComponent);
	// 가까운 건 또렷하게, 먼 섬만 푸르게 바래도록 옅게
	Fog->SetFogDensity(FogDensity);
	Fog->SetFogHeightFalloff(FogHeightFalloff);
	Fog->SetStartDistance(FogStartDistance);
	Fog->SetFogInscatteringColor(FogColor);
	Fog->SetVolumetricFog(false);
	Fog->SetVolumetricFogScatteringDistribution(FogScatteringDistribution);
	Fog->SetVolumetricFogExtinctionScale(FogExtinctionScale);
	Fog->SetVolumetricFogAlbedo(FogAlbedo);

	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcess->SetupAttachment(RootComponent);
	PostProcess->bUnbound = true;

	FPostProcessSettings& PP = PostProcess->Settings;
	// 노출 고정: 밝기가 출렁이지 않게
	PP.bOverride_AutoExposureMethod = true;
	PP.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	PP.bOverride_AutoExposureBias = true;
	// 물리 카메라 노출을 끈 수동 노출: 장면 밝기 x 2^Bias. 해 9lux 기준 0 근처가 적당
	PP.AutoExposureBias = ExposureBias;
	PP.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	PP.AutoExposureApplyPhysicalCameraExposure = false;
	// 동화 같은 색감: 채도·대비 살짝 올리고, 그림자는 푸르게, 하이라이트는 따뜻하게
	PP.bOverride_ColorSaturation = true;
	PP.ColorSaturation = FVector4(Saturation, Saturation, Saturation, 1.f);
	PP.bOverride_ColorContrast = true;
	PP.ColorContrast = FVector4(Contrast, Contrast, Contrast, 1.f);
	PP.bOverride_ColorGainShadows = true;
	PP.ColorGainShadows = ShadowGain;
	PP.bOverride_ColorGainHighlights = true;
	PP.ColorGainHighlights = HighlightGain;
	PP.bOverride_BloomIntensity = true;
	PP.BloomIntensity = BloomIntensity;
	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = VignetteIntensity;
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
		// 만화풍: 명암을 더 평평하게 끊는다
		ToonMID->SetScalarParameterValue(TEXT("Strength"), ToonStrength);
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
	Sun->SetWorldRotation(FRotator(-FMath::Lerp(SunElevation, FinaleSunElevation, Alpha), SunAzimuth + Alpha * FinaleSunTurn, 0.f));
	Sun->SetLightColor(FMath::Lerp(SunColor, FinaleSunColor, Alpha));
	Fog->SetFogInscatteringColor(FMath::Lerp(FinaleFogFrom, FinaleFogTo, Alpha));
	PostProcess->Settings.BloomIntensity = FMath::Lerp(FinaleBloomFrom, FinaleBloomTo, Alpha);
}
