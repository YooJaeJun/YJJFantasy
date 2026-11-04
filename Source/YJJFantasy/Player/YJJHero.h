#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "YJJHero.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

/**
 * 3인칭 모험 영웅.
 * 이동·달리기·점프·활공(공중에서 Space)·검 공격·상호작용(E).
 * 몸은 기본 도형으로 조립하고 코드로 애니메이션한다 (스켈레탈 메시 없음).
 * 입력은 Enhanced Input을 에셋 없이 코드에서 만든다.
 */
UCLASS()
class YJJFANTASY_API AYJJHero : public ACharacter
{
	GENERATED_BODY()

public:
	AYJJHero();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;
	virtual void Tick(float DeltaTime) override;
	virtual void Landed(const FHitResult& Hit) override;

	// ---- 상태 (HUD용)
	float GetStamina() const { return Stamina; }
	float GetMaxStamina() const;
	int32 GetHealth() const { return Health; }
	int32 GetMaxHealth() const { return MaxHealth; }
	bool IsGliding() const { return bGliding; }
	bool IsDead() const { return bDead; }
	bool IsExhausted() const { return bExhausted; }
	float GetHurtFlash() const { return HurtFlash; }
	/** 화면 암전 정도 0~1 (낙사·부활) */
	float GetScreenFade() const { return ScreenFade; }
	/** 지금 상호작용할 수 있는 것의 설명. 없으면 빈 문자열 */
	FString GetInteractPrompt() const;

	/** 슬라임 등에 닿음 */
	void TakeHit(const FVector& From);

	/** 체크포인트로 순간이동 (테스트·부활) */
	void WarpTo(const FVector& Location, float Yaw);

	/** 자동 플레이 테스트에서 직접 부른다 */
	void DoInteract();
	void DoAttack();
	void SetGliding(bool bOn);

	/** 낙사 판정 높이 */
	float KillZ = -100000.f;

protected:
	virtual void BeginPlay() override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void JumpPressed(const FInputActionValue& Value);
	void JumpReleased(const FInputActionValue& Value);
	void StartSprint(const FInputActionValue& Value);
	void StopSprint(const FInputActionValue& Value);
	void InteractPressed(const FInputActionValue& Value);
	void AttackPressed(const FInputActionValue& Value);

	UPROPERTY(VisibleAnywhere, Category = "YJJ|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "YJJ|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(EditAnywhere, Category = "YJJ|Move")
	float WalkSpeed = 620.f;

	UPROPERTY(EditAnywhere, Category = "YJJ|Move")
	float SprintSpeed = 1050.f;

	UPROPERTY(EditAnywhere, Category = "YJJ|Move")
	float GlideSpeed = 1100.f;

	UPROPERTY(EditAnywhere, Category = "YJJ|Move")
	float GlideSinkSpeed = 230.f;

	UPROPERTY(EditAnywhere, Category = "YJJ|Move")
	float UpdraftSpeed = 1500.f;

	UPROPERTY(EditAnywhere, Category = "YJJ|Stamina")
	float BaseStamina = 100.f;

	UPROPERTY(EditAnywhere, Category = "YJJ|Camera")
	float LookSensitivity = 1.f;

private:
	void BuildInput();
	void BuildBody();
	void UpdateMovement(float DeltaTime);
	void UpdateStamina(float DeltaTime);
	void UpdateAttack(float DeltaTime);
	void UpdateBody(float DeltaTime);
	void UpdateCamera(float DeltaTime);
	void UpdateRespawn(float DeltaTime);
	void UpdateFinale(float DeltaTime);
	bool IsInUpdraft() const;
	AActor* FindInteractable(FString* OutPrompt) const;
	void Respawn();
	bool CanAct() const;

	// ---- 입력
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
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> InteractAction;
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> AttackAction;

	// ---- 몸
	UPROPERTY()
	TObjectPtr<USceneComponent> Visual;
	UPROPERTY()
	TObjectPtr<USceneComponent> Torso;
	UPROPERTY()
	TObjectPtr<USceneComponent> Head;
	UPROPERTY()
	TObjectPtr<USceneComponent> LegL;
	UPROPERTY()
	TObjectPtr<USceneComponent> LegR;
	UPROPERTY()
	TObjectPtr<USceneComponent> ArmL;
	UPROPERTY()
	TObjectPtr<USceneComponent> ArmR;
	UPROPERTY()
	TArray<TObjectPtr<USceneComponent>> Scarf;
	UPROPERTY()
	TArray<TObjectPtr<USceneComponent>> Ponytail;
	UPROPERTY()
	TObjectPtr<USceneComponent> SwordPivot;
	UPROPERTY()
	TObjectPtr<USceneComponent> Glider;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Slash;
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> SlashMID;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> DropShadow;

	// ---- 상태
	float Stamina = 100.f;
	float StaminaRegenDelay = 0.f;
	bool bExhausted = false;
	bool bSprintHeld = false;
	bool bSprinting = false;
	bool bGliding = false;
	bool bWasInUpdraft = false;
	float GlideBlend = 0.f;
	FVector2D MoveInput = FVector2D::ZeroVector;

	int32 MaxHealth = 5;
	int32 Health = 5;
	float Invulnerable = 0.f;
	float HurtFlash = 0.f;
	float SinceHurt = 100.f;
	bool bDead = false;
	float RespawnTimer = -1.f;
	float ScreenFade = 1.f;

	float AttackTime = -1.f;
	int32 AttackCombo = 0;
	bool bAttackHitDone = false;

	float WalkPhase = 0.f;
	float LandSquash = 0.f;
	float LeanPitch = 0.f;
	float LeanRoll = 0.f;
	float LastYaw = 0.f;
	float CameraShake = 0.f;
	float Time = 0.f;
};
