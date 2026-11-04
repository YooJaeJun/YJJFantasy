#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YJJChest.generated.h"

class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * 숨겨진 보물 상자. 열면 뚜껑이 젖혀지며 별가루가 쏟아진다.
 * 찾기 어려운 곳(나무 꼭대기, 떠 있는 바위, 탑 뒤)에 있어서 탐험할 이유가 된다.
 */
UCLASS()
class YJJFANTASY_API AYJJChest : public AActor
{
	GENERATED_BODY()

public:
	AYJJChest();

	virtual void Tick(float DeltaTime) override;

	bool CanInteract(const FVector& From) const;
	void Interact();
	bool IsOpened() const { return bOpened; }

	static constexpr int32 StardustInside = 5;
	static constexpr float InteractRange = 230.f;

protected:
	virtual void BeginPlay() override;

private:
	bool bOpened = false;
	float Time = 0.f;
	float OpenTime = 0.f;

	UPROPERTY()
	TObjectPtr<USceneComponent> LidPivot;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Glow;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> GlowMID;

	UPROPERTY()
	TObjectPtr<UPointLightComponent> Light;
};
