#include "Player/YJJHero.h"

#include "Data/YJJLoc.h"
#include "Data/YJJData.h"
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

USceneComponent* AYJJHero::MakePivot(AActor* Owner, USceneComponent* Parent, const FVector& Location)
{
	USceneComponent* Pivot = NewObject<USceneComponent>(Owner);
	Pivot->SetMobility(EComponentMobility::Movable);
	Pivot->SetupAttachment(Parent);
	Pivot->SetRelativeLocation(Location);
	Pivot->RegisterComponent();
	Owner->AddInstanceComponent(Pivot);
	return Pivot;
}

AYJJHero::AYJJHero()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(CapsuleRadius, CapsuleHalfHeight);

	// 카메라 방향으로 몸을 돌리지 않고, 이동 방향으로 몸을 돌린다 (모험 게임 기본 조작감)
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->bOrientRotationToMovement = true;
	MoveComp->bCanWalkOffLedgesWhenCrouching = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SocketOffset = CameraSocketOffset;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->ProbeSize = CameraProbeSize;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void AYJJHero::BeginPlay()
{
	Super::BeginPlay();
	ApplyTuning();
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

void AYJJHero::ApplyTuning()
{
	// Content/YJJFantasy/Data/Tuning.csv 의 Hero.* (게임 중 yjj.ReloadData 후 다시 시작하면 반영)
	WalkSpeed = FYJJData::Tune(TEXT("Hero.WalkSpeed"));
	SprintSpeed = FYJJData::Tune(TEXT("Hero.SprintSpeed"));
	GlideSpeed = FYJJData::Tune(TEXT("Hero.GlideSpeed"));
	GlideSinkSpeed = FYJJData::Tune(TEXT("Hero.GlideSinkSpeed"));
	UpdraftSpeed = FYJJData::Tune(TEXT("Hero.UpdraftSpeed"));
	BaseStamina = FYJJData::Tune(TEXT("Hero.BaseStamina"));
	LookSensitivity = FYJJData::Tune(TEXT("Hero.LookSensitivity"));
	MaxHealth = FMath::RoundToInt(FYJJData::Tune(TEXT("Hero.MaxHealth")));
	Health = MaxHealth;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->RotationRate = FRotator(0.f, FYJJData::Tune(TEXT("Hero.TurnRate")), 0.f);
	MoveComp->MaxWalkSpeed = WalkSpeed;
	MoveComp->MaxAcceleration = FYJJData::Tune(TEXT("Hero.Acceleration"));
	MoveComp->JumpZVelocity = FYJJData::Tune(TEXT("Hero.JumpZ"));
	MoveComp->AirControl = FYJJData::Tune(TEXT("Hero.AirControl"));
	MoveComp->GravityScale = FYJJData::Tune(TEXT("Hero.Gravity"));
	MoveComp->BrakingDecelerationWalking = FYJJData::Tune(TEXT("Hero.BrakingWalk"));
	MoveComp->BrakingDecelerationFalling = FYJJData::Tune(TEXT("Hero.BrakingFall"));
	MoveComp->MaxStepHeight = FYJJData::Tune(TEXT("Hero.MaxStepHeight"));
	MoveComp->SetWalkableFloorAngle(FYJJData::Tune(TEXT("Hero.WalkableFloorAngle")));
	CameraBoom->TargetArmLength = FYJJData::Tune(TEXT("Hero.CameraArm"));
	CameraBoom->CameraLagSpeed = FYJJData::Tune(TEXT("Hero.CameraLag"));
	FollowCamera->FieldOfView = FYJJData::Tune(TEXT("Hero.CameraFov"));
}

// ============================================================ 몸 조립

void AYJJHero::BuildBody()
{
	UMaterialInterface* LeatherMat = YJJShapes::Solid(this, FYJJData::Color(TEXT("HeroLeather")), 0.f, Sword.LeatherRoughness);
	UMaterialInterface* GoldMat = YJJShapes::Solid(this, FYJJData::Color(TEXT("HeroGold")), Sword.GoldEmissive, Sword.GoldRoughness);

	// 발바닥 = 캡슐 아래 (-96). 기울기·찌그러짐은 이 축에 준다
	Visual = MakePivot(this, RootComponent, FVector(0, 0, -CapsuleHalfHeight));

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
	SwordPivot = MakePivot(this, Visual, Sword.BackSpot);
	SwordPivot->SetRelativeScale3D(FVector(Sword.Scale));
	UMaterialInterface* Blade = YJJShapes::Surface(this, FYJJData::Color(TEXT("HeroBlade")), FYJJData::Color(TEXT("HeroBladeSide")), Sword.BladeEmissive, Sword.BladeRoughness);
	YJJShapes::Add(this, SwordPivot, EYJJShape::Cube, FVector(0, 0, Sword.BladeZ), Sword.BladeSize, Blade, false);
	YJJShapes::Add(this, SwordPivot, EYJJShape::Cube, FVector(0, 0, Sword.TipZ), Sword.TipSize, Blade, false, FRotator(0, 0, Sword.TipRoll));
	YJJShapes::Add(this, SwordPivot, EYJJShape::Cube, FVector(0, 0, Sword.GuardZ), Sword.GuardSize, GoldMat, false);
	YJJShapes::Add(this, SwordPivot, EYJJShape::Cylinder, FVector(0, 0, Sword.GripZ), Sword.GripSize, LeatherMat, false);
	YJJShapes::Add(this, SwordPivot, EYJJShape::Sphere, FVector(0, 0, Sword.PommelZ), FVector(Sword.PommelSize), YJJShapes::Solid(this, FYJJData::Color(TEXT("HeroPommel")), Sword.PommelGlow), false);

	// 베기 궤적 (납작한 빛 원판)
	SlashMID = YJJShapes::Glow(this, FYJJData::Color(TEXT("HeroSlash")), 0.f, 0.f, SlashFx.Rim);
	Slash = YJJShapes::Add(this, Visual, EYJJShape::Cylinder, SlashFx.Spot, FVector(SlashFx.Diameter, SlashFx.Diameter, SlashFx.Thickness), SlashMID, false);
	Slash->SetVisibility(false);

	// 활공 날개: 머리 위에서 펼쳐져 두 앞발로 붙잡는다
	Glider = MakePivot(this, Visual, Wing.Spot);
	// 깃털 세 겹 날개: 앞쪽이 길고 뒤로 갈수록 짧아진다. 끝은 청록색
	UMaterialInterface* Feather = YJJShapes::Surface(this, FYJJData::Color(TEXT("HeroFeather")), FYJJData::Color(TEXT("HeroFeatherSide")), Wing.FeatherEmissive, Wing.FeatherRoughness);
	UMaterialInterface* FeatherTip = YJJShapes::Surface(this, FYJJData::Color(TEXT("HeroFeatherTip")), FYJJData::Color(TEXT("HeroTrim")), Wing.TipEmissive, Wing.TipRoughness);
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		const FRotator Sweep(0, Side * -Wing.SweepYaw, Side * -Wing.SweepRoll);
		YJJShapes::Add(this, Glider, EYJJShape::Cube, Sweep.RotateVector(FVector(Wing.BarForward, Side * Wing.BarHalfSpan, 0)) + FVector(0, 0, Wing.BarLift), Wing.BarSize, GoldMat, false, Sweep);
		for (int32 Layer = 0; Layer < Wing.Layers; ++Layer)
		{
			const float Len = Wing.FeatherLength - Layer * Wing.LayerShorten;
			const FVector Local(-Wing.FeatherBack - Layer * Wing.LayerBack, Side * (Len * 0.5f + Wing.FeatherBack), -Layer * Wing.LayerDrop);
			YJJShapes::Add(this, Glider, EYJJShape::Cube, Sweep.RotateVector(Local) + FVector(0, 0, Wing.FeatherLift), FVector(Wing.FeatherWidth, Len, Wing.FeatherThickness), Feather, false, Sweep + FRotator(Layer * -Wing.LayerTilt, 0, 0));
			// 깃털 끝 색
			const FVector TipLocal(-Wing.FeatherBack - Layer * Wing.LayerBack, Side * (Len + Wing.TipLength * 0.5f), -Layer * Wing.LayerDrop);
			YJJShapes::Add(this, Glider, EYJJShape::Cube, Sweep.RotateVector(TipLocal) + FVector(0, 0, Wing.FeatherLift), FVector(Wing.FeatherWidth, Wing.TipLength, Wing.FeatherThickness), FeatherTip, false, Sweep + FRotator(Layer * -Wing.LayerTilt, 0, 0));
		}
	}
	Glider->SetRelativeScale3D(FVector(Wing.FoldedScale));
	Glider->SetVisibility(false, true);

	// 발밑 그림자 (점프 착지 위치 보기 쉽게)
	DropShadow = YJJShapes::Add(this, RootComponent, EYJJShape::Cylinder, FVector::ZeroVector, FVector(Shadow.Diameter, Shadow.Diameter, Shadow.Thickness), YJJShapes::Solid(this, FYJJData::Color(TEXT("HeroShadow"))), false);
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
		if (Attack == CurrentAnim && AttackTime < AnimPick.RestartWindow)
		{
			CurrentAnim = EAnim::Count;
		}
		const float Len = Anims.IsValidIndex((int32)Attack) && Anims[(int32)Attack] ? Anims[(int32)Attack]->GetPlayLength() : FYJJData::Tune(TEXT("Hero.AttackDuration"));
		PlayAnim(Attack, Len / FYJJData::Tune(TEXT("Hero.AttackDuration")));
	}
	else if (SinceHurt < AnimPick.HurtTime)
	{
		PlayAnim(EAnim::Hurt, 1.f);
	}
	else if (bGliding)
	{
		PlayAnim(EAnim::Glide, 1.f);
	}
	else if (MoveComp->IsFalling())
	{
		PlayAnim(GetVelocity().Z > AnimPick.JumpRiseSpeed ? EAnim::Jump : EAnim::Fall, 1.f);
	}
	else if (Speed > AnimPick.RunFromSpeed)
	{
		PlayAnim(EAnim::Run, FMath::Clamp(Speed / AnimPick.RunNaturalSpeed, AnimPick.RunRateMin, AnimPick.RunRateMax));
	}
	else if (Speed > AnimPick.WalkFromSpeed)
	{
		PlayAnim(EAnim::Walk, FMath::Clamp(Speed / AnimPick.WalkNaturalSpeed, AnimPick.WalkRateMin, AnimPick.WalkRateMax));
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
	MapLookKey(EKeys::Gamepad_Right2D, GamepadLookScale);

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
	YJJAudio::Play(this, EYJJSound::Jump, FVector::ZeroVector, FYJJData::Tune(TEXT("Hero.JumpVolume")), FMath::FRandRange(Sound.PitchMin, Sound.PitchMax));
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
		YJJAudio::Play(this, EYJJSound::GlideOpen, FVector::ZeroVector, FYJJData::Tune(TEXT("Hero.GlideVolume")));
		// 펼치는 순간 떨어지던 속도를 크게 줄인다 (붕 뜨는 느낌)
		FVector V = MoveComp->Velocity;
		V.Z = FMath::Max(V.Z * FYJJData::Tune(TEXT("Hero.GlideOpenKeep")), -GlideSinkSpeed);
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
			Consider(*It, FYJJLoc::Format(TEXT("Prompt.LightLantern"), { It->GetLanternName() }));
		}
	}
	for (TActorIterator<AYJJChest> It(GetWorld()); It; ++It)
	{
		if (It->CanInteract(P))
		{
			Consider(*It, FYJJLoc::Get(TEXT("Prompt.OpenChest")));
		}
	}
	for (TActorIterator<AYJJShrine> It(GetWorld()); It; ++It)
	{
		if (It->CanInteract(P))
		{
			Consider(*It, FYJJLoc::Get(TEXT("Prompt.Shrine")));
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
	if (AttackTime >= 0.f && AttackTime < FYJJData::Tune(TEXT("Hero.AttackDuration")) * FYJJData::Tune(TEXT("Hero.ComboLock")))
	{
		return;
	}
	AttackCombo = (AttackTime >= 0.f) ? (AttackCombo + 1) % 3 : 0;
	AttackTime = 0.f;
	bAttackHitDone = false;
	Slash->SetVisibility(true);
	YJJAudio::Play(this, EYJJSound::Swing, FVector::ZeroVector, FYJJData::Tune(TEXT("Hero.SwingVolume")), 1.f + AttackCombo * Sound.ComboPitchStep);

	// 공격하는 순간 살짝 앞으로 내딛는다
	if (!GetCharacterMovement()->IsFalling())
	{
		LaunchCharacter(GetActorForwardVector() * FYJJData::Tune(TEXT("Hero.AttackLunge")), false, false);
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
	Invulnerable = FYJJData::Tune(TEXT("Hero.InvulnerableAfterHit"));
	HurtFlash = 1.f;
	SinceHurt = 0.f;
	CameraShake = 1.f;
	SetGliding(false);

	FVector Away = GetActorLocation() - From;
	Away.Z = 0.f;
	// 섬 밖으로 날아가지 않을 만큼만 밀어낸다
	LaunchCharacter(Away.GetSafeNormal() * FYJJData::Tune(TEXT("Hero.KnockbackSide")) + FVector(0, 0, FYJJData::Tune(TEXT("Hero.KnockbackUp"))), true, true);
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
		Controller->SetControlRotation(FRotator(-WarpLookDown, Yaw, 0));
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
	Invulnerable = FYJJData::Tune(TEXT("Hero.InvulnerableAfterRespawn"));
	Stamina = GetMaxStamina();
	bExhausted = false;
	YJJAudio::Play(this, EYJJSound::Respawn, FVector::ZeroVector, FYJJData::Tune(TEXT("Hero.RespawnVolume")));
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
		ScreenFade = FMath::Min(1.f, RespawnTimer / FYJJData::Tune(TEXT("Hero.RespawnFadeOut")));
		if (RespawnTimer >= FYJJData::Tune(TEXT("Hero.RespawnDelay")))
		{
			RespawnTimer = -1.f;
			Respawn();
		}
	}
	else
	{
		ScreenFade = FMath::Max(0.f, ScreenFade - DeltaTime / FYJJData::Tune(TEXT("Hero.RespawnFadeIn")));
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
	HurtFlash = FMath::Max(0.f, HurtFlash - DeltaTime * HurtFlashFade);
	SinceHurt += DeltaTime;
	// 한동안 안 맞으면 체력이 천천히 찬다
	if (!bDead && Health < MaxHealth && SinceHurt > FYJJData::Tune(TEXT("Hero.RegenDelay")))
	{
		Health++;
		SinceHurt = FYJJData::Tune(TEXT("Hero.RegenDelay")) - FYJJData::Tune(TEXT("Hero.RegenInterval"));
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
		YJJAudio::Play(this, EYJJSound::Updraft, FVector::ZeroVector, FYJJData::Tune(TEXT("Hero.UpdraftVolume")));
	}
	bWasInUpdraft = bInUpdraft;

	if (bGliding)
	{
		MoveComp->GravityScale = 0.f;
		MoveComp->AirControl = FYJJData::Tune(TEXT("Hero.GlideAirControl"));
		MoveComp->MaxWalkSpeed = GlideSpeed;
		FVector V = MoveComp->Velocity;
		const float TargetZ = bInUpdraft ? UpdraftSpeed : -GlideSinkSpeed;
		V.Z = FMath::FInterpTo(V.Z, TargetZ, DeltaTime, bInUpdraft ? FYJJData::Tune(TEXT("Hero.UpdraftBlend")) : FYJJData::Tune(TEXT("Hero.GlideSinkBlend")));
		MoveComp->Velocity = V;

		// 입력이 없어도 앞으로 미끄러진다
		if (MoveInput.IsNearlyZero() && !bInUpdraft)
		{
			AddMovementInput(GetActorForwardVector(), FYJJData::Tune(TEXT("Hero.GlideAutoForward")));
		}
	}
	else
	{
		MoveComp->GravityScale = FYJJData::Tune(TEXT("Hero.Gravity"));
		MoveComp->AirControl = FYJJData::Tune(TEXT("Hero.AirControl"));
		bSprinting = bSprintHeld && !bExhausted && !bFalling && Stamina > 0.f && GetVelocity().Size2D() > FYJJData::Tune(TEXT("Hero.SprintMinSpeed"));
		MoveComp->MaxWalkSpeed = bSprinting ? SprintSpeed : WalkSpeed;

		// 활공 없이 상승 기류에 들어가도 조금 떠오른다 (활공하라는 힌트)
		if (bInUpdraft && bFalling && MoveComp->Velocity.Z < FYJJData::Tune(TEXT("Hero.UpdraftNudgeSpeed")))
		{
			MoveComp->Velocity.Z = FMath::FInterpTo(MoveComp->Velocity.Z, FYJJData::Tune(TEXT("Hero.UpdraftNudgeSpeed")), DeltaTime, FYJJData::Tune(TEXT("Hero.UpdraftNudgeBlend")));
		}
	}
}

void AYJJHero::UpdateStamina(float DeltaTime)
{
	const float MaxStamina = GetMaxStamina();
	float Cost = 0.f;
	if (bGliding && !bWasInUpdraft)
	{
		Cost = FYJJData::Tune(TEXT("Hero.StaminaGlideCost"));
	}
	else if (bSprinting)
	{
		Cost = FYJJData::Tune(TEXT("Hero.StaminaSprintCost"));
	}

	if (Cost > 0.f)
	{
		Stamina = FMath::Max(0.f, Stamina - Cost * DeltaTime);
		StaminaRegenDelay = FYJJData::Tune(TEXT("Hero.StaminaRegenDelay"));
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
			Stamina = FMath::Min(MaxStamina, Stamina + FYJJData::Tune(TEXT("Hero.StaminaRegen")) * DeltaTime);
		}
	}
	if (bExhausted && Stamina > MaxStamina * FYJJData::Tune(TEXT("Hero.ExhaustRecoverRatio")))
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
	const float A = AttackTime / FYJJData::Tune(TEXT("Hero.AttackDuration"));

	// 베기 궤적: 휘두르는 방향으로 돌면서 사라진다
	const float Dir = (AttackCombo == 1) ? -1.f : 1.f;
	const float Sweep = FMath::Min(A * SlashFx.SweepSpeed, 1.f);
	Slash->SetRelativeRotation(FRotator(AttackCombo == FinisherCombo ? SlashFx.FinisherPitch : SlashFx.Pitch * Dir, Dir * (-SlashFx.SweepHalf + SlashFx.SweepHalf * 2.f * Sweep), 0));
	Slash->SetRelativeScale3D(FVector(SlashFx.Scale, SlashFx.Scale * FMath::Sin(Sweep * PI) + SlashFx.MinWidth, SlashFx.Flat));
	SlashMID->SetScalarParameterValue(TEXT("Intensity"), FMath::Max(0.f, SlashFx.Glow * (1.f - A * SlashFx.FadeSpeed)));

	if (!bAttackHitDone && A > FYJJData::Tune(TEXT("Hero.AttackHitMoment")))
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
			const FVector To = It->GetActorLocation() + FVector(0, 0, FYJJData::Tune(TEXT("Hero.AttackAimHeight"))) - P;
			if (To.Size() < FYJJData::Tune(TEXT("Hero.AttackRange")) + It->GetRadius() && FVector::DotProduct(To.GetSafeNormal2D(), Fwd) > FYJJData::Tune(TEXT("Hero.AttackCone")) && FMath::Abs(To.Z) < FYJJData::Tune(TEXT("Hero.AttackHeight")))
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
	const FVector Eye = GetActorLocation() + FVector(0, 0, Finale.EyeHeight);
	// 카메라는 영웅 뒤에 있으므로, 컨트롤 방향을 수정 쪽으로 돌리면 영웅 너머로 수정이 보인다
	const FVector Focus = Shrine->GetCrystalLocation() + FVector(0, 0, Finale.LookRise * FMath::Min(Adv->GetPhaseTime() / Finale.LookRiseTime, 1.f));
	const FRotator Look = (Focus - Eye).Rotation();
	const FRotator Target(FMath::Clamp(Look.Pitch, Finale.PitchMin, Finale.PitchMax), Look.Yaw, 0.f);
	Controller->SetControlRotation(FMath::RInterpTo(Controller->GetControlRotation(), Target, DeltaTime, Finale.TurnSpeed));
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

	GlideBlend = FMath::FInterpTo(GlideBlend, bGliding ? 1.f : 0.f, DeltaTime, Wing.OpenSpeed);
	if (!bGliding && GlideBlend < Wing.HiddenBelow && Glider->IsVisible())
	{
		Glider->SetVisibility(false, true);
	}
	Glider->SetRelativeScale3D(FVector(FMath::Max(GlideBlend, Wing.FoldedScale)));

	UpdateAnimation();

	// 회전 속도로 몸을 기울인다 (팔다리 동작은 애니메이션이 한다)
	const float Yaw = GetActorRotation().Yaw;
	const float YawRate = FMath::FindDeltaAngleDegrees(LastYaw, Yaw) / FMath::Max(DeltaTime, MinDeltaTime);
	LastYaw = Yaw;
	LeanRoll = FMath::FInterpTo(LeanRoll, FMath::Clamp(YawRate * Lean.RunPerYaw * SpeedRatio, -Lean.RunMax, Lean.RunMax) + (bGliding ? FMath::Clamp(YawRate * Lean.GlidePerYaw, -Lean.GlideMax, Lean.GlideMax) : 0.f), DeltaTime, Lean.Speed);
	// 활공 땐 몸을 앞으로 눕혀 날개에 매달린 모양
	const float TargetPitch = bGliding ? -Lean.GlidePitch : 0.f;
	LeanPitch = FMath::FInterpTo(LeanPitch, TargetPitch, DeltaTime, Lean.Speed);

	// 착지하면 찌부러졌다가 통 튀어 오른다 (고무공 느낌)
	LandSquash = FMath::FInterpTo(LandSquash, 0.f, DeltaTime, Squash.Recover);
	Visual->SetRelativeLocation(FVector(0, 0, -CapsuleHalfHeight - LandSquash * Squash.Sink));
	Visual->SetRelativeRotation(FRotator(LeanPitch, 0.f, -LeanRoll));
	Visual->SetRelativeScale3D(FVector(1.f + LandSquash * Squash.Widen, 1.f + LandSquash * Squash.Widen, 1.f - LandSquash * Squash.Flatten));

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
		SwordPivot->SetRelativeLocation(Sword.RestSpot);
		SwordPivot->SetRelativeRotation(FRotator(0.f, 0.f, Sword.RestRoll));
	}

	// 다쳐서 무적일 땐 깜빡인다
	const bool bVisible = Invulnerable <= 0.f || FMath::Fmod(Time, BlinkPeriod) > BlinkOff;
	Visual->SetVisibility(bVisible, true);
	if (Slash->IsVisible() != (AttackTime >= 0.f))
	{
		Slash->SetVisibility(AttackTime >= 0.f);
	}
	if (!Glider->IsVisible() && GlideBlend > Wing.HiddenBelow)
	{
		Glider->SetVisibility(true, true);
	}

	// 발밑 그림자
	FHitResult Hit;
	FCollisionQueryParams Params(TEXT("HeroShadow"), false, this);
	const FVector P = GetActorLocation();
	if (GetWorld()->LineTraceSingleByObjectType(Hit, P, P - FVector(0, 0, Shadow.Search), FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		const float H = P.Z - CapsuleHalfHeight - Hit.ImpactPoint.Z;
		const float S = FMath::Clamp(1.f - H / Shadow.FadeHeight, Shadow.MinScale, 1.f);
		// 땅에 서 있을 땐 진짜 그림자가 있으니 공중에서만 착지 위치 표시
		DropShadow->SetVisibility(H > Shadow.ShowAbove);
		DropShadow->SetWorldLocation(Hit.ImpactPoint + FVector(0, 0, 2));
		DropShadow->SetWorldScale3D(FVector(Shadow.Diameter / YJJShapes::BaseShapeSize * S, Shadow.Diameter / YJJShapes::BaseShapeSize * S, Shadow.Thickness / YJJShapes::BaseShapeSize));
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

	float TargetArm = FYJJData::Tune(TEXT("Hero.CameraArm"));
	float TargetFov = FYJJData::Tune(TEXT("Hero.CameraFov"));
	if (bGliding)
	{
		TargetArm = FYJJData::Tune(TEXT("Hero.CameraArmGlide"));
		TargetFov = FYJJData::Tune(TEXT("Hero.CameraFovGlide"));
	}
	else if (bSprinting)
	{
		TargetArm = FYJJData::Tune(TEXT("Hero.CameraArmSprint"));
		TargetFov = FYJJData::Tune(TEXT("Hero.CameraFovSprint"));
	}
	if (bFinale)
	{
		TargetArm = FYJJData::Tune(TEXT("Hero.CameraArmFinale"));
		TargetFov = FYJJData::Tune(TEXT("Hero.CameraFovFinale"));
	}
	// 엔딩 땐 기둥에 카메라가 막혀 영웅 등만 보이지 않게
	CameraBoom->bDoCollisionTest = !bFinale;
	TargetFov += FMath::Clamp((Speed - FYJJData::Tune(TEXT("Hero.CameraSpeedFovFrom"))) / FYJJData::Tune(TEXT("Hero.CameraSpeedFovRange")), 0.f, 1.f) * FYJJData::Tune(TEXT("Hero.CameraSpeedFovAdd"));

	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArm, DeltaTime, FYJJData::Tune(TEXT("Hero.CameraArmBlend")));
	FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, TargetFov, DeltaTime, FYJJData::Tune(TEXT("Hero.CameraFovBlend"))));

	CameraShake = FMath::Max(0.f, CameraShake - DeltaTime * FYJJData::Tune(TEXT("Hero.CameraShakeDecay")));
	const float Shake = CameraShake * CameraShake * FYJJData::Tune(TEXT("Hero.CameraShake"));
	FollowCamera->SetRelativeLocation(FVector(0.f, FMath::Sin(Time * ShakeFreqY) * Shake, FMath::Sin(Time * ShakeFreqZ) * Shake));
}

void AYJJHero::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	const float FallSpeed = -GetCharacterMovement()->Velocity.Z;
	LandSquash = FMath::Clamp(FallSpeed / Squash.FullAtSpeed, Squash.Min, 1.f);
	SetGliding(false);
	YJJAudio::Play(this, EYJJSound::Land, FVector::ZeroVector, FMath::Clamp(FallSpeed / Sound.LandFullAtSpeed, Sound.LandMin, FYJJData::Tune(TEXT("Hero.LandVolumeMax"))));
}
