#include "Player/YJJHero.h"

#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/YJJAdventureSubsystem.h"
#include "Core/YJJAudio.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SkeletalMesh.h"
#include "Gameplay/YJJChest.h"
#include "Gameplay/YJJLantern.h"
#include "Gameplay/YJJShrine.h"
#include "Gameplay/YJJSlime.h"
#include "Gameplay/YJJTraversal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "World/YJJShapes.h"
#include "YJJAssetPaths.h"
#include "YJJFantasy.h"

namespace
{
	// 몸 색: 하얀 여행자 옷 + 청록 망토 + 금발
	namespace HeroColor
	{
		const FLinearColor Skin(1.f, 0.82f, 0.7f);
		const FLinearColor Cloth(0.95f, 0.93f, 0.88f);
		const FLinearColor ClothShade(0.8f, 0.8f, 0.86f);
		const FLinearColor Trim(0.16f, 0.55f, 0.62f);
		const FLinearColor Hair(0.98f, 0.82f, 0.45f);
		const FLinearColor HairShade(0.85f, 0.6f, 0.3f);
		const FLinearColor Leather(0.42f, 0.27f, 0.17f);
		const FLinearColor Gold(0.95f, 0.72f, 0.3f);
		const FLinearColor ScarfColor(0.95f, 0.38f, 0.32f);
		const FLinearColor EyeColor(0.1f, 0.45f, 0.5f);
	}

	constexpr float StaminaSprintCost = 20.f;
	constexpr float StaminaGlideCost = 7.f;
	constexpr float StaminaRegen = 32.f;
	constexpr float AttackDuration = 0.34f;
	constexpr float AttackRange = 260.f;

	// 고양이 모델: Blender 키 1.55m -> 게임에서 약 1.7m, glTF 앞쪽(+Y)을 액터 앞(+X)으로
	constexpr float HeroMeshScale = 1.1f;
	constexpr float HeroMeshYaw = -90.f;

	USceneComponent* MakePivot(AActor* Owner, USceneComponent* Parent, const FVector& Location)
	{
		USceneComponent* Pivot = NewObject<USceneComponent>(Owner);
		Pivot->SetMobility(EComponentMobility::Movable);
		Pivot->SetupAttachment(Parent);
		Pivot->SetRelativeLocation(Location);
		Pivot->RegisterComponent();
		Owner->AddInstanceComponent(Pivot);
		return Pivot;
	}
}

AYJJHero::AYJJHero()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(38.f, 96.f);

	// 카메라 방향으로 몸을 돌리지 않고, 이동 방향으로 몸을 돌린다 (모험 게임 기본 조작감)
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->bOrientRotationToMovement = true;
	MoveComp->RotationRate = FRotator(0.f, 720.f, 0.f);
	MoveComp->MaxWalkSpeed = WalkSpeed;
	MoveComp->MaxAcceleration = 3000.f;
	MoveComp->JumpZVelocity = 880.f;
	MoveComp->AirControl = 0.55f;
	MoveComp->GravityScale = 1.75f;
	MoveComp->BrakingDecelerationWalking = 2600.f;
	MoveComp->BrakingDecelerationFalling = 200.f;
	MoveComp->MaxStepHeight = 50.f;
	MoveComp->SetWalkableFloorAngle(50.f);
	MoveComp->bCanWalkOffLedgesWhenCrouching = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 430.f;
	CameraBoom->SocketOffset = FVector(0.f, 40.f, 70.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 10.f;
	CameraBoom->ProbeSize = 16.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->FieldOfView = 80.f;
}

void AYJJHero::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	Stamina = GetMaxStamina();
	LastYaw = GetActorRotation().Yaw;
	BuildBody();

	if (UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(this))
	{
		if (Adv->GetCheckpoint().IsZero())
		{
			Adv->SetCheckpoint(GetActorLocation(), GetActorRotation().Yaw);
		}
	}
}

// ============================================================ 몸 조립

void AYJJHero::BuildBody()
{
	UMaterialInterface* LeatherMat = YJJShapes::Solid(this, HeroColor::Leather, 0.f, 0.7f);
	UMaterialInterface* GoldMat = YJJShapes::Solid(this, HeroColor::Gold, 0.1f, 0.35f);

	// 발바닥 = 캡슐 아래 (-96). 기울기·찌그러짐은 이 축에 준다
	Visual = MakePivot(this, RootComponent, FVector(0, 0, -96));

	// 리깅된 고양이 몸 (Blender: Tools/Character/make_hero.py)
	Body = NewObject<USkeletalMeshComponent>(this, TEXT("CatBody"));
	Body->SetupAttachment(Visual);
	if (USkeletalMesh* CatMesh = LoadObject<USkeletalMesh>(nullptr, YJJAssetPaths::HeroMesh))
	{
		Body->SetSkeletalMesh(CatMesh);
	}
	else
	{
		UE_LOG(LogYJJFantasy, Warning, TEXT("[Hero] 캐릭터 메시 없음: BuildContent.ps1로 임포트 필요"));
	}
	Body->SetRelativeScale3D(FVector(HeroMeshScale));
	Body->SetRelativeRotation(FRotator(0.f, HeroMeshYaw, 0.f));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Body->RegisterComponent();
	AddInstanceComponent(Body);
	GetMesh()->SetVisibility(false);

	static const TCHAR* AnimNames[] = { TEXT("Idle"), TEXT("Walk"), TEXT("Run"), TEXT("Jump"), TEXT("Fall"), TEXT("Glide"), TEXT("Attack1"), TEXT("Attack2"), TEXT("Attack3"), TEXT("Hurt") };
	static_assert(UE_ARRAY_COUNT(AnimNames) == (int32)EAnim::Count, "애니메이션 이름 수");
	for (const TCHAR* Name : AnimNames)
	{
		const FString Path = FString::Printf(TEXT("%s/SK_Hero%s.SK_Hero%s"), YJJAssetPaths::HeroAnimDir, Name, Name);
		UAnimSequence* Anim = LoadObject<UAnimSequence>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!Anim)
		{
			UE_LOG(LogYJJFantasy, Warning, TEXT("[Hero] 애니메이션 없음 %s"), *Path);
		}
		Anims.Add(Anim);
	}
	PlayAnim(EAnim::Idle, 1.f);

	// 검: 평소엔 등에 비스듬히, 공격할 때 오른손에 (고양이 몸에 맞춘 짧은 검)
	SwordPivot = MakePivot(this, Visual, FVector(-30, 0, 60));
	SwordPivot->SetRelativeScale3D(FVector(0.75f));
	UMaterialInterface* Blade = YJJShapes::Surface(this, FLinearColor(0.85f, 0.95f, 1.f), FLinearColor(0.55f, 0.7f, 0.85f), 0.6f, 0.15f);
	YJJShapes::Add(this, SwordPivot, EYJJShape::Cube, FVector(0, 0, 62), FVector(4, 12, 104), Blade, false);
	YJJShapes::Add(this, SwordPivot, EYJJShape::Cube, FVector(0, 0, 116), FVector(4, 6, 10), Blade, false, FRotator(0, 0, 45));
	YJJShapes::Add(this, SwordPivot, EYJJShape::Cube, FVector(0, 0, 8), FVector(10, 34, 8), GoldMat, false);
	YJJShapes::Add(this, SwordPivot, EYJJShape::Cylinder, FVector(0, 0, -10), FVector(8, 8, 26), LeatherMat, false);
	YJJShapes::Add(this, SwordPivot, EYJJShape::Sphere, FVector(0, 0, -26), FVector(11, 11, 11), YJJShapes::Solid(this, FLinearColor(0.3f, 0.85f, 1.f), 2.f), false);

	// 베기 궤적 (납작한 빛 원판)
	SlashMID = YJJShapes::Glow(this, FLinearColor(0.7f, 0.95f, 1.f), 0.f, 0.f, 2.f);
	Slash = YJJShapes::Add(this, Visual, EYJJShape::Cylinder, FVector(70, 0, 80), FVector(380, 380, 4), SlashMID, false);
	Slash->SetVisibility(false);

	// 활공 날개: 머리 위에서 펼쳐져 두 앞발로 붙잡는다
	Glider = MakePivot(this, Visual, FVector(-10, 0, 165));
	// 깃털 세 겹 날개: 앞쪽이 길고 뒤로 갈수록 짧아진다. 끝은 청록색
	UMaterialInterface* Feather = YJJShapes::Surface(this, FLinearColor(1.f, 0.98f, 0.94f), FLinearColor(0.85f, 0.9f, 0.95f), 0.2f, 0.7f);
	UMaterialInterface* FeatherTip = YJJShapes::Surface(this, FLinearColor(0.45f, 0.85f, 0.85f), HeroColor::Trim, 0.3f, 0.6f);
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		const FRotator Sweep(0, Side * -14.f, Side * -12.f);
		YJJShapes::Add(this, Glider, EYJJShape::Cube, Sweep.RotateVector(FVector(14, Side * 120.f, 0)) + FVector(0, 0, 26), FVector(8, 236, 7), GoldMat, false, Sweep);
		for (int32 Layer = 0; Layer < 3; ++Layer)
		{
			const float Len = 230.f - Layer * 45.f;
			const FVector Local(-8.f - Layer * 26.f, Side * (Len * 0.5f + 8.f), -Layer * 3.f);
			YJJShapes::Add(this, Glider, EYJJShape::Cube, Sweep.RotateVector(Local) + FVector(0, 0, 20), FVector(34, Len, 4), Feather, false, Sweep + FRotator(Layer * -4.f, 0, 0));
			// 깃털 끝 색
			const FVector TipLocal(-8.f - Layer * 26.f, Side * (Len + 18.f), -Layer * 3.f);
			YJJShapes::Add(this, Glider, EYJJShape::Cube, Sweep.RotateVector(TipLocal) + FVector(0, 0, 20), FVector(34, 36, 4), FeatherTip, false, Sweep + FRotator(Layer * -4.f, 0, 0));
		}
	}
	Glider->SetRelativeScale3D(FVector(0.01f));
	Glider->SetVisibility(false, true);

	// 발밑 그림자 (점프 착지 위치 보기 쉽게)
	DropShadow = YJJShapes::Add(this, RootComponent, EYJJShape::Cylinder, FVector::ZeroVector, FVector(90, 90, 2), YJJShapes::Solid(this, FLinearColor(0.04f, 0.05f, 0.08f)), false);
	DropShadow->SetUsingAbsoluteRotation(true);
	DropShadow->SetCastShadow(false);
}

void AYJJHero::PlayAnim(EAnim Anim, float Rate)
{
	UAnimSequence* Seq = Anims.IsValidIndex((int32)Anim) ? Anims[(int32)Anim].Get() : nullptr;
	if (!Seq || !Body)
	{
		return;
	}
	if (Anim != CurrentAnim)
	{
		const bool bLoop = Anim == EAnim::Idle || Anim == EAnim::Walk || Anim == EAnim::Run || Anim == EAnim::Fall || Anim == EAnim::Glide;
		Body->PlayAnimation(Seq, bLoop);
		CurrentAnim = Anim;
	}
	Body->SetPlayRate(Rate);
}

void AYJJHero::UpdateAnimation()
{
	const UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	const float Speed = GetVelocity().Size2D();
	if (AttackTime >= 0.f)
	{
		const EAnim Attack = AttackCombo == 0 ? EAnim::Attack1 : (AttackCombo == 1 ? EAnim::Attack2 : EAnim::Attack3);
		// 연타로 같은 공격이 다시 나오면 처음부터
		if (Attack == CurrentAnim && AttackTime < 0.02f)
		{
			CurrentAnim = EAnim::Count;
		}
		const float Len = Anims.IsValidIndex((int32)Attack) && Anims[(int32)Attack] ? Anims[(int32)Attack]->GetPlayLength() : AttackDuration;
		PlayAnim(Attack, Len / AttackDuration);
	}
	else if (SinceHurt < 0.4f)
	{
		PlayAnim(EAnim::Hurt, 1.f);
	}
	else if (bGliding)
	{
		PlayAnim(EAnim::Glide, 1.f);
	}
	else if (MoveComp->IsFalling())
	{
		PlayAnim(GetVelocity().Z > 150.f ? EAnim::Jump : EAnim::Fall, 1.f);
	}
	else if (Speed > 760.f)
	{
		PlayAnim(EAnim::Run, FMath::Clamp(Speed / 1050.f, 0.7f, 1.4f));
	}
	else if (Speed > 40.f)
	{
		PlayAnim(EAnim::Walk, FMath::Clamp(Speed / 480.f, 0.5f, 1.6f));
	}
	else
	{
		PlayAnim(EAnim::Idle, 1.f);
	}
}

// ============================================================ 입력

void AYJJHero::BuildInput()
{
	if (MappingContext)
	{
		return;
	}

	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		Action->ValueType = Type;
		return Action;
	};
	MoveAction = MakeAction(TEXT("IA_Move"), EInputActionValueType::Axis2D);
	LookAction = MakeAction(TEXT("IA_Look"), EInputActionValueType::Axis2D);
	JumpAction = MakeAction(TEXT("IA_Jump"), EInputActionValueType::Boolean);
	SprintAction = MakeAction(TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	InteractAction = MakeAction(TEXT("IA_Interact"), EInputActionValueType::Boolean);
	AttackAction = MakeAction(TEXT("IA_Attack"), EInputActionValueType::Boolean);

	MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Hero"));

	// 키 하나를 Axis2D 액션에 매핑: X축 입력을 Y로 돌리거나(bToY) 부호를 뒤집는다(bNegate)
	auto MapAxisKey = [this](UInputAction* Action, const FKey& Key, bool bToY, bool bNegate)
	{
		FEnhancedActionKeyMapping& Mapping = MappingContext->MapKey(Action, Key);
		if (bToY)
		{
			UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(MappingContext);
			Swizzle->Order = EInputAxisSwizzle::YXZ;
			Mapping.Modifiers.Add(Swizzle);
		}
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(MappingContext));
		}
	};

	MapAxisKey(MoveAction, EKeys::W, true, false);
	MapAxisKey(MoveAction, EKeys::S, true, true);
	MapAxisKey(MoveAction, EKeys::D, false, false);
	MapAxisKey(MoveAction, EKeys::A, false, true);
	MappingContext->MapKey(MoveAction, EKeys::Gamepad_Left2D);

	// 마우스/스틱 Y는 위로 올리면 +, 카메라 피치 입력은 아래가 + 이므로 Y만 뒤집는다
	auto MapLookKey = [this](const FKey& Key, float Scale)
	{
		FEnhancedActionKeyMapping& Mapping = MappingContext->MapKey(LookAction, Key);
		UInputModifierNegate* NegateY = NewObject<UInputModifierNegate>(MappingContext);
		NegateY->bX = false;
		NegateY->bY = true;
		NegateY->bZ = false;
		Mapping.Modifiers.Add(NegateY);
		if (Scale != 1.f)
		{
			UInputModifierScalar* Scalar = NewObject<UInputModifierScalar>(MappingContext);
			Scalar->Scalar = FVector(Scale, Scale, 1.f);
			Mapping.Modifiers.Add(Scalar);
		}
	};
	MapLookKey(EKeys::Mouse2D, 1.f);
	MapLookKey(EKeys::Gamepad_Right2D, 2.5f);

	MappingContext->MapKey(JumpAction, EKeys::SpaceBar);
	MappingContext->MapKey(JumpAction, EKeys::Gamepad_FaceButton_Bottom);
	MappingContext->MapKey(SprintAction, EKeys::LeftShift);
	MappingContext->MapKey(SprintAction, EKeys::RightMouseButton);
	MappingContext->MapKey(SprintAction, EKeys::Gamepad_RightTrigger);
	MappingContext->MapKey(InteractAction, EKeys::E);
	MappingContext->MapKey(InteractAction, EKeys::F);
	MappingContext->MapKey(InteractAction, EKeys::Gamepad_FaceButton_Left);
	MappingContext->MapKey(AttackAction, EKeys::LeftMouseButton);
	MappingContext->MapKey(AttackAction, EKeys::Gamepad_FaceButton_Right);
}

void AYJJHero::PawnClientRestart()
{
	Super::PawnClientRestart();
	BuildInput();

	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->ClearAllMappings();
			Subsystem->AddMappingContext(MappingContext, 0);
		}
	}
}

void AYJJHero::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	BuildInput();

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		return;
	}

	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AYJJHero::Move);
	Input->BindAction(MoveAction, ETriggerEvent::Completed, this, &AYJJHero::Move);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AYJJHero::Look);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AYJJHero::JumpPressed);
	Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &AYJJHero::JumpReleased);
	Input->BindAction(SprintAction, ETriggerEvent::Started, this, &AYJJHero::StartSprint);
	Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &AYJJHero::StopSprint);
	Input->BindAction(InteractAction, ETriggerEvent::Started, this, &AYJJHero::InteractPressed);
	Input->BindAction(AttackAction, ETriggerEvent::Started, this, &AYJJHero::AttackPressed);
}

bool AYJJHero::CanAct() const
{
	const UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(this);
	return !bDead && RespawnTimer < 0.f && (!Adv || Adv->GetPhase() != EYJJPhase::Finale);
}

void AYJJHero::Move(const FInputActionValue& Value)
{
	MoveInput = Value.Get<FVector2D>();
	if (!Controller || !CanAct())
	{
		return;
	}

	const FRotationMatrix YawMatrix(FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f));
	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::X), MoveInput.Y);
	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::Y), MoveInput.X);
}

void AYJJHero::Look(const FInputActionValue& Value)
{
	const UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(this);
	if (Adv && Adv->GetPhase() == EYJJPhase::Finale)
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>() * LookSensitivity;
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void AYJJHero::JumpPressed(const FInputActionValue& Value)
{
	if (!CanAct())
	{
		return;
	}
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (MoveComp->IsFalling())
	{
		// 공중에서 누르면 활공을 펴거나 접는다
		SetGliding(!bGliding);
		return;
	}
	Jump();
	YJJAudio::Play(this, EYJJSound::Jump, FVector::ZeroVector, 0.5f, FMath::FRandRange(0.95f, 1.05f));
}

void AYJJHero::JumpReleased(const FInputActionValue& Value)
{
	StopJumping();
}

void AYJJHero::StartSprint(const FInputActionValue& Value)
{
	bSprintHeld = true;
}

void AYJJHero::StopSprint(const FInputActionValue& Value)
{
	bSprintHeld = false;
}

void AYJJHero::InteractPressed(const FInputActionValue& Value)
{
	DoInteract();
}

void AYJJHero::AttackPressed(const FInputActionValue& Value)
{
	DoAttack();
}

void AYJJHero::SetGliding(bool bOn)
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (bOn && (!MoveComp->IsFalling() || Stamina < 1.f || !CanAct()))
	{
		UE_LOG(LogYJJFantasy, Display, TEXT("[Hero] glide refused falling=%d stamina=%.0f canact=%d"), MoveComp->IsFalling(), Stamina, CanAct());
		return;
	}
	if (bGliding == bOn)
	{
		return;
	}
	bGliding = bOn;
	if (bGliding)
	{
		Glider->SetVisibility(true, true);
		YJJAudio::Play(this, EYJJSound::GlideOpen, FVector::ZeroVector, 0.6f);
		// 펼치는 순간 떨어지던 속도를 크게 줄인다 (붕 뜨는 느낌)
		FVector V = MoveComp->Velocity;
		V.Z = FMath::Max(V.Z * 0.2f, -GlideSinkSpeed);
		MoveComp->Velocity = V;
		AttackTime = -1.f;
	}
}

AActor* AYJJHero::FindInteractable(FString* OutPrompt) const
{
	// 범위 안에 여러 개가 있으면 가장 가까운 것 (등불 옆 상자 등)
	const FVector P = GetActorLocation();
	AActor* Best = nullptr;
	float BestDist = TNumericLimits<float>::Max();
	auto Consider = [&](AActor* Actor, const FString& Prompt)
	{
		const float Dist = FVector::DistSquared(P, Actor->GetActorLocation());
		if (Dist < BestDist)
		{
			BestDist = Dist;
			Best = Actor;
			if (OutPrompt)
			{
				*OutPrompt = Prompt;
			}
		}
	};
	for (TActorIterator<AYJJLantern> It(GetWorld()); It; ++It)
	{
		if (It->CanInteract(P))
		{
			Consider(*It, FString::Printf(TEXT("%s 밝히기"), *It->GetLanternName()));
		}
	}
	for (TActorIterator<AYJJChest> It(GetWorld()); It; ++It)
	{
		if (It->CanInteract(P))
		{
			Consider(*It, TEXT("보물 상자 열기"));
		}
	}
	for (TActorIterator<AYJJShrine> It(GetWorld()); It; ++It)
	{
		if (It->CanInteract(P))
		{
			Consider(*It, TEXT("성소에 빛을 되돌리기"));
		}
	}
	return Best;
}

void AYJJHero::DoInteract()
{
	if (!CanAct())
	{
		return;
	}
	AActor* Target = FindInteractable(nullptr);
	if (AYJJLantern* Lantern = Cast<AYJJLantern>(Target))
	{
		Lantern->Interact();
		Health = MaxHealth;
	}
	else if (AYJJChest* Chest = Cast<AYJJChest>(Target))
	{
		Chest->Interact();
	}
	else if (AYJJShrine* Shrine = Cast<AYJJShrine>(Target))
	{
		Shrine->Interact();
	}
}

FString AYJJHero::GetInteractPrompt() const
{
	// 등불 받침 모서리처럼 살짝 걸쳐 있어도(낙하 상태) 안내는 보여준다. 활공 중만 제외
	if (!CanAct() || bGliding)
	{
		return FString();
	}
	FString Prompt;
	FindInteractable(&Prompt);
	return Prompt;
}

void AYJJHero::DoAttack()
{
	if (!CanAct() || bGliding)
	{
		return;
	}
	// 마지막 1/3 구간에선 다음 공격을 이어서 할 수 있다 (콤보)
	if (AttackTime >= 0.f && AttackTime < AttackDuration * 0.65f)
	{
		return;
	}
	AttackCombo = (AttackTime >= 0.f) ? (AttackCombo + 1) % 3 : 0;
	AttackTime = 0.f;
	bAttackHitDone = false;
	Slash->SetVisibility(true);
	YJJAudio::Play(this, EYJJSound::Swing, FVector::ZeroVector, 0.6f, 1.f + AttackCombo * 0.08f);

	// 공격하는 순간 살짝 앞으로 내딛는다
	if (!GetCharacterMovement()->IsFalling())
	{
		LaunchCharacter(GetActorForwardVector() * 450.f, false, false);
	}
}

// ============================================================ 체력·부활

void AYJJHero::TakeHit(const FVector& From)
{
	if (Invulnerable > 0.f || bDead || !CanAct())
	{
		return;
	}
	Health = FMath::Max(0, Health - 1);
	Invulnerable = 1.3f;
	HurtFlash = 1.f;
	SinceHurt = 0.f;
	CameraShake = 1.f;
	SetGliding(false);

	FVector Away = GetActorLocation() - From;
	Away.Z = 0.f;
	// 섬 밖으로 날아가지 않을 만큼만 밀어낸다
	LaunchCharacter(Away.GetSafeNormal() * 420.f + FVector(0, 0, 380.f), true, true);
	YJJAudio::Play(this, EYJJSound::Hurt);
	UE_LOG(LogYJJFantasy, Display, TEXT("[Hero] hit health=%d"), Health);

	if (Health <= 0)
	{
		bDead = true;
		RespawnTimer = 0.f;
	}
}

void AYJJHero::WarpTo(const FVector& Location, float Yaw)
{
	SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorRotation(FRotator(0, Yaw, 0));
	GetCharacterMovement()->Velocity = FVector::ZeroVector;
	if (Controller)
	{
		Controller->SetControlRotation(FRotator(-12.f, Yaw, 0));
	}
	SetGliding(false);
}

void AYJJHero::Respawn()
{
	if (const UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(this))
	{
		WarpTo(Adv->GetCheckpoint(), Adv->GetCheckpointYaw());
	}
	if (bDead)
	{
		Health = MaxHealth;
	}
	bDead = false;
	Invulnerable = 1.5f;
	Stamina = GetMaxStamina();
	bExhausted = false;
	YJJAudio::Play(this, EYJJSound::Respawn, FVector::ZeroVector, 0.7f);
	UE_LOG(LogYJJFantasy, Display, TEXT("[Hero] respawn"));
}

void AYJJHero::UpdateRespawn(float DeltaTime)
{
	// 하늘 아래로 떨어지면 암전 후 체크포인트로
	if (RespawnTimer < 0.f && GetActorLocation().Z < KillZ)
	{
		RespawnTimer = 0.f;
		UE_LOG(LogYJJFantasy, Display, TEXT("[Hero] fell z=%.0f"), GetActorLocation().Z);
	}

	if (RespawnTimer >= 0.f)
	{
		RespawnTimer += DeltaTime;
		ScreenFade = FMath::Min(1.f, RespawnTimer / 0.45f);
		if (RespawnTimer >= 0.6f)
		{
			RespawnTimer = -1.f;
			Respawn();
		}
	}
	else
	{
		ScreenFade = FMath::Max(0.f, ScreenFade - DeltaTime / 0.6f);
	}
}

// ============================================================ 매 프레임

float AYJJHero::GetMaxStamina() const
{
	const UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(this);
	return BaseStamina + (Adv ? Adv->GetStaminaBonus() : 0.f);
}

bool AYJJHero::IsInUpdraft() const
{
	const FVector P = GetActorLocation();
	for (TActorIterator<AYJJUpdraft> It(GetWorld()); It; ++It)
	{
		if (It->IsInside(P))
		{
			return true;
		}
	}
	return false;
}

void AYJJHero::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Time += DeltaTime;

	UpdateRespawn(DeltaTime);
	UpdateMovement(DeltaTime);
	UpdateStamina(DeltaTime);
	UpdateAttack(DeltaTime);
	UpdateFinale(DeltaTime);
	UpdateBody(DeltaTime);
	UpdateCamera(DeltaTime);

	Invulnerable = FMath::Max(0.f, Invulnerable - DeltaTime);
	HurtFlash = FMath::Max(0.f, HurtFlash - DeltaTime * 2.5f);
	SinceHurt += DeltaTime;
	// 한동안 안 맞으면 체력이 천천히 찬다
	if (!bDead && Health < MaxHealth && SinceHurt > 8.f)
	{
		Health++;
		SinceHurt = 4.f;
	}
}

void AYJJHero::UpdateMovement(float DeltaTime)
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	const bool bFalling = MoveComp->IsFalling();
	const bool bInUpdraft = IsInUpdraft();

	if (!bFalling && bGliding)
	{
		SetGliding(false);
	}

	if (bInUpdraft && !bWasInUpdraft)
	{
		YJJAudio::Play(this, EYJJSound::Updraft, FVector::ZeroVector, 0.6f);
	}
	bWasInUpdraft = bInUpdraft;

	if (bGliding)
	{
		MoveComp->GravityScale = 0.f;
		MoveComp->AirControl = 1.f;
		MoveComp->MaxWalkSpeed = GlideSpeed;
		FVector V = MoveComp->Velocity;
		const float TargetZ = bInUpdraft ? UpdraftSpeed : -GlideSinkSpeed;
		V.Z = FMath::FInterpTo(V.Z, TargetZ, DeltaTime, bInUpdraft ? 2.5f : 4.f);
		MoveComp->Velocity = V;

		// 입력이 없어도 앞으로 미끄러진다
		if (MoveInput.IsNearlyZero() && !bInUpdraft)
		{
			AddMovementInput(GetActorForwardVector(), 0.45f);
		}
	}
	else
	{
		MoveComp->GravityScale = 1.75f;
		MoveComp->AirControl = 0.55f;
		bSprinting = bSprintHeld && !bExhausted && !bFalling && Stamina > 0.f && GetVelocity().Size2D() > 50.f;
		MoveComp->MaxWalkSpeed = bSprinting ? SprintSpeed : WalkSpeed;

		// 활공 없이 상승 기류에 들어가도 조금 떠오른다 (활공하라는 힌트)
		if (bInUpdraft && bFalling && MoveComp->Velocity.Z < 300.f)
		{
			MoveComp->Velocity.Z = FMath::FInterpTo(MoveComp->Velocity.Z, 300.f, DeltaTime, 3.f);
		}
	}
}

void AYJJHero::UpdateStamina(float DeltaTime)
{
	const float MaxStamina = GetMaxStamina();
	float Cost = 0.f;
	if (bGliding && !bWasInUpdraft)
	{
		Cost = StaminaGlideCost;
	}
	else if (bSprinting)
	{
		Cost = StaminaSprintCost;
	}

	if (Cost > 0.f)
	{
		Stamina = FMath::Max(0.f, Stamina - Cost * DeltaTime);
		StaminaRegenDelay = 0.6f;
		if (Stamina <= 0.f)
		{
			bExhausted = true;
			SetGliding(false);
		}
	}
	else if (!GetCharacterMovement()->IsFalling() || bWasInUpdraft)
	{
		StaminaRegenDelay -= DeltaTime;
		if (StaminaRegenDelay <= 0.f)
		{
			Stamina = FMath::Min(MaxStamina, Stamina + StaminaRegen * DeltaTime);
		}
	}
	if (bExhausted && Stamina > MaxStamina * 0.3f)
	{
		bExhausted = false;
	}
}

void AYJJHero::UpdateAttack(float DeltaTime)
{
	if (AttackTime < 0.f)
	{
		return;
	}
	AttackTime += DeltaTime;
	const float A = AttackTime / AttackDuration;

	// 베기 궤적: 휘두르는 방향으로 돌면서 사라진다
	const float Dir = (AttackCombo == 1) ? -1.f : 1.f;
	Slash->SetRelativeRotation(FRotator(AttackCombo == 2 ? 70.f : 8.f * Dir, Dir * (-80.f + 160.f * FMath::Min(A * 1.6f, 1.f)), 0));
	Slash->SetRelativeScale3D(FVector(3.8f, 3.8f * FMath::Sin(FMath::Min(A * 1.6f, 1.f) * PI) + 0.01f, 0.04f));
	SlashMID->SetScalarParameterValue(TEXT("Intensity"), FMath::Max(0.f, 2.5f * (1.f - A * 1.3f)));

	if (!bAttackHitDone && A > 0.25f)
	{
		bAttackHitDone = true;
		const FVector P = GetActorLocation();
		const FVector Fwd = GetActorForwardVector();
		bool bHit = false;
		for (TActorIterator<AYJJSlime> It(GetWorld()); It; ++It)
		{
			if (!It->IsAlive())
			{
				continue;
			}
			const FVector To = It->GetActorLocation() + FVector(0, 0, 50) - P;
			if (To.Size() < AttackRange + It->GetRadius() && FVector::DotProduct(To.GetSafeNormal2D(), Fwd) > 0.1f && FMath::Abs(To.Z) < 200.f)
			{
				It->TakeHit(P);
				bHit = true;
			}
		}
		if (bHit)
		{
			CameraShake = FMath::Max(CameraShake, 0.5f);
		}
	}

	if (A >= 1.f)
	{
		AttackTime = -1.f;
		Slash->SetVisibility(false);
	}
}

void AYJJHero::UpdateFinale(float DeltaTime)
{
	const UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(this);
	if (!Adv || Adv->GetPhase() != EYJJPhase::Finale || !Controller)
	{
		return;
	}
	const AYJJShrine* Shrine = Adv->GetShrine();
	if (!Shrine)
	{
		return;
	}
	// 카메라를 천천히 수정 쪽으로 올려다보게
	const FVector Eye = GetActorLocation() + FVector(0, 0, 70);
	// 카메라는 영웅 뒤에 있으므로, 컨트롤 방향을 수정 쪽으로 돌리면 영웅 너머로 수정이 보인다
	const FVector Focus = Shrine->GetCrystalLocation() + FVector(0, 0, 500.f * FMath::Min(Adv->GetPhaseTime() / 6.f, 1.f));
	const FRotator Look = (Focus - Eye).Rotation();
	const FRotator Target(FMath::Clamp(Look.Pitch, -5.f, 22.f), Look.Yaw, 0.f);
	Controller->SetControlRotation(FMath::RInterpTo(Controller->GetControlRotation(), Target, DeltaTime, 1.5f));
}

void AYJJHero::UpdateBody(float DeltaTime)
{
	if (!Visual)
	{
		return;
	}
	const UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	const FVector V = GetVelocity();
	const float Speed = V.Size2D();
	const float SpeedRatio = FMath::Clamp(Speed / SprintSpeed, 0.f, 1.f);
	const bool bFalling = MoveComp->IsFalling();

	GlideBlend = FMath::FInterpTo(GlideBlend, bGliding ? 1.f : 0.f, DeltaTime, 7.f);
	if (!bGliding && GlideBlend < 0.02f && Glider->IsVisible())
	{
		Glider->SetVisibility(false, true);
	}
	Glider->SetRelativeScale3D(FVector(FMath::Max(GlideBlend, 0.01f)));

	UpdateAnimation();

	// 회전 속도로 몸을 기울인다 (팔다리 동작은 애니메이션이 한다)
	const float Yaw = GetActorRotation().Yaw;
	const float YawRate = FMath::FindDeltaAngleDegrees(LastYaw, Yaw) / FMath::Max(DeltaTime, 0.001f);
	LastYaw = Yaw;
	LeanRoll = FMath::FInterpTo(LeanRoll, FMath::Clamp(YawRate * 0.03f * SpeedRatio, -18.f, 18.f) + (bGliding ? FMath::Clamp(YawRate * 0.05f, -30.f, 30.f) : 0.f), DeltaTime, 6.f);
	// 활공 땐 몸을 앞으로 눕혀 날개에 매달린 모양
	const float TargetPitch = bGliding ? -35.f : 0.f;
	LeanPitch = FMath::FInterpTo(LeanPitch, TargetPitch, DeltaTime, 6.f);

	// 착지하면 찌부러졌다가 통 튀어 오른다 (고무공 느낌)
	LandSquash = FMath::FInterpTo(LandSquash, 0.f, DeltaTime, 9.f);
	Visual->SetRelativeLocation(FVector(0, 0, -96.f - LandSquash * 10.f));
	Visual->SetRelativeRotation(FRotator(LeanPitch, 0.f, -LeanRoll));
	Visual->SetRelativeScale3D(FVector(1.f + LandSquash * 0.22f, 1.f + LandSquash * 0.22f, 1.f - LandSquash * 0.28f));

	// 검: 공격 중엔 오른손(hand_r)에서 팔 방향으로 뻗고, 아니면 등에
	if (AttackTime >= 0.f && Body && Body->GetBoneIndex(TEXT("hand_r")) != INDEX_NONE)
	{
		const FVector Hand = Body->GetBoneLocation(TEXT("hand_r"));
		const FVector Shoulder = Body->GetBoneLocation(TEXT("arm_r"));
		const FVector Dir = (Hand - Shoulder).GetSafeNormal();
		SwordPivot->SetWorldLocationAndRotation(Hand, FRotationMatrix::MakeFromZX(Dir, GetActorForwardVector()).Rotator());
	}
	else
	{
		SwordPivot->SetRelativeLocation(FVector(-34.f, 0.f, 40.f));
		SwordPivot->SetRelativeRotation(FRotator(0.f, 0.f, 35.f));
	}

	// 다쳐서 무적일 땐 깜빡인다
	const bool bVisible = Invulnerable <= 0.f || FMath::Fmod(Time, 0.14f) > 0.05f;
	Visual->SetVisibility(bVisible, true);
	if (Slash->IsVisible() != (AttackTime >= 0.f))
	{
		Slash->SetVisibility(AttackTime >= 0.f);
	}
	if (!Glider->IsVisible() && GlideBlend > 0.02f)
	{
		Glider->SetVisibility(true, true);
	}

	// 발밑 그림자
	FHitResult Hit;
	FCollisionQueryParams Params(TEXT("HeroShadow"), false, this);
	const FVector P = GetActorLocation();
	if (GetWorld()->LineTraceSingleByObjectType(Hit, P, P - FVector(0, 0, 3000), FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		const float H = P.Z - 96.f - Hit.ImpactPoint.Z;
		const float S = FMath::Clamp(1.f - H / 1500.f, 0.3f, 1.f);
		// 땅에 서 있을 땐 진짜 그림자가 있으니 공중에서만 착지 위치 표시
		DropShadow->SetVisibility(H > 60.f);
		DropShadow->SetWorldLocation(Hit.ImpactPoint + FVector(0, 0, 2));
		DropShadow->SetWorldScale3D(FVector(0.9f * S, 0.9f * S, 0.02f));
	}
	else
	{
		DropShadow->SetVisibility(false);
	}
}

void AYJJHero::UpdateCamera(float DeltaTime)
{
	const float Speed = GetVelocity().Size2D();
	const UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(this);
	const bool bFinale = Adv && Adv->GetPhase() == EYJJPhase::Finale;

	float TargetArm = 430.f;
	float TargetFov = 80.f;
	if (bGliding)
	{
		TargetArm = 560.f;
		TargetFov = 92.f;
	}
	else if (bSprinting)
	{
		TargetArm = 470.f;
		TargetFov = 88.f;
	}
	if (bFinale)
	{
		TargetArm = 1500.f;
		TargetFov = 75.f;
	}
	// 엔딩 땐 기둥에 카메라가 막혀 영웅 등만 보이지 않게
	CameraBoom->bDoCollisionTest = !bFinale;
	TargetFov += FMath::Clamp((Speed - 900.f) / 600.f, 0.f, 1.f) * 4.f;

	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArm, DeltaTime, 3.f);
	FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, TargetFov, DeltaTime, 4.f));

	CameraShake = FMath::Max(0.f, CameraShake - DeltaTime * 4.f);
	const float Shake = CameraShake * CameraShake * 10.f;
	FollowCamera->SetRelativeLocation(FVector(0.f, FMath::Sin(Time * 61.f) * Shake, FMath::Sin(Time * 47.f) * Shake));
}

void AYJJHero::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	const float FallSpeed = -GetCharacterMovement()->Velocity.Z;
	LandSquash = FMath::Clamp(FallSpeed / 1400.f, 0.2f, 1.f);
	SetGliding(false);
	YJJAudio::Play(this, EYJJSound::Land, FVector::ZeroVector, FMath::Clamp(FallSpeed / 1500.f, 0.2f, 0.8f));
}
