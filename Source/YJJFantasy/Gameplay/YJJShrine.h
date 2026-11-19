#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YJJShrine.generated.h"

class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * 바람의 성소. 중앙 섬의 큰 제단과 떠 있는 수정.
 * 등불이 모두 켜지면 깨어나고, 상호작용하면 엔딩 연출이 시작된다.
 */
UCLASS()
class YJJFANTASY_API AYJJShrine : public AActor
{
	GENERATED_BODY()

public:
	AYJJShrine();

	virtual void Tick(float DeltaTime) override;

	void SetAwake(bool bInAwake);
	bool IsAwake() const { return bAwake; }

	bool CanInteract(const FVector& From) const;
	void Interact();

	/** 엔딩 중 카메라가 바라볼 곳 */
	FVector GetCrystalLocation() const;

	static constexpr float InteractRange = 520.f;

protected:
	virtual void BeginPlay() override;

private:
	/** 제단 모양: 룬 타일 수·반지름, 제단 윗면 높이 */
	static constexpr int32 RuneCount = 5;
	static constexpr float RuneRadius = 380.f;
	static constexpr float TopZ = 120.f;

	bool bAwake = false;
	bool bFinale = false;
	float Time = 0.f;
	float FinaleTime = 0.f;
	float CrystalHeight = 650.f;

	UPROPERTY()
	TObjectPtr<USceneComponent> CrystalPivot;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Aura;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Beam;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Shockwave;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Shards;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Sparks;

	UPROPERTY()
	TObjectPtr<UPointLightComponent> Light;

	/** 엔딩 연출용 카메라 (성소 둘레를 천천히 돈다) */
	UPROPERTY()
	TObjectPtr<class ACameraActor> FinaleCamera;

	float CameraAngle = 0.f;
	bool bCameraReturned = false;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> CrystalMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> AuraMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BeamMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> ShockMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> SparkMID;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> RuneMIDs;
};
