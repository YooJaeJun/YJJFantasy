#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "YJJHero.generated.h"

class UAnimSequence;
class UCameraComponent;
class USkeletalMeshComponent;
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
	/** 고양이 모델: Blender 키 1.55m -> 게임에서 약 1.7m, glTF 앞쪽(+Y)을 액터 앞(+X)으로 */
	static constexpr float HeroMeshScale = 1.1f;
	static constexpr float HeroMeshYaw = -90.f;

	static USceneComponent* MakePivot(AActor* Owner, USceneComponent* Parent, const FVector& Location);

	// ---- 몸·카메라 기본 (생성자에서 쓰여 이름 붙은 상수)
	static constexpr float CapsuleRadius = 38.f;
	static constexpr float CapsuleHalfHeight = 96.f;
	inline static const FVector CameraSocketOffset = FVector(0.f, 40.f, 70.f);
	static constexpr float CameraProbeSize = 16.f;
	static constexpr float GamepadLookScale = 2.5f;
	static constexpr float WarpLookDown = 12.f;
	static constexpr float HurtFlashFade = 2.5f;
	static constexpr float MinDeltaTime = 0.001f;
	/** 무적일 때 깜빡임 (초) */
	static constexpr float BlinkPeriod = 0.14f;
	static constexpr float BlinkOff = 0.05f;
	static constexpr float ShakeFreqY = 61.f;
	static constexpr float ShakeFreqZ = 47.f;
	/** 콤보 세 번째(0부터 2)는 위에서 내려찍기 */
	static constexpr int32 FinisherCombo = 2;

	/** 검 (cm, 몸 기준) */
	struct FSwordShape
	{
		FVector BackSpot;
		FVector RestSpot;
		float RestRoll;
		float Scale;
		float BladeZ;
		FVector BladeSize;
		float TipZ;
		FVector TipSize;
		float TipRoll;
		float GuardZ;
		FVector GuardSize;
		float GripZ;
		FVector GripSize;
		float PommelZ;
		float PommelSize;
		float PommelGlow;
		float BladeEmissive;
		float BladeRoughness;
		float GoldEmissive;
		float GoldRoughness;
		float LeatherRoughness;
	};
	inline static const FSwordShape Sword = {
		FVector(-30.f, 0.f, 60.f), FVector(-34.f, 0.f, 40.f), 35.f, 0.75f,
		62.f, FVector(4.f, 12.f, 104.f), 116.f, FVector(4.f, 6.f, 10.f), 45.f,
		8.f, FVector(10.f, 34.f, 8.f), -10.f, FVector(8.f, 8.f, 26.f), -26.f, 11.f, 2.f,
		0.6f, 0.15f, 0.1f, 0.35f, 0.7f,
	};

	/** 베기 궤적 원판 */
	struct FSlashShape
	{
		FVector Spot;
		float Diameter;
		float Thickness;
		float Rim;
		float SweepSpeed;
		float SweepHalf;
		float Pitch;
		float FinisherPitch;
		float Scale;
		float MinWidth;
		float Flat;
		float Glow;
		float FadeSpeed;
	};
	inline static const FSlashShape SlashFx = { FVector(70.f, 0.f, 80.f), 380.f, 4.f, 2.f, 1.6f, 80.f, 8.f, 70.f, 3.8f, 0.01f, 0.04f, 2.5f, 1.3f };

	/** 활공 날개 (깃털 세 겹, cm) */
	struct FWingShape
	{
		FVector Spot;
		float SweepYaw;
		float SweepRoll;
		float BarForward;
		float BarHalfSpan;
		float BarLift;
		FVector BarSize;
		int32 Layers;
		float FeatherLength;
		float LayerShorten;
		float FeatherBack;
		float LayerBack;
		float LayerDrop;
		float LayerTilt;
		float FeatherLift;
		float FeatherWidth;
		float FeatherThickness;
		float TipLength;
		float FeatherEmissive;
		float FeatherRoughness;
		float TipEmissive;
		float TipRoughness;
		float FoldedScale;
		float OpenSpeed;
		float HiddenBelow;
	};
	inline static const FWingShape Wing = {
		FVector(-10.f, 0.f, 165.f), 14.f, 12.f, 14.f, 120.f, 26.f, FVector(8.f, 236.f, 7.f),
		3, 230.f, 45.f, 8.f, 26.f, 3.f, 4.f, 20.f, 34.f, 4.f, 36.f,
		0.2f, 0.7f, 0.3f, 0.6f, 0.01f, 7.f, 0.02f,
	};

	/** 발밑 그림자 (점프 착지 위치) */
	struct FShadowShape
	{
		float Diameter;
		float Thickness;
		float Search;
		float FadeHeight;
		float MinScale;
		float ShowAbove;
	};
	inline static const FShadowShape Shadow = { 90.f, 2.f, 3000.f, 1500.f, 0.3f, 60.f };

	/** 애니메이션 고르기 (속도 cm/s) */
	struct FAnimPick
	{
		float RestartWindow;
		float HurtTime;
		float JumpRiseSpeed;
		float RunFromSpeed;
		float RunNaturalSpeed;
		float RunRateMin;
		float RunRateMax;
		float WalkFromSpeed;
		float WalkNaturalSpeed;
		float WalkRateMin;
		float WalkRateMax;
	};
	inline static const FAnimPick AnimPick = { 0.02f, 0.4f, 150.f, 760.f, 1050.f, 0.7f, 1.4f, 40.f, 480.f, 0.5f, 1.6f };

	/** 회전·활공할 때 몸 기울기 (도) */
	struct FLeanShape
	{
		float RunPerYaw;
		float RunMax;
		float GlidePerYaw;
		float GlideMax;
		float GlidePitch;
		float Speed;
	};
	inline static const FLeanShape Lean = { 0.03f, 18.f, 0.05f, 30.f, 35.f, 6.f };

	/** 착지 찌부러짐 (고무공) */
	struct FSquashShape
	{
		float Recover;
		float Sink;
		float Widen;
		float Flatten;
		float FullAtSpeed;
		float Min;
	};
	inline static const FSquashShape Squash = { 9.f, 10.f, 0.22f, 0.28f, 1400.f, 0.2f };

	/** 소리 높낮이 */
	struct FSoundShape
	{
		float PitchMin;
		float PitchMax;
		float ComboPitchStep;
		float LandFullAtSpeed;
		float LandMin;
	};
	inline static const FSoundShape Sound = { 0.95f, 1.05f, 0.08f, 1500.f, 0.2f };

	/** 엔딩 때 수정을 올려다보는 시선 */
	struct FFinaleLook
	{
		float EyeHeight;
		float LookRise;
		float LookRiseTime;
		float PitchMin;
		float PitchMax;
		float TurnSpeed;
	};
	inline static const FFinaleLook Finale = { 70.f, 500.f, 6.f, -5.f, 22.f, 1.5f };

	/** Tuning.csv 의 Hero.* 를 이동·카메라에 적용 */
	void ApplyTuning();
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

	// ---- 몸: Blender로 만든 리깅 고양이 (Tools/Character/make_hero.py)
	UPROPERTY()
	TObjectPtr<USceneComponent> Visual;
	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> Body;

	/** 애니메이션 상태 (단일 노드 재생, 상태가 바뀔 때만 교체) */
	enum class EAnim : uint8 { Idle, Walk, Run, Jump, Fall, Glide, Attack1, Attack2, Attack3, Hurt, Count };
	UPROPERTY()
	TArray<TObjectPtr<UAnimSequence>> Anims;
	EAnim CurrentAnim = EAnim::Count;
	void PlayAnim(EAnim Anim, float Rate);
	void UpdateAnimation();

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
