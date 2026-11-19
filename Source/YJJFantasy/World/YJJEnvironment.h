#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YJJEnvironment.generated.h"

class UDirectionalLightComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UVolumetricCloudComponent;
class UExponentialHeightFogComponent;
class UPostProcessComponent;

/**
 * 판타지 분위기 조명 한 세트: 낮게 깔린 따뜻한 해, 대기, 볼류메트릭 구름, 빛줄기(볼류메트릭 포그), 색감 보정.
 * 맵에 하나 두면 되고, 없으면 게임모드가 시작할 때 스폰한다.
 */
UCLASS()
class YJJFANTASY_API AYJJEnvironment : public AActor
{
	GENERATED_BODY()

public:
	AYJJEnvironment();

	/** 해 고도(도). 낮을수록 그림자가 길고 노을빛이 난다 */
	UPROPERTY(EditAnywhere, Category = "YJJ|Sun", meta = (ClampMin = "-10", ClampMax = "90"))
	float SunElevation = 22.f;

	/** 해 방위(도) */
	UPROPERTY(EditAnywhere, Category = "YJJ|Sun")
	float SunAzimuth = 35.f;

	virtual void OnConstruction(const FTransform& Transform) override;

	/** 엔딩 연출: 0 = 평소, 1 = 황금빛 노을 */
	void SetFinaleAlpha(float Alpha);

	/** 셀 셰이딩 세기 조절 (테스트용 콘솔 명령) */
	void SetToonParams(float Ref, float Strength, float Outline);

protected:
	virtual void BeginPlay() override;

	// ---- 조명·하늘·색감 (생성자에서 쓰여서 데이터 테이블 대신 이름 붙은 상수)
	static constexpr float SunIntensityLux = 9.f;
	inline static const FLinearColor SunColor = FLinearColor(1.f, 0.86f, 0.68f);
	static constexpr float SunVolumetricScattering = 3.f;
	static constexpr float SunBloomScale = 0.25f;
	static constexpr float SunBloomThreshold = 4.f;
	/** 고도 3km라 머리 위가 진한 남색이 되는데, 밝은 하늘색으로 끌어올린다 */
	inline static const FLinearColor SkyLuminance = FLinearColor(1.6f, 1.75f, 1.9f);
	static constexpr float SkyMultiScattering = 2.f;
	static constexpr float SkyLightIntensity = 1.2f;
	/** 볼류메트릭 구름층 (km). 만화풍에선 숨기고 월드 빌더가 만화 구름을 깐다 */
	static constexpr float CloudLayerBottomKm = 0.6f;
	static constexpr float CloudLayerHeightKm = 1.6f;
	static constexpr float FogDensity = 0.0022f;
	static constexpr float FogHeightFalloff = 0.06f;
	static constexpr float FogStartDistance = 2500.f;
	inline static const FLinearColor FogColor = FLinearColor(0.45f, 0.68f, 1.f);
	static constexpr float FogScatteringDistribution = 0.75f;
	static constexpr float FogExtinctionScale = 0.6f;
	inline static const FColor FogAlbedo = FColor(255, 240, 220);
	/** 물리 카메라 노출을 끈 수동 노출: 장면 밝기 x 2^Bias */
	static constexpr float ExposureBias = -0.6f;
	static constexpr float Saturation = 1.32f;
	static constexpr float Contrast = 1.05f;
	inline static const FVector4 ShadowGain = FVector4(0.92f, 0.98f, 1.12f, 1.f);
	inline static const FVector4 HighlightGain = FVector4(1.06f, 1.0f, 0.92f, 1.f);
	static constexpr float BloomIntensity = 0.55f;
	static constexpr float VignetteIntensity = 0.18f;
	/** 셀 셰이딩 명암 단계화 세기 (머티리얼 기본값보다 평평하게) */
	static constexpr float ToonStrength = 0.88f;

	// ---- 엔딩 노을
	static constexpr float FinaleSunElevation = 12.f;
	static constexpr float FinaleSunTurn = 25.f;
	inline static const FLinearColor FinaleSunColor = FLinearColor(1.f, 0.62f, 0.38f);
	inline static const FLinearColor FinaleFogFrom = FLinearColor(0.35f, 0.5f, 0.8f);
	inline static const FLinearColor FinaleFogTo = FLinearColor(0.85f, 0.5f, 0.55f);
	static constexpr float FinaleBloomFrom = 0.9f;
	static constexpr float FinaleBloomTo = 1.6f;

	UPROPERTY()
	TObjectPtr<class UMaterialInstanceDynamic> ToonMID;

	UPROPERTY(VisibleAnywhere, Category = "YJJ")
	TObjectPtr<UDirectionalLightComponent> Sun;

	UPROPERTY(VisibleAnywhere, Category = "YJJ")
	TObjectPtr<USkyAtmosphereComponent> Atmosphere;

	UPROPERTY(VisibleAnywhere, Category = "YJJ")
	TObjectPtr<USkyLightComponent> SkyLight;

	UPROPERTY(VisibleAnywhere, Category = "YJJ")
	TObjectPtr<UVolumetricCloudComponent> Clouds;

	UPROPERTY(VisibleAnywhere, Category = "YJJ")
	TObjectPtr<UExponentialHeightFogComponent> Fog;

	UPROPERTY(VisibleAnywhere, Category = "YJJ")
	TObjectPtr<UPostProcessComponent> PostProcess;
};
