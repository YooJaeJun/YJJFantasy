#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "YJJHero.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

/**
 * 3인칭 모험 영웅.
 * 입력(Enhanced Input)은 에셋 없이 코드에서 만든다: WASD/왼스틱 이동, 마우스/오른스틱 시점, Space/A 점프, Shift 달리기.
 * 몸체는 임시 원기둥 메시이며, AI로 만든 캐릭터가 준비되면 교체한다.
 */
UCLASS()
class YJJFANTASY_API AYJJHero : public ACharacter
{
	GENERATED_BODY()

public:
	AYJJHero();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;

protected:
	virtual void BeginPlay() override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartSprint(const FInputActionValue& Value);
	void StopSprint(const FInputActionValue& Value);

	UPROPERTY(VisibleAnywhere, Category = "YJJ|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "YJJ|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	/** 임시 몸체 */
	UPROPERTY(VisibleAnywhere, Category = "YJJ|Body")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(EditAnywhere, Category = "YJJ|Move")
	float WalkSpeed = 500.f;

	UPROPERTY(EditAnywhere, Category = "YJJ|Move")
	float SprintSpeed = 900.f;

	UPROPERTY(EditAnywhere, Category = "YJJ|Camera")
	float LookSensitivity = 1.f;

private:
	void BuildInput();

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> SprintAction;
};
