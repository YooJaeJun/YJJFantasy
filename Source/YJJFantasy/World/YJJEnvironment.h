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
