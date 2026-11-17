#include "World/YJJWorldBuilder.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/YJJAdventureSubsystem.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "YJJAssetPaths.h"
#include "GameFramework/PlayerController.h"
#include "Gameplay/YJJChest.h"
#include "Gameplay/YJJLantern.h"
#include "Gameplay/YJJShrine.h"
#include "Gameplay/YJJSlime.h"
#include "Gameplay/YJJStardust.h"
#include "Gameplay/YJJTraversal.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Player/YJJHero.h"
#include "World/YJJEnvironment.h"
#include "World/YJJShapes.h"
#include "World/YJJSkyWhale.h"
#include "YJJFantasy.h"

namespace
{
	// 섬 팔레트: 동화 같은 밝은 색
	namespace IslandColor
	{
		const FLinearColor GrassHub(0.42f, 0.78f, 0.36f);
		const FLinearColor GrassMeadow(0.55f, 0.85f, 0.32f);
		const FLinearColor GrassCrystal(0.36f, 0.72f, 0.62f);
		const FLinearColor GrassWater(0.38f, 0.8f, 0.45f);
		const FLinearColor GrassForest(0.28f, 0.62f, 0.3f);
		const FLinearColor GrassRuins(0.62f, 0.78f, 0.42f);
		const FLinearColor RockWarm(0.62f, 0.56f, 0.5f);
		const FLinearColor RockCool(0.5f, 0.52f, 0.6f);
		const FLinearColor RockDark(0.42f, 0.4f, 0.44f);
		const FLinearColor Marble(0.82f, 0.79f, 0.74f);
		const FLinearColor LeafBright(0.45f, 0.8f, 0.3f);
		const FLinearColor LeafDeep(0.2f, 0.55f, 0.32f);
		const FLinearColor LeafPink(1.f, 0.7f, 0.8f);
		const FLinearColor LeafGold(0.98f, 0.75f, 0.3f);
	}

	template <typename T>
	T* SpawnDeferred(UWorld* World, const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator)
	{
		return World->SpawnActorDeferred<T>(T::StaticClass(), FTransform(Rotation, Location), nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	}

	template <typename T>
	void Finish(T* Actor)
	{
		Actor->FinishSpawning(Actor->GetTransform());
	}
}

// ============================================================ 소품 액터

AYJJProp::AYJJProp()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);
}

AYJJMotes::AYJJMotes()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);
}

void AYJJMotes::BeginPlay()
{
	Super::BeginPlay();
	Instances = YJJShapes::AddInstanced(this, RootComponent, EYJJShape::Sphere, YJJShapes::Glow(this, FLinearColor(1.f, 0.93f, 0.7f), 4.f, 1.f, 0.6f), false);
	Instances->SetCastShadow(false);
	for (int32 i = 0; i < 70; ++i)
	{
		FMote& M = Motes.AddDefaulted_GetRef();
		M.Offset = FVector(YJJShapes::Hash(i * 3) * 2.f - 1.f, YJJShapes::Hash(i * 3 + 1) * 2.f - 1.f, YJJShapes::Hash(i * 3 + 2) * 2.f - 1.f) * FVector(2200, 2200, 900);
		M.Drift = FVector(YJJShapes::Hash(i * 5) - 0.5f, YJJShapes::Hash(i * 5 + 1) - 0.5f, 0.3f + YJJShapes::Hash(i * 5 + 2) * 0.4f) * 60.f;
		M.Phase = YJJShapes::Hash(i * 7) * 10.f;
		Instances->AddInstance(FTransform(FRotator::ZeroRotator, M.Offset, FVector(0.06f)));
	}
}

void AYJJMotes::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Time += DeltaTime;

	// 카메라 주변 상자 안에서 떠다니다가 벗어나면 반대편으로 감싼다
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}
	const FVector Center = Pawn->GetActorLocation();
	SetActorLocation(FVector::ZeroVector);
	const FVector Box(2200, 2200, 900);
	for (int32 i = 0; i < Motes.Num(); ++i)
	{
		FMote& M = Motes[i];
		M.Offset += (M.Drift + FVector(FMath::Sin(Time * 0.7f + M.Phase) * 40.f, FMath::Cos(Time * 0.5f + M.Phase) * 40.f, 0.f)) * DeltaTime;
		FVector World = M.Offset;
		// 영웅 기준 상대 좌표로 감싸기
		FVector Rel = World - Center;
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			if (Rel[Axis] > Box[Axis])
			{
				Rel[Axis] -= Box[Axis] * 2.f;
			}
			else if (Rel[Axis] < -Box[Axis])
			{
				Rel[Axis] += Box[Axis] * 2.f;
			}
		}
		M.Offset = Center + Rel;
		const float Twinkle = 0.5f + 0.5f * FMath::Sin(Time * 2.f + M.Phase * 4.f);
		Instances->UpdateInstanceTransform(i, FTransform(FRotator::ZeroRotator, M.Offset, FVector(0.04f + Twinkle * 0.05f)), true, i == Motes.Num() - 1, true);
	}
}

// ============================================================ 월드 빌더

AYJJWorldBuilder::AYJJWorldBuilder()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

FVector AYJJWorldBuilder::GetSpawnLocation()
{
	return W(-1350.f, -1350.f, 140.f);
}

float AYJJWorldBuilder::GetSpawnYaw()
{
	return 45.f;
}

void AYJJWorldBuilder::BeginPlay()
{
	Super::BeginPlay();
	const double Start = FPlatformTime::Seconds();

	bool bHasEnvironment = false;
	for (TActorIterator<AYJJEnvironment> It(GetWorld()); It; ++It)
	{
		bHasEnvironment = true;
	}
	if (!bHasEnvironment)
	{
		GetWorld()->SpawnActor<AYJJEnvironment>(AYJJEnvironment::StaticClass(), FTransform(W(0, 0, -2000)));
	}

	YJJShapes::SetColliderSink(&Colliders);
	BuildHub();
	BuildMeadow();
	BuildCrystal();
	BuildWaterfall();
	BuildForest();
	BuildRuins();
	BuildVista();
	YJJShapes::SetColliderSink(nullptr);

	GetWorld()->SpawnActor<AYJJMotes>(AYJJMotes::StaticClass(), FTransform::Identity);

	// 영웅: 시작 위치 + 낙사 높이
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (AYJJHero* Hero = Cast<AYJJHero>(PC->GetPawn()))
		{
			Hero->KillZ = BaseZ - 4500.f;
			Hero->WarpTo(GetSpawnLocation(), GetSpawnYaw());
		}
	}
	if (UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(this))
	{
		Adv->SetCheckpoint(GetSpawnLocation(), GetSpawnYaw());
	}

	UE_LOG(LogYJJFantasy, Display, TEXT("[World] built in %.2fs, actors=%d colliders=%d"), FPlatformTime::Seconds() - Start, GetWorld()->GetActorCount(), Colliders.Num());
	Colliders.Reset();
}

// ------------------------------------------------------------ 기본 블록

AYJJProp* AYJJWorldBuilder::NewProp(const FVector& Location, const FRotator& Rotation)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return GetWorld()->SpawnActor<AYJJProp>(AYJJProp::StaticClass(), FTransform(Rotation, Location), Params);
}

AYJJProp* AYJJWorldBuilder::BuildIsland(const FVector& Center, float Radius, const FLinearColor& Grass, const FLinearColor& Rock, int32 Seed, bool bCollide, bool bDecorate)
{
	AYJJProp* Island = NewProp(Center);
	USceneComponent* R = Island->GetRootComponent();
	// 만화풍 케이크 섬: 잔디 크림 + 코코아색 흙층 + 둥글둥글한 바닥
	UMaterialInterface* Ground = YJJShapes::Surface(Island, Grass, YJJShapes::Shade(Grass, 0.8f));
	UMaterialInterface* Icing = YJJShapes::Surface(Island, YJJShapes::Shade(Grass, 1.08f), YJJShapes::Shade(Grass, 0.86f));
	UMaterialInterface* Dirt = YJJShapes::Surface(Island, FLinearColor(0.72f, 0.5f, 0.34f), FLinearColor(0.62f, 0.42f, 0.28f));
	UMaterialInterface* Under = YJJShapes::Surface(Island, Rock, YJJShapes::Shade(Rock, 0.88f));

	const float D = Radius * 2.f;
	YJJShapes::Add(Island, R, EYJJShape::Cylinder, FVector(0, 0, -70), FVector(D, D, 140), Ground, bCollide);
	YJJShapes::Add(Island, R, EYJJShape::Cylinder, FVector(0, 0, -215), FVector(D * 0.985f, D * 0.985f, 190), Dirt, false);
	// 둥근 바닥: 층층이 작아지는 납작한 공 두 개. 각 공의 꼭대기는 흙층 안(-150)에 숨겨 윗면을 뚫지 않게
	const float TopHide = -150.f;
	const float H1 = Radius * 0.25f;
	YJJShapes::Add(Island, R, EYJJShape::Sphere, FVector(0, 0, TopHide - H1), FVector(D * 0.96f, D * 0.96f, H1 * 2.f), Under, false);
	const float H2 = Radius * 0.36f;
	const float Top2 = TopHide - H1 * 0.9f;
	YJJShapes::Add(Island, R, EYJJShape::Sphere, FVector(0, 0, Top2 - H2), FVector(D * 0.62f, D * 0.62f, H2 * 2.f), Under, false);
	// 바닥에 매달린 둥근 바위 방울
	for (int32 i = 0; i < 4; ++i)
	{
		const float A = YJJShapes::Hash(Seed * 11 + i) * TWO_PI;
		const float Dist = Radius * (0.3f + 0.25f * YJJShapes::Hash(Seed * 13 + i));
		const float S = Radius * (0.28f + 0.14f * YJJShapes::Hash(Seed * 17 + i));
		YJJShapes::Add(Island, R, EYJJShape::Sphere, FVector(FMath::Cos(A) * Dist, FMath::Sin(A) * Dist, TopHide - H1 * 1.5f - S * 0.3f), FVector(S, S, S * 0.85f), Under, false);
	}
	// 가장자리로 흘러내린 잔디 크림 방울 (윗면보다 살짝 아래라 밟히지 않는다)
	const int32 RimCount = FMath::Clamp(FMath::RoundToInt(Radius / 110.f), 10, 40);
	for (int32 i = 0; i < RimCount; ++i)
	{
		const float A = (i + YJJShapes::Hash(Seed * 19 + i) * 0.5f) * (TWO_PI / RimCount);
		const float S = 150.f + 130.f * YJJShapes::Hash(Seed * 23 + i);
		const FVector P(FMath::Cos(A) * Radius * 0.99f, FMath::Sin(A) * Radius * 0.99f, -60.f - S * 0.4f);
		YJJShapes::Add(Island, R, EYJJShape::Sphere, P, FVector(S * 1.3f, S * 1.1f, S * 0.75f), Icing, false, FRotator(0, FMath::RadiansToDegrees(A), 0));
	}

	if (bDecorate)
	{
		static const TArray<FLinearColor> DefaultFlowers = { FLinearColor(1.f, 0.85f, 0.3f), FLinearColor(1.f, 0.55f, 0.7f), FLinearColor(1.f, 1.f, 0.95f) };
		Decorate(Island, Radius, Grass, Seed, FMath::RoundToInt(Radius / 18.f), DefaultFlowers);
	}
	return Island;
}

void AYJJWorldBuilder::AddHill(AYJJProp* Island, const FVector& Local, float Radius, float Height, const FLinearColor& Grass, const FLinearColor& Rock)
{
	// 납작한 구를 반쯤 묻어서 완만한 언덕 (경사 약 20도 이하라 걸어서 오를 수 있다)
	const float ZRadius = Height / 0.25f;
	YJJShapes::Add(Island, Island->GetRootComponent(), EYJJShape::Sphere, Local + FVector(0, 0, Height - ZRadius), FVector(Radius * 2.f, Radius * 2.f, ZRadius * 2.f),
		YJJShapes::Surface(Island, Grass, Rock, 0.f), true);
}

void AYJJWorldBuilder::Decorate(AYJJProp* Island, float Radius, const FLinearColor& Grass, int32 Seed, int32 FlowerCount, const TArray<FLinearColor>& FlowerColors)
{
	USceneComponent* R = Island->GetRootComponent();
	const FVector C = Island->GetActorLocation();

	// 풀숲: 가는 풀잎은 먹선 때문에 지저분해서, 동글동글한 풀 덩어리를 드문드문 (만화 배경처럼)
	UInstancedStaticMeshComponent* GrassA = YJJShapes::AddInstanced(Island, R, EYJJShape::Sphere, YJJShapes::Surface(Island, YJJShapes::Shade(Grass, 1.15f), YJJShapes::Shade(Grass, 0.85f)), false);
	UInstancedStaticMeshComponent* GrassB = YJJShapes::AddInstanced(Island, R, EYJJShape::Sphere, YJJShapes::Surface(Island, YJJShapes::Shade(Grass, 1.3f), Grass), false);
	GrassA->SetCastShadow(false);
	GrassB->SetCastShadow(false);
	const int32 GrassCount = FMath::RoundToInt(Radius * Radius / 26000.f);
	for (int32 i = 0; i < GrassCount; ++i)
	{
		const float A = YJJShapes::Hash(Seed * 101 + i) * TWO_PI;
		const float Dist = FMath::Sqrt(YJJShapes::Hash(Seed * 103 + i)) * Radius * 0.92f;
		float Z;
		if (!GroundAt(C + FVector(FMath::Cos(A) * Dist, FMath::Sin(A) * Dist, 0), Z, 1500.f))
		{
			continue;
		}
		const float S = 0.6f + YJJShapes::Hash(Seed * 107 + i) * 0.8f;
		const FVector Local(FMath::Cos(A) * Dist, FMath::Sin(A) * Dist, Z - C.Z);
		// 풀숲 하나 = 둥근 덩어리 세 개 (가운데가 크다)
		for (int32 b = 0; b < 3; ++b)
		{
			const float BA = b * 2.1f + i;
			const float BS = (b == 0 ? 1.f : 0.7f) * S;
			const FVector Off = (b == 0) ? FVector::ZeroVector : FVector(FMath::Cos(BA) * 26.f * S, FMath::Sin(BA) * 26.f * S, 0.f);
			(i % 2 ? GrassA : GrassB)->AddInstance(FTransform(FRotator(0, BA * 57.f, 0), Local + Off + FVector(0, 0, 6.f * BS), FVector(0.55f * BS, 0.48f * BS, 0.36f * BS)));
		}
	}
	UE_LOG(LogYJJFantasy, Display, TEXT("[World] decorate %s grass=%d/%d"), *Island->GetName(), GrassA->GetInstanceCount() + GrassB->GetInstanceCount(), GrassCount);

	// 꽃 (동그란 꽃송이 + 하얀 중심)
	for (int32 c = 0; c < FlowerColors.Num(); ++c)
	{
		UInstancedStaticMeshComponent* Flowers = YJJShapes::AddInstanced(Island, R, EYJJShape::Sphere, YJJShapes::Solid(Island, FlowerColors[c], 0.25f, 0.6f), false);
		Flowers->SetCastShadow(false);
		for (int32 i = 0; i < FlowerCount / FlowerColors.Num(); ++i)
		{
			const int32 K = Seed * 211 + c * 977 + i;
			// 꽃은 무리 지어 핀다
			const float ClusterA = YJJShapes::Hash(K / 6 * 7 + 1) * TWO_PI;
			const float ClusterD = FMath::Sqrt(YJJShapes::Hash(K / 6 * 7 + 2)) * Radius * 0.85f;
			const FVector ClusterP(FMath::Cos(ClusterA) * ClusterD, FMath::Sin(ClusterA) * ClusterD, 0);
			const FVector P = ClusterP + FVector(YJJShapes::Hash(K * 3) - 0.5f, YJJShapes::Hash(K * 3 + 1) - 0.5f, 0) * 260.f;
			float Z;
			if (!GroundAt(C + P, Z, 1500.f))
			{
				continue;
			}
			const float S = 0.7f + YJJShapes::Hash(K * 5) * 0.6f;
			Flowers->AddInstance(FTransform(FRotator::ZeroRotator, FVector(P.X, P.Y, Z - C.Z + 16.f * S), FVector(0.2f * S, 0.2f * S, 0.12f * S)));
		}
	}

	// 돌멩이
	UInstancedStaticMeshComponent* Pebbles = YJJShapes::AddInstanced(Island, R, EYJJShape::Sphere, YJJShapes::Surface(Island, FLinearColor(0.75f, 0.73f, 0.7f), IslandColor::RockWarm), false);
	for (int32 i = 0; i < FMath::RoundToInt(Radius / 60.f); ++i)
	{
		const float A = YJJShapes::Hash(Seed * 307 + i) * TWO_PI;
		const float Dist = FMath::Sqrt(YJJShapes::Hash(Seed * 311 + i)) * Radius * 0.9f;
		float Z;
		if (!GroundAt(C + FVector(FMath::Cos(A) * Dist, FMath::Sin(A) * Dist, 0), Z, 1500.f))
		{
			continue;
		}
		const float S = 0.3f + YJJShapes::Hash(Seed * 313 + i) * 0.5f;
		Pebbles->AddInstance(FTransform(FRotator(0, A * 57.f, 0), FVector(FMath::Cos(A) * Dist, FMath::Sin(A) * Dist, Z - C.Z + 5.f), FVector(0.6f * S, 0.45f * S, 0.25f * S)));
	}
}

void AYJJWorldBuilder::AddTree(const FVector& Base, float Scale, int32 Seed, const FLinearColor& Leaf)
{
	AYJJProp* Tree = NewProp(Base, FRotator(0, YJJShapes::Hash(Seed) * 360.f, 0));
	USceneComponent* R = Tree->GetRootComponent();
	const float S = Scale;
	UMaterialInterface* Bark = YJJShapes::Surface(Tree, FLinearColor(0.7f, 0.48f, 0.32f), FLinearColor(0.58f, 0.38f, 0.25f));
	UMaterialInterface* LeafMat = YJJShapes::Surface(Tree, YJJShapes::Shade(Leaf, 1.12f), YJJShapes::Shade(Leaf, 0.8f), 0.04f, 0.9f);

	// 막대사탕 나무: 곧은 줄기 + 동그란 잎 덩어리 하나 (+ 작은 혹 둘)
	YJJShapes::Add(Tree, R, EYJJShape::Cylinder, FVector(0, 0, 170 * S), FVector(50 * S, 50 * S, 340 * S), Bark, true);
	YJJShapes::Add(Tree, R, EYJJShape::Sphere, FVector(0, 0, 10 * S), FVector(90 * S, 90 * S, 40 * S), Bark, false);
	const float Crown = (360.f + 60.f * YJJShapes::Hash(Seed * 3)) * S;
	const FVector CrownC(0, 0, 440.f * S);
	YJJShapes::Add(Tree, R, EYJJShape::Sphere, CrownC, FVector(Crown, Crown, Crown * 0.9f), LeafMat, true);
	for (int32 i = 0; i < 2; ++i)
	{
		const float A = i * 2.6f + Seed;
		const float Size = Crown * (0.42f + 0.1f * YJJShapes::Hash(Seed * 11 + i));
		YJJShapes::Add(Tree, R, EYJJShape::Sphere, CrownC + FVector(FMath::Cos(A) * Crown * 0.42f, FMath::Sin(A) * Crown * 0.42f, Crown * (0.12f + 0.15f * i)), FVector(Size, Size, Size * 0.9f), LeafMat, false);
	}
	// 셋 중 하나는 멍한 얼굴이 있다 (엉뚱한 포인트)
	if (Seed % 3 == 0)
	{
		YJJShapes::AddFace(Tree, R, CrownC + FVector(Crown * 0.49f, 0, -Crown * 0.05f), FRotator::ZeroRotator, Crown * 0.38f, Seed % 2);
	}
}

void AYJJWorldBuilder::AddPine(const FVector& Base, float Scale, const FLinearColor& Leaf)
{
	AYJJProp* Tree = NewProp(Base);
	USceneComponent* R = Tree->GetRootComponent();
	const float S = Scale;
	// 소프트아이스크림 같은 둥근 층 나무 (뾰족한 원뿔 대신)
	YJJShapes::Add(Tree, R, EYJJShape::Cylinder, FVector(0, 0, 90 * S), FVector(44 * S, 44 * S, 180 * S), YJJShapes::Surface(Tree, FLinearColor(0.7f, 0.48f, 0.32f), FLinearColor(0.58f, 0.38f, 0.25f)), true);
	UMaterialInterface* LeafMat = YJJShapes::Surface(Tree, YJJShapes::Shade(Leaf, 1.3f), YJJShapes::Shade(Leaf, 0.95f), 0.04f, 0.9f);
	for (int32 i = 0; i < 3; ++i)
	{
		const float W0 = (300.f - i * 80.f) * S;
		YJJShapes::Add(Tree, R, EYJJShape::Sphere, FVector(0, 0, (230.f + i * 120.f) * S), FVector(W0, W0, W0 * 0.55f), LeafMat, i == 0);
	}
	// 꼭대기 동그란 방울
	YJJShapes::Add(Tree, R, EYJJShape::Sphere, FVector(0, 0, 590.f * S), FVector(70 * S, 70 * S, 70 * S), LeafMat, false);
}

void AYJJWorldBuilder::AddRock(const FVector& Base, float Size, int32 Seed, const FLinearColor& Color)
{
	AYJJProp* Rock = NewProp(Base, FRotator(0, YJJShapes::Hash(Seed) * 360.f, 0));
	// 둥근 조약돌 바위. 짝수 번째는 졸린 얼굴
	UMaterialInterface* Mat = YJJShapes::Surface(Rock, YJJShapes::Shade(Color, 1.25f), Color);
	YJJShapes::Add(Rock, Rock->GetRootComponent(), EYJJShape::Sphere, FVector(0, 0, Size * 0.3f), FVector(Size * 1.3f, Size, Size * 0.9f), Mat, true);
	YJJShapes::Add(Rock, Rock->GetRootComponent(), EYJJShape::Sphere, FVector(-Size * 0.45f, Size * 0.35f, Size * 0.1f), FVector(Size * 0.6f, Size * 0.55f, Size * 0.45f), Mat, true);
	if (Seed % 2 == 0)
	{
		YJJShapes::AddFace(Rock, Rock->GetRootComponent(), FVector(Size * 0.62f, 0, Size * 0.38f), FRotator::ZeroRotator, Size * 0.55f, 1);
	}
}

void AYJJWorldBuilder::AddCrystals(const FVector& Base, float Size, const FLinearColor& Color, int32 Seed, bool bLight)
{
	AYJJProp* Cluster = NewProp(Base);
	USceneComponent* R = Cluster->GetRootComponent();
	// 만화풍 사탕 수정: 통통하고 파스텔, 은은하게만 빛난다
	UMaterialInterface* Mat = YJJShapes::Surface(Cluster, YJJShapes::Shade(Color, 1.35f), YJJShapes::Shade(Color, 1.05f), 0.45f, 0.4f);
	const int32 Count = 3 + Seed % 3;
	for (int32 i = 0; i < Count; ++i)
	{
		const float A = i * 2.399963f + Seed;
		const float H = Size * (i == 0 ? 1.f : 0.45f + 0.4f * YJJShapes::Hash(Seed * 5 + i));
		const float Wd = H * 0.48f;
		// 가장 큰 수정 몇몇은 동그란 놀란 눈
		if (i == 0 && Seed % 2 == 1)
		{
			YJJShapes::AddFace(Cluster, R, FVector(Wd * 0.33f, 0, H * 0.25f), FRotator(0, 0, 0), Wd * 0.32f, 2);
		}
		const float Off = (i == 0) ? 0.f : Size * 0.22f;
		const FRotator Tilt(i == 0 ? 0.f : 25.f + 15.f * YJJShapes::Hash(i), FMath::RadiansToDegrees(A), 0);
		const FVector Dir = Tilt.RotateVector(FVector(0, 0, 1));
		const FVector P = FVector(FMath::Cos(A) * Off, FMath::Sin(A) * Off, 0) + Dir * H * 0.4f;
		YJJShapes::Add(Cluster, R, EYJJShape::Cone, P, FVector(Wd, Wd, H), Mat, i == 0, Tilt);
		YJJShapes::Add(Cluster, R, EYJJShape::Cone, P - Dir * H * 0.5f, FVector(Wd, Wd, H * 0.25f), Mat, false, Tilt + FRotator(180, 0, 0));
	}
	if (bLight)
	{
		UPointLightComponent* Light = NewObject<UPointLightComponent>(Cluster);
		Light->SetupAttachment(R);
		Light->SetRelativeLocation(FVector(0, 0, Size * 0.6f));
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetIntensity(120.f);
		Light->SetAttenuationRadius(Size * 4.f);
		Light->SetLightColor(Color);
		Light->SetCastShadows(false);
		Light->RegisterComponent();
		Cluster->AddInstanceComponent(Light);
	}
}

void AYJJWorldBuilder::AddPillar(const FVector& Base, float Height, float Radius, bool bBroken, const FLinearColor& Stone)
{
	AYJJProp* Pillar = NewProp(Base);
	USceneComponent* R = Pillar->GetRootComponent();
	UMaterialInterface* Mat = YJJShapes::Surface(Pillar, Stone, YJJShapes::Shade(Stone, 0.82f));
	YJJShapes::Add(Pillar, R, EYJJShape::Cube, FVector(0, 0, 25), FVector(Radius * 2.8f, Radius * 2.8f, 50), Mat, true);
	YJJShapes::Add(Pillar, R, EYJJShape::Cylinder, FVector(0, 0, 50 + Height * 0.5f), FVector(Radius * 2.f, Radius * 2.f, Height), Mat, true);
	// 세로 홈 장식
	for (int32 i = 0; i < 6; ++i)
	{
		const float A = i * (TWO_PI / 6.f);
		YJJShapes::Add(Pillar, R, EYJJShape::Cube, FVector(FMath::Cos(A) * Radius, FMath::Sin(A) * Radius, 50 + Height * 0.5f), FVector(10, 10, Height * 0.92f), Mat, false, FRotator(0, FMath::RadiansToDegrees(A), 0));
	}
	if (!bBroken)
	{
		YJJShapes::Add(Pillar, R, EYJJShape::Cube, FVector(0, 0, 50 + Height + 30), FVector(Radius * 2.9f, Radius * 2.9f, 60), Mat, true);
	}
	else
	{
		YJJShapes::Add(Pillar, R, EYJJShape::Cone, FVector(0, 0, 50 + Height + 20), FVector(Radius * 2.f, Radius * 2.f, 60), Mat, false, FRotator(0, 0, 20));
	}
}

void AYJJWorldBuilder::AddArch(const FVector& Base, float Yaw, float Width, float Height, const FLinearColor& Stone)
{
	AYJJProp* Arch = NewProp(Base, FRotator(0, Yaw, 0));
	USceneComponent* R = Arch->GetRootComponent();
	UMaterialInterface* Mat = YJJShapes::Surface(Arch, Stone, YJJShapes::Shade(Stone, 0.82f));
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		YJJShapes::Add(Arch, R, EYJJShape::Cube, FVector(0, Side * Width * 0.5f, Height * 0.5f), FVector(120, 120, Height), Mat, true);
	}
	YJJShapes::Add(Arch, R, EYJJShape::Cube, FVector(0, 0, Height + 50), FVector(150, Width + 200, 100), Mat, true);
	YJJShapes::Add(Arch, R, EYJJShape::Cylinder, FVector(0, 0, Height - 10), FVector(Width * 0.95f, 110, Width * 0.5f), Mat, false, FRotator(0, 0, 90));
	YJJShapes::Add(Arch, R, EYJJShape::Sphere, FVector(70, 0, Height + 30), FVector(50, 50, 50), YJJShapes::Solid(Arch, FLinearColor(0.4f, 0.9f, 1.f), 2.5f), false);
}

void AYJJWorldBuilder::AddWaterfall(const FVector& Top, float Yaw, float Width, float Drop)
{
	AYJJProp* Fall = NewProp(Top, FRotator(0, Yaw, 0));
	USceneComponent* R = Fall->GetRootComponent();
	// 쏟아지는 물줄기 (살짝 앞으로 휘게 세 토막)
	UMaterialInterface* Water = YJJShapes::Water(Fall, 1.6f, 0.78f);
	YJJShapes::Add(Fall, R, EYJJShape::Cube, FVector(40, 0, -Drop * 0.12f), FVector(60, Width, Drop * 0.26f), Water, false, FRotator(-25, 0, 0));
	YJJShapes::Add(Fall, R, EYJJShape::Cube, FVector(110, 0, -Drop * 0.55f), FVector(60, Width * 1.05f, Drop * 0.7f), Water, false);
	YJJShapes::Add(Fall, R, EYJJShape::Cube, FVector(110, 0, -Drop * 0.55f), FVector(120, Width * 1.2f, Drop * 0.7f), YJJShapes::Glow(Fall, FLinearColor(0.75f, 0.95f, 1.f), 0.25f, 0.f, 1.f), false);
	// 아래 물보라
	YJJShapes::Add(Fall, R, EYJJShape::Sphere, FVector(110, 0, -Drop * 0.92f), FVector(Width * 1.4f, Width * 1.4f, Width * 0.8f), YJJShapes::Glow(Fall, FLinearColor(0.85f, 0.97f, 1.f), 0.6f, 0.2f, 1.f), false);
}

AYJJProp* AYJJWorldBuilder::AddStone(const FVector& Top, float Radius, const FLinearColor& Grass, const FLinearColor& Rock, int32 Seed)
{
	// 작은 떠 있는 징검돌
	AYJJProp* Stone = NewProp(Top);
	USceneComponent* R = Stone->GetRootComponent();
	const float D = Radius * 2.f;
	YJJShapes::Add(Stone, R, EYJJShape::Cylinder, FVector(0, 0, -30), FVector(D, D, 60), YJJShapes::Surface(Stone, Grass, Rock), true);
	YJJShapes::Add(Stone, R, EYJJShape::Sphere, FVector(0, 0, -60 - Radius * 0.62f), FVector(D * 0.95f, D * 0.95f, Radius * 1.3f), YJJShapes::Surface(Stone, Rock, YJJShapes::Shade(Rock, 0.85f)), false);
	YJJShapes::Add(Stone, R, EYJJShape::Sphere, FVector(Radius * 0.4f, 0, -20), FVector(Radius * 0.7f, Radius * 0.6f, 70), YJJShapes::Surface(Stone, Grass, Rock), true, FRotator(0, Seed * 47.f, 0));
	return Stone;
}

void AYJJWorldBuilder::AddStardust(const FVector& Location)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	GetWorld()->SpawnActor<AYJJStardust>(AYJJStardust::StaticClass(), FTransform(Location), Params);
}

void AYJJWorldBuilder::AddChest(const FVector& Location, float Yaw)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	GetWorld()->SpawnActor<AYJJChest>(AYJJChest::StaticClass(), FTransform(FRotator(0, Yaw, 0), Location), Params);
}

void AYJJWorldBuilder::AddStardustLine(const FVector& From, const FVector& To, int32 Count, float Arc)
{
	for (int32 i = 0; i < Count; ++i)
	{
		const float T = Count > 1 ? i / static_cast<float>(Count - 1) : 0.5f;
		AddStardust(FMath::Lerp(From, To, T) + FVector(0, 0, Arc * FMath::Sin(T * PI)));
	}
}

void AYJJWorldBuilder::AddStardustRing(const FVector& Center, float Radius, int32 Count)
{
	for (int32 i = 0; i < Count; ++i)
	{
		const float A = i * (TWO_PI / Count);
		AddStardust(Center + FVector(FMath::Cos(A) * Radius, FMath::Sin(A) * Radius, 0));
	}
}

bool AYJJWorldBuilder::GroundAt(const FVector& XY, float& OutZ, float From) const
{
	const FVector Start(XY.X, XY.Y, XY.Z + From);
	const FVector End(XY.X, XY.Y, XY.Z - From);

	// 짓는 중에는 물리 씬 검색이 새 도형을 못 보므로, 만든 충돌 도형마다 직접 검사한다
	if (Colliders.Num() > 0)
	{
		float BestZ = -TNumericLimits<float>::Max();
		float BestNormalZ = 0.f;
		for (const YJJShapes::FCollider& Collider : Colliders)
		{
			// 대충 거른다: 도형 중심에서 수평으로 크기보다 멀면 건너뛴다
			const FVector C = Collider.Transform.GetLocation();
			const float Extent = Collider.Transform.GetScale3D().GetMax() * 50.f * 1.75f;
			if (FMath::Abs(C.X - XY.X) > Extent || FMath::Abs(C.Y - XY.Y) > Extent)
			{
				continue;
			}
			float T;
			FVector N;
			if (YJJShapes::RayCast(Collider, Start, End, T, N))
			{
				const float Z = FMath::Lerp(Start.Z, End.Z, T);
				if (Z > BestZ)
				{
					BestZ = Z;
					BestNormalZ = N.Z;
				}
			}
		}
		if (BestZ > -TNumericLimits<float>::Max())
		{
			OutZ = BestZ;
			return BestNormalZ > 0.6f;
		}
		return false;
	}

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, FCollisionObjectQueryParams(ECC_WorldStatic)))
	{
		OutZ = Hit.ImpactPoint.Z;
		return Hit.ImpactNormal.Z > 0.6f;
	}
	return false;
}

FVector AYJJWorldBuilder::OnGround(const FVector& XY, float Lift) const
{
	float Z = XY.Z;
	GroundAt(XY, Z, 3000.f);
	return FVector(XY.X, XY.Y, Z + Lift);
}

void AYJJWorldBuilder::AddZone(const FString& Name, const FString& Subtitle, const FVector& Center, float Radius)
{
	if (UYJJAdventureSubsystem* Adv = UYJJAdventureSubsystem::Get(this))
	{
		FYJJZone Zone;
		Zone.Name = Name;
		Zone.Subtitle = Subtitle;
		Zone.Center = Center;
		Zone.Radius = Radius;
		Adv->AddZone(Zone);
	}
}

// ============================================================ 섬 구성

void AYJJWorldBuilder::BuildHub()
{
	// 바람의 성소: 시작 섬. 가운데 성소, 동쪽 징검돌(들꽃 언덕), 북쪽 버섯(수정섬), 서쪽 상승 기류(폭포 정원), 남쪽 절벽(고목의 숲)
	const FVector C = W(0, 0, 0);
	AYJJProp* Island = BuildIsland(C, 2700.f, IslandColor::GrassHub, IslandColor::RockWarm, 1, true, false);
	AddHill(Island, FVector(-1500, 900, 0), 700, 90, IslandColor::GrassHub, IslandColor::RockWarm);
	AddHill(Island, FVector(1300, -1500, 0), 600, 70, IslandColor::GrassHub, IslandColor::RockWarm);
	static const TArray<FLinearColor> Flowers = { FLinearColor(1.f, 0.85f, 0.3f), FLinearColor(0.6f, 0.75f, 1.f), FLinearColor(1.f, 1.f, 0.95f) };
	Decorate(Island, 2700.f, IslandColor::GrassHub, 1, 150, Flowers);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	GetWorld()->SpawnActor<AYJJShrine>(AYJJShrine::StaticClass(), FTransform(C), Params);

	// 가장자리 나무와 바위
	const int32 Trees[][3] = { { -2100, -700, 1 }, { -1900, 1600, 2 }, { 900, 2200, 3 }, { 2150, -800, 4 }, { -400, -2250, 5 }, { 1700, 1500, 6 }, { -2300, 300, 7 } };
	for (const auto& T : Trees)
	{
		AddTree(OnGround(W(T[0], T[1], 0)), 1.f + (T[2] % 3) * 0.15f, T[2], T[2] % 2 ? IslandColor::LeafBright : IslandColor::LeafPink);
	}
	AddRock(OnGround(W(-900, -2000, 0)), 220, 3, IslandColor::RockWarm);
	AddRock(OnGround(W(2100, 600, 0)), 180, 4, IslandColor::RockWarm);
	AddRock(OnGround(W(-2200, -1500, 0)), 260, 5, IslandColor::RockWarm);
	AddPillar(OnGround(W(-1700, -1950, 0)), 420, 55, true, IslandColor::Marble);
	AddPillar(OnGround(W(1900, 1900, 0)), 520, 55, false, IslandColor::Marble);

	// 동쪽 -> 들꽃 언덕: 오르막 징검돌
	for (int32 i = 0; i < 6; ++i)
	{
		const FVector P = W(2950.f + i * 430.f, 250.f + i * 150.f + FMath::Sin(i * 1.3f) * 120.f, 40.f + i * 50.f);
		AddStone(P, 170.f + (i % 2) * 30.f, IslandColor::GrassHub, IslandColor::RockWarm, i);
		AddStardust(P + FVector(0, 0, 150));
	}

	// 북쪽 -> 수정섬: 튕김 버섯
	AYJJBouncePad* Pad = SpawnDeferred<AYJJBouncePad>(GetWorld(), OnGround(W(-700, 2250, 0)));
	Pad->Setup(2000.f, 1.2f, FLinearColor(0.95f, 0.32f, 0.38f));
	Finish(Pad);
	AddStardustLine(W(-700, 2250, 700), W(-1100, 3400, 1300), 5);

	// 서쪽 -> 폭포 정원: 상승 기류
	AYJJUpdraft* Draft = SpawnDeferred<AYJJUpdraft>(GetWorld(), OnGround(W(-2350, -500, 0)));
	Draft->Setup(340.f, 3600.f);
	Finish(Draft);
	for (int32 i = 0; i < 6; ++i)
	{
		AddStardust(W(-2350, -500, 500.f + i * 550.f));
	}

	// 남쪽 절벽 끝 표지 (활공해서 내려가라는 뜻)
	AddArch(OnGround(W(0, -2450, 0)), 0.f, 420.f, 380.f, IslandColor::Marble);
	AddStardustLine(W(150, -3200, 0), W(1600, -5200, -700), 6);

	// 성소 둘레 별가루
	AddStardustRing(W(0, 0, 230), 1250.f, 8);

	// 보물: 남서쪽 바위 뒤 가장자리
	AddChest(OnGround(W(-2400, -1150, 0)), 30.f);

	AddZone(TEXT("바람의 성소"), TEXT("빛을 잃은 하늘의 심장"), C, 2900.f);
}

void AYJJWorldBuilder::BuildMeadow()
{
	// 들꽃 언덕: 완만한 언덕, 꽃밭, 슬라임 셋. 등불은 언덕 꼭대기
	const FVector C = W(6900, 1300, 320);
	AYJJProp* Island = BuildIsland(C, 2000.f, IslandColor::GrassMeadow, IslandColor::RockWarm, 2, true, false);
	AddHill(Island, FVector(500, 300, 0), 900, 180, IslandColor::GrassMeadow, IslandColor::RockWarm);
	AddHill(Island, FVector(-700, -700, 0), 500, 80, IslandColor::GrassMeadow, IslandColor::RockWarm);
	static const TArray<FLinearColor> Flowers = { FLinearColor(1.f, 0.5f, 0.6f), FLinearColor(1.f, 0.85f, 0.25f), FLinearColor(0.75f, 0.6f, 1.f), FLinearColor(1.f, 1.f, 1.f) };
	Decorate(Island, 2000.f, IslandColor::GrassMeadow, 2, 360, Flowers);

	AddTree(OnGround(C + FVector(-1200, 900, 0)), 1.3f, 11, IslandColor::LeafPink);
	AddTree(OnGround(C + FVector(1400, -900, 0)), 1.1f, 12, IslandColor::LeafBright);
	AddTree(OnGround(C + FVector(-300, -1500, 0)), 0.9f, 13, IslandColor::LeafPink);
	AddRock(OnGround(C + FVector(1300, 1100, 0)), 200, 14, IslandColor::RockWarm);

	// AI 풍차 (언덕 북쪽 가장자리). 밑동만 밟히는 원기둥으로 막는다
	if (UStaticMesh* Windmill = YJJShapes::AIMesh(TEXT("SM_Windmill")))
	{
		const FVector Base = OnGround(C + FVector(-250, 1500, 0), -20.f);
		AYJJProp* Mill = NewProp(Base, FRotator(0, -110, 0));
		YJJShapes::Add(Mill, Mill->GetRootComponent(), EYJJShape::Cylinder, FVector(0, 0, 250), FVector(420, 420, 500), nullptr, true)->SetVisibility(false);
		YJJShapes::AddAI(Mill, Mill->GetRootComponent(), Windmill, FVector::ZeroVector, 900.f, true);
	}

	AYJJLantern* Lantern = SpawnDeferred<AYJJLantern>(GetWorld(), OnGround(C + FVector(500, 300, 0)), FRotator(0, 200, 0));
	Lantern->Setup(TEXT("들꽃 언덕의 등불"), FLinearColor(1.f, 0.6f, 0.7f));
	Finish(Lantern);

	const FLinearColor SlimeColors[] = { FLinearColor(0.5f, 0.85f, 0.5f), FLinearColor(1.f, 0.6f, 0.7f), FLinearColor(0.55f, 0.85f, 0.5f) };
	const FVector SlimePos[] = { FVector(-600, 500, 0), FVector(900, -500, 0), FVector(-400, -900, 0) };
	for (int32 i = 0; i < 3; ++i)
	{
		AYJJSlime* Slime = SpawnDeferred<AYJJSlime>(GetWorld(), OnGround(C + SlimePos[i]));
		Slime->Setup(SlimeColors[i], 900.f, 1.f);
		Finish(Slime);
	}

	// 하늘 위 별가루 무리: 버섯을 밟아야 닿는다
	AYJJBouncePad* Pad = SpawnDeferred<AYJJBouncePad>(GetWorld(), OnGround(C + FVector(-1300, -200, 0)));
	Pad->Setup(1700.f, 1.f, FLinearColor(1.f, 0.75f, 0.3f));
	Finish(Pad);
	AddStardustRing(OnGround(C + FVector(-1300, -200, 0), 900.f), 180.f, 5);

	// 남쪽 상승 기류 -> 하늘 유적
	AYJJUpdraft* Draft = SpawnDeferred<AYJJUpdraft>(GetWorld(), OnGround(C + FVector(500, -1650, 0)));
	Draft->Setup(320.f, 5000.f);
	Finish(Draft);
	AddStardustLine(C + FVector(500, -1650, 1500), C + FVector(500, -1650, 4500), 4);
	AddStardustLine(C + FVector(900, -2400, 4700), W(9300, -3300, 4300), 5);

	// 보물: 분홍 나무 꼭대기 (버섯을 밟고 활공해서)
	AddChest(OnGround(C + FVector(-1174, 900, 0)), -60.f);

	AddZone(TEXT("들꽃 언덕"), TEXT("바람이 꽃잎을 나르는 곳"), C, 2100.f);
}

void AYJJWorldBuilder::BuildCrystal()
{
	// 수정섬: 푸른 수정이 빛나는 섬. 움직이는 발판 두 개를 건너 도착
	const FVector C = W(-2700, 7700, 1400);
	AYJJProp* Island = BuildIsland(C, 1800.f, IslandColor::GrassCrystal, IslandColor::RockCool, 3, true, false);
	AddHill(Island, FVector(-300, 400, 0), 800, 120, IslandColor::GrassCrystal, IslandColor::RockCool);
	static const TArray<FLinearColor> Flowers = { FLinearColor(0.6f, 0.9f, 1.f), FLinearColor(0.85f, 0.75f, 1.f) };
	Decorate(Island, 1800.f, IslandColor::GrassCrystal, 3, 160, Flowers);

	const FLinearColor Cyan(0.35f, 0.85f, 1.f);
	const FLinearColor Violet(0.7f, 0.5f, 1.f);
	AddCrystals(OnGround(C + FVector(-900, 900, 0)), 700.f, Cyan, 1, true);
	AddCrystals(OnGround(C + FVector(900, 700, 0)), 520.f, Violet, 2, true);
	// 작은 수정 둘은 AI 수정 무리로 (없으면 도형 수정)
	if (UStaticMesh* Cluster = YJJShapes::AIMesh(TEXT("SM_CrystalCluster")))
	{
		const FVector Spots[] = { FVector(-1100, -600, 0), FVector(700, -900, 0) };
		for (int32 i = 0; i < 2; ++i)
		{
			AYJJProp* P = NewProp(OnGround(C + Spots[i], -30.f), FRotator(0, 70.f + i * 140.f, 0));
			YJJShapes::Add(P, P->GetRootComponent(), EYJJShape::Cylinder, FVector(0, 0, 150), FVector(380, 380, 300), nullptr, true)->SetVisibility(false);
			YJJShapes::AddAI(P, P->GetRootComponent(), Cluster, FVector::ZeroVector, i == 0 ? 650.f : 560.f, true);
		}
	}
	else
	{
		AddCrystals(OnGround(C + FVector(-1100, -600, 0)), 420.f, Cyan, 3, false);
		AddCrystals(OnGround(C + FVector(700, -900, 0)), 380.f, Violet, 4, false);
	}
	AddCrystals(OnGround(C + FVector(100, 1300, 0)), 600.f, Cyan, 5, true);
	AddPine(OnGround(C + FVector(1300, -200, 0)), 1.1f, IslandColor::LeafDeep);
	AddPine(OnGround(C + FVector(-1400, 200, 0)), 1.3f, IslandColor::LeafDeep);

	// 수정 아치 안의 등불
	AddArch(OnGround(C + FVector(-300, 400, 0)), 30.f, 520.f, 460.f, FLinearColor(0.7f, 0.78f, 0.9f));
	AYJJLantern* Lantern = SpawnDeferred<AYJJLantern>(GetWorld(), OnGround(C + FVector(-300, 400, 0)), FRotator(0, 250, 0));
	Lantern->Setup(TEXT("수정섬의 등불"), Cyan);
	Finish(Lantern);

	for (int32 i = 0; i < 2; ++i)
	{
		AYJJSlime* Slime = SpawnDeferred<AYJJSlime>(GetWorld(), OnGround(C + FVector(500 - i * 1000, -500 + i * 200, 0)));
		Slime->Setup(FLinearColor(0.45f, 0.7f, 1.f), 800.f, 1.15f);
		Finish(Slime);
	}

	// 길: 중앙섬 북쪽 버섯 -> 좌우로 움직이는 발판 -> 위아래로 움직이는 발판 -> 수정섬
	AYJJMovingPlatform* A = SpawnDeferred<AYJJMovingPlatform>(GetWorld(), W(-1600, 3900, 700));
	A->Setup(FVector(900, 0, 0), 6.f, 260.f, 0.f);
	Finish(A);
	AYJJMovingPlatform* B = SpawnDeferred<AYJJMovingPlatform>(GetWorld(), W(-1900, 5250, 700));
	B->Setup(FVector(0, 0, 750), 5.f, 240.f, 0.25f);
	Finish(B);
	AddStone(W(-2150, 6000, 1380), 200.f, IslandColor::GrassCrystal, IslandColor::RockCool, 31);
	AddStardustLine(W(-1600, 3900, 900), W(-700, 3900, 900), 3);
	AddStardustLine(W(-1900, 5250, 1100), W(-1900, 5250, 1700), 3);

	// 섬 아래쪽에 숨은 별가루 (활공으로 한 바퀴)
	AddStardustRing(C + FVector(0, 0, -900), 2100.f, 6);

	// 보물: 북쪽 끝 너머 떠 있는 작은 바위 (섬 끝에서 활공)
	AddStone(W(-2600, 10100, 1150), 260.f, IslandColor::GrassCrystal, IslandColor::RockCool, 77);
	AddChest(W(-2600, 10100, 1150), 180.f);

	AddZone(TEXT("수정섬"), TEXT("잠든 별빛이 자라는 섬"), C, 1900.f);
}

void AYJJWorldBuilder::BuildWaterfall()
{
	// 폭포 정원: 높은 곳. 계단식 절벽 위 호수에서 폭포가 하늘 아래로 쏟아진다
	const FVector C = W(-7700, -1900, 2800);
	AYJJProp* Island = BuildIsland(C, 2100.f, IslandColor::GrassWater, IslandColor::RockWarm, 4, true, false);
	USceneComponent* R = Island->GetRootComponent();
	UMaterialInterface* Cliff = YJJShapes::Surface(Island, IslandColor::GrassWater, IslandColor::RockWarm);
	// 계단식 대지 (점프로 오른다: 단 높이 160 이하)
	YJJShapes::Add(Island, R, EYJJShape::Cylinder, FVector(-500, -300, 75), FVector(2000, 2000, 150), Cliff, true);
	YJJShapes::Add(Island, R, EYJJShape::Cylinder, FVector(-700, -400, 225), FVector(1400, 1400, 150), Cliff, true);
	YJJShapes::Add(Island, R, EYJJShape::Cylinder, FVector(-850, -500, 375), FVector(900, 900, 150), Cliff, true);
	// 오르는 바위 계단
	for (int32 i = 0; i < 4; ++i)
	{
		const float A = -0.6f + i * 0.45f;
		YJJShapes::Add(Island, R, EYJJShape::Cylinder, FVector(-500 + FMath::Cos(A) * 1080.f, -300 + FMath::Sin(A) * 1080.f, 40.f + i * 40.f), FVector(260, 260, 80.f + i * 80.f), Cliff, true);
	}
	// 호수 + 폭포 (동쪽 끝으로 흘러내린다)
	YJJShapes::Add(Island, R, EYJJShape::Cylinder, FVector(600, 400, 8), FVector(1300, 1100, 10), YJJShapes::Water(Island, 0.4f, 0.85f), false);
	YJJShapes::Add(Island, R, EYJJShape::Cube, FVector(1500, 450, 4), FVector(900, 420, 10), YJJShapes::Water(Island, 1.2f, 0.85f), false);
	static const TArray<FLinearColor> Flowers = { FLinearColor(1.f, 1.f, 1.f), FLinearColor(0.6f, 0.8f, 1.f), FLinearColor(1.f, 0.85f, 0.3f) };
	Decorate(Island, 2100.f, IslandColor::GrassWater, 4, 220, Flowers);
	AddWaterfall(C + FVector(2000, 450, 6), 0.f, 380.f, 3600.f);
	AddWaterfall(C + FVector(-1100, 1500, 380), 120.f, 200.f, 1800.f);

	AddTree(OnGround(C + FVector(900, -1100, 0)), 1.2f, 21, IslandColor::LeafBright);
	AddTree(OnGround(C + FVector(-1500, 800, 0)), 1.f, 22, IslandColor::LeafGold);
	AddPine(OnGround(C + FVector(200, -1500, 0)), 1.2f, IslandColor::LeafDeep);
	AddRock(OnGround(C + FVector(1200, 1200, 0)), 240, 23, IslandColor::RockWarm);
	AddPillar(OnGround(C + FVector(-850, -500, 0)) + FVector(400, 0, 0), 380, 45, true, IslandColor::Marble);

	AYJJLantern* Lantern = SpawnDeferred<AYJJLantern>(GetWorld(), OnGround(C + FVector(-850, -500, 0)), FRotator(0, 20, 0));
	Lantern->Setup(TEXT("폭포 정원의 등불"), FLinearColor(0.45f, 0.75f, 1.f));
	Finish(Lantern);

	for (int32 i = 0; i < 2; ++i)
	{
		AYJJSlime* Slime = SpawnDeferred<AYJJSlime>(GetWorld(), OnGround(C + FVector(700 - i * 300, -700 + i * 1500, 0)));
		Slime->Setup(FLinearColor(0.4f, 0.8f, 0.9f), 700.f, 0.9f);
		Finish(Slime);
	}

	// 폭포를 따라 내려가는 별가루
	AddStardustLine(C + FVector(2300, 450, -300), C + FVector(2300, 450, -2600), 5);
	AddStardustLine(C + FVector(-500, 700, 300), C + FVector(-850, -500, 600), 4);

	// 보물: 둘째 단 대지 끝 (등불과 떨어진 곳)
	AddChest(OnGround(C + FVector(-200, 0, 0)), 219.f);

	AddZone(TEXT("폭포 정원"), TEXT("하늘 아래로 흐르는 강"), C, 2200.f);
}

void AYJJWorldBuilder::BuildForest()
{
	// 고목의 숲: 낮은 곳. 거대한 고목을 나선 발판으로 올라 꼭대기 등불을 켠다
	const FVector C = W(2500, -8200, -900);
	AYJJProp* Island = BuildIsland(C, 2300.f, IslandColor::GrassForest, IslandColor::RockDark, 5, true, false);
	static const TArray<FLinearColor> Flowers = { FLinearColor(1.f, 0.9f, 0.5f), FLinearColor(0.95f, 0.6f, 0.4f) };
	Decorate(Island, 2300.f, IslandColor::GrassForest, 5, 150, Flowers);

	// 고목
	AYJJProp* Tree = NewProp(C);
	USceneComponent* R = Tree->GetRootComponent();
	UMaterialInterface* Bark = YJJShapes::Surface(Tree, FLinearColor(0.5f, 0.42f, 0.3f), FLinearColor(0.42f, 0.3f, 0.22f));
	UMaterialInterface* Leaf = YJJShapes::Surface(Tree, FLinearColor(0.45f, 0.78f, 0.35f), FLinearColor(0.2f, 0.48f, 0.3f), 0.03f, 0.9f);
	UMaterialInterface* Moss = YJJShapes::Surface(Tree, IslandColor::GrassForest, FLinearColor(0.42f, 0.32f, 0.24f));
	YJJShapes::Add(Tree, R, EYJJShape::Cylinder, FVector(0, 0, 1300), FVector(560, 560, 2600), Bark, true);
	YJJShapes::Add(Tree, R, EYJJShape::Cone, FVector(0, 0, 200), FVector(1100, 1100, 400), Bark, true);
	for (int32 i = 0; i < 5; ++i)
	{
		const float A = i * (TWO_PI / 5.f) + 0.3f;
		YJJShapes::Add(Tree, R, EYJJShape::Cylinder, FVector(FMath::Cos(A) * 420.f, FMath::Sin(A) * 420.f, 80), FVector(160, 160, 520), Bark, true, FRotator(-50, FMath::RadiansToDegrees(A), 0));
	}
	// 나선 발판 (버섯 선반 모양). 점프 높이가 약 195라 한 단 165씩 올라간다
	UMaterialInterface* Cap = YJJShapes::Surface(Tree, FLinearColor(0.95f, 0.75f, 0.45f), FLinearColor(0.85f, 0.55f, 0.35f), 0.08f);
	for (int32 i = 0; i < 16; ++i)
	{
		const float A = i * 0.62f;
		const float Z = 125.f + i * 165.f;
		const FVector P(FMath::Cos(A) * 480.f, FMath::Sin(A) * 480.f, Z);
		YJJShapes::Add(Tree, R, EYJJShape::Cylinder, P, FVector(440, 440, 50), Moss, true, FRotator(0, FMath::RadiansToDegrees(A), 0));
		// 선반 아래 버섯 갓 장식
		YJJShapes::Add(Tree, R, EYJJShape::Sphere, P + FVector(0, 0, -30), FVector(400, 400, 90), Cap, false);
		if (i % 3 == 0)
		{
			AddStardust(C + P + FVector(0, 0, 150));
		}
	}
	// 꼭대기 넓은 가지 + 큰 잎 덩어리
	YJJShapes::Add(Tree, R, EYJJShape::Cylinder, FVector(0, 0, 2620), FVector(1100, 1100, 60), Moss, true);
	const FVector Canopy[] = { FVector(0, 0, 3200), FVector(700, 300, 2900), FVector(-600, 500, 3000), FVector(200, -700, 2950), FVector(-400, -500, 3300) };
	for (int32 i = 0; i < 5; ++i)
	{
		const float S = (i == 0) ? 1500.f : 1000.f;
		// 위쪽만 남기고 가지 위를 걸을 수 있게 잎은 충돌 없음
		YJJShapes::Add(Tree, R, EYJJShape::Sphere, Canopy[i] + FVector(0, 0, 300), FVector(S, S, S * 0.6f), Leaf, false);
	}
	// 매달린 빛 열매
	for (int32 i = 0; i < 8; ++i)
	{
		const float A = i * 0.8f;
		YJJShapes::Add(Tree, R, EYJJShape::Sphere, FVector(FMath::Cos(A) * 650.f, FMath::Sin(A) * 650.f, 2800.f - (i % 3) * 120.f), FVector(50, 50, 60), YJJShapes::Solid(Tree, IslandColor::LeafGold, 3.f), false);
	}

	AYJJLantern* Lantern = SpawnDeferred<AYJJLantern>(GetWorld(), C + FVector(250, 0, 2650), FRotator(0, 180, 0));
	Lantern->Setup(TEXT("고목의 등불"), FLinearColor(0.7f, 1.f, 0.45f));
	Finish(Lantern);
	// 지름길 버섯
	AYJJBouncePad* Pad = SpawnDeferred<AYJJBouncePad>(GetWorld(), OnGround(C + FVector(-900, -700, 0)));
	Pad->Setup(2100.f, 1.1f, FLinearColor(0.75f, 0.45f, 1.f));
	Finish(Pad);

	// 주변 숲
	const int32 Trees[][3] = { { -1700, 600, 1 }, { 1500, 1300, 2 }, { 1700, -900, 3 }, { -1200, -1500, 4 }, { 300, 1800, 5 }, { -1800, -300, 6 } };
	for (const auto& T : Trees)
	{
		if (T[2] % 2)
		{
			AddPine(OnGround(C + FVector(T[0], T[1], 0)), 1.3f + (T[2] % 3) * 0.2f, IslandColor::LeafDeep);
		}
		else
		{
			AddTree(OnGround(C + FVector(T[0], T[1], 0)), 1.2f, 30 + T[2], IslandColor::LeafGold);
		}
	}
	AddRock(OnGround(C + FVector(1000, 700, 0)), 260, 41, IslandColor::RockDark);

	for (int32 i = 0; i < 4; ++i)
	{
		const float A = i * HALF_PI + 0.4f;
		AYJJSlime* Slime = SpawnDeferred<AYJJSlime>(GetWorld(), OnGround(C + FVector(FMath::Cos(A) * 1300.f, FMath::Sin(A) * 1300.f, 0)));
		Slime->Setup(i % 2 ? FLinearColor(0.95f, 0.7f, 0.3f) : FLinearColor(0.55f, 0.8f, 0.4f), 800.f, 0.9f + (i % 2) * 0.35f);
		Finish(Slime);
	}

	// 동쪽 상승 기류 -> 하늘 유적
	AYJJUpdraft* Draft = SpawnDeferred<AYJJUpdraft>(GetWorld(), OnGround(C + FVector(1900, 900, 0)));
	Draft->Setup(330.f, 6600.f);
	Finish(Draft);
	AddStardustLine(C + FVector(1900, 900, 1500), C + FVector(1900, 900, 6000), 5);

	// 보물: 고목 꼭대기 가지 위
	AddChest(C + FVector(-380, 380, 2650), -45.f);

	AddZone(TEXT("고목의 숲"), TEXT("천 년을 자란 나무의 그늘"), C, 2400.f);
}

void AYJJWorldBuilder::BuildRuins()
{
	// 하늘 유적: 가장 높은 곳. 무너진 신전 탑을 점프로 올라 꼭대기 등불
	const FVector C = W(9600, -5300, 4300);
	AYJJProp* Island = BuildIsland(C, 1900.f, IslandColor::GrassRuins, IslandColor::RockCool, 6, true, false);
	static const TArray<FLinearColor> Flowers = { FLinearColor(1.f, 0.85f, 0.3f), FLinearColor(1.f, 1.f, 1.f) };
	Decorate(Island, 1900.f, IslandColor::GrassRuins, 6, 120, Flowers);

	AYJJProp* Temple = NewProp(C);
	USceneComponent* R = Temple->GetRootComponent();
	UMaterialInterface* Stone = YJJShapes::Surface(Temple, IslandColor::Marble, FLinearColor(0.7f, 0.7f, 0.74f));
	UMaterialInterface* Gold = YJJShapes::Solid(Temple, FLinearColor(0.9f, 0.68f, 0.28f), 0.15f, 0.35f);
	// 바닥 타일
	YJJShapes::Add(Temple, R, EYJJShape::Cube, FVector(0, 0, 15), FVector(1800, 1800, 30), Stone, true);
	// 탑: 점점 좁아지는 다섯 층 (층 높이 150, 계단 블록으로 올라간다)
	for (int32 i = 0; i < 5; ++i)
	{
		const float S = 1300.f - i * 220.f;
		YJJShapes::Add(Temple, R, EYJJShape::Cube, FVector(0, 0, 30 + i * 300.f + 150.f), FVector(S, S, 300), Stone, true, FRotator(0, i * 12.f, 0));
		YJJShapes::Add(Temple, R, EYJJShape::Cube, FVector(0, 0, 30 + i * 300.f + 305.f), FVector(S + 40.f, S + 40.f, 14), Gold, false, FRotator(0, i * 12.f, 0));
		// 층마다 바깥에 떠 있는 디딤 블록 두 개 (아래층 -> 위층)
		const float A = i * 1.7f;
		const float Out = S * 0.5f + 160.f;
		const FVector Dir(FMath::Cos(A), FMath::Sin(A), 0.f);
		YJJShapes::Add(Temple, R, EYJJShape::Cube, Dir * Out + FVector(0, 0, 30 + i * 300.f + 100.f), FVector(240, 240, 60), Stone, true, FRotator(0, FMath::RadiansToDegrees(A), 0));
		YJJShapes::Add(Temple, R, EYJJShape::Cube, Dir * (Out - 60.f) + FVector(0, 0, 30 + i * 300.f + 230.f), FVector(200, 200, 60), Stone, true, FRotator(0, FMath::RadiansToDegrees(A), 0));
		AddStardust(C + Dir * Out + FVector(0, 0, 30 + i * 300.f + 260.f));
	}
	// 꼭대기 지붕 기둥
	const float TopZ = 30 + 5 * 300.f;
	for (int32 i = 0; i < 4; ++i)
	{
		const float A = i * HALF_PI + PI / 4.f + 48.f * PI / 180.f;
		YJJShapes::Add(Temple, R, EYJJShape::Cylinder, FVector(FMath::Cos(A) * 330.f, FMath::Sin(A) * 330.f, TopZ + 200.f), FVector(70, 70, 400), Stone, true);
	}
	YJJShapes::Add(Temple, R, EYJJShape::Cone, FVector(0, 0, TopZ + 540.f), FVector(1050, 1050, 300), Gold, false, FRotator(0, 48, 0));

	AYJJLantern* Lantern = SpawnDeferred<AYJJLantern>(GetWorld(), C + FVector(0, 0, TopZ), FRotator(0, 140, 0));
	Lantern->Setup(TEXT("하늘 유적의 등불"), FLinearColor(1.f, 0.82f, 0.35f));
	Finish(Lantern);

	// 둘레 무너진 기둥
	for (int32 i = 0; i < 6; ++i)
	{
		const float A = i * (TWO_PI / 6.f) + 0.2f;
		AddPillar(OnGround(C + FVector(FMath::Cos(A) * 1350.f, FMath::Sin(A) * 1350.f, 0)), 300.f + (i % 3) * 180.f, 50.f, i % 2 == 0, IslandColor::Marble);
	}
	for (int32 i = 0; i < 2; ++i)
	{
		AYJJSlime* Slime = SpawnDeferred<AYJJSlime>(GetWorld(), OnGround(C + FVector(-1100 + i * 2100, 900 - i * 1800, 0)));
		Slime->Setup(FLinearColor(0.75f, 0.5f, 1.f), 500.f, 1.3f);
		Finish(Slime);
	}
	// 탑 둘레를 도는 별가루
	AddStardustRing(C + FVector(0, 0, TopZ + 700.f), 900.f, 6);

	// 보물: 탑 뒤 그늘
	AddChest(OnGround(C + FVector(750, 850, 0)), 200.f);

	AddZone(TEXT("하늘 유적"), TEXT("바람을 섬기던 옛 신전"), C, 2000.f);
}

void AYJJWorldBuilder::BuildVista()
{
	// 먼 풍경: 닿을 수 없는 큰 섬들, 떠 있는 고리, 거대한 수정 (충돌 없음)
	struct FFar { float X, Y, Z, R; int32 Seed; bool bTrees; };
	const FFar Far[] = {
		{ 26000, 9000, 2500, 5200, 101, true },
		{ -22000, 16000, 5500, 4200, 102, true },
		{ -30000, -12000, -1500, 6500, 103, true },
		{ 12000, -30000, 1500, 4800, 104, true },
		{ 34000, -16000, 8000, 3600, 105, false },
		{ -6000, 32000, 1000, 5600, 106, true },
		{ 4000, 22000, 9000, 2400, 107, false },
		{ -16000, -26000, 7000, 2800, 108, false },
	};
	for (const FFar& F : Far)
	{
		const FLinearColor Grass = (F.Seed % 2) ? IslandColor::GrassMeadow : IslandColor::GrassForest;
		BuildIsland(W(F.X, F.Y, F.Z), F.R, Grass, IslandColor::RockWarm, F.Seed, false, false);
		if (F.bTrees)
		{
			// 멀리 보이는 실루엣만 (큰 나무 몇 그루)
			for (int32 i = 0; i < 5; ++i)
			{
				const float A = YJJShapes::Hash(F.Seed * 7 + i) * TWO_PI;
				const float D = YJJShapes::Hash(F.Seed * 9 + i) * F.R * 0.7f;
				AddTree(W(F.X + FMath::Cos(A) * D, F.Y + FMath::Sin(A) * D, F.Z), 3.f + YJJShapes::Hash(i) * 2.f, F.Seed + i, i % 2 ? IslandColor::LeafPink : IslandColor::LeafBright);
			}
		}
		// 큰 섬 일부엔 폭포
		if (F.Seed % 3 == 0)
		{
			AddWaterfall(W(F.X + F.R * 0.95f, F.Y, F.Z), 0.f, 900.f, 9000.f);
		}
	}

	// 하늘에 떠 있는 거대한 고리 유적
	AYJJProp* Ring = NewProp(W(-14000, 4000, 6000), FRotator(0, 30, 70));
	UMaterialInterface* RingMat = YJJShapes::Surface(Ring, IslandColor::Marble, FLinearColor(0.7f, 0.72f, 0.78f), 0.1f);
	for (int32 i = 0; i < 24; ++i)
	{
		if (i == 7 || i == 8 || i == 16)
		{
			continue; // 무너진 틈
		}
		const float A = i * (TWO_PI / 24.f);
		YJJShapes::Add(Ring, Ring->GetRootComponent(), EYJJShape::Cube, FVector(FMath::Cos(A) * 4000.f, FMath::Sin(A) * 4000.f, 0), FVector(1100, 500, 400), RingMat, false, FRotator(0, FMath::RadiansToDegrees(A) + 90.f, 0));
	}
	YJJShapes::Add(Ring, Ring->GetRootComponent(), EYJJShape::Sphere, FVector::ZeroVector, FVector(900, 900, 900), YJJShapes::Glow(Ring, FLinearColor(0.5f, 0.85f, 1.f), 1.5f, 0.6f, 1.f), false);

	// AI로 만든 떠 있는 성소 섬 (TRELLIS.2). 크기는 메시 경계로 맞춘다
	if (UStaticMesh* ShrineMesh = LoadObject<UStaticMesh>(nullptr, YJJAssetPaths::AIFloatingShrine))
	{
		struct FAIIsland { float X, Y, Z, Width, Yaw; };
		const FAIIsland AIIslands[] = {
			{ 5200, 9200, 2400, 3200, 200 },
			{ -15500, -5000, 3800, 6000, 70 },
			{ 16000, 15000, 5200, 4800, 240 },
			{ -9000, 18000, -1800, 4000, 140 },
		};
		const FBoxSphereBounds B = ShrineMesh->GetBounds();
		const float MeshWidth = FMath::Max(1.f, 2.f * FMath::Max(B.BoxExtent.X, B.BoxExtent.Y));
		for (const FAIIsland& I : AIIslands)
		{
			AYJJProp* Isle = NewProp(W(I.X, I.Y, I.Z), FRotator(0, I.Yaw, 0));
			UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(Isle);
			Comp->SetMobility(EComponentMobility::Static);
			Comp->SetStaticMesh(ShrineMesh);
			const float S = I.Width / MeshWidth;
			Comp->SetRelativeScale3D(FVector(S));
			Comp->SetRelativeLocation(-B.Origin * S);
			Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Comp->SetupAttachment(Isle->GetRootComponent());
			Comp->RegisterComponent();
			Isle->AddInstanceComponent(Comp);
			// 사실적인 AI 텍스처 대신 만화 재질: 위를 보는 면은 잔디, 옆은 따뜻한 바위 (모양은 AI 그대로)
			UMaterialInterface* Toon = YJJShapes::Surface(Isle, IslandColor::GrassMeadow, IslandColor::RockWarm);
			for (int32 Slot = 0; Slot < Comp->GetNumMaterials(); ++Slot)
			{
				Comp->SetMaterial(Slot, Toon);
			}
			// 바닥: 다른 섬들처럼 둥근 바위 방울
			const float Bottom = -B.BoxExtent.Z * S;
			YJJShapes::Add(Isle, Isle->GetRootComponent(), EYJJShape::Sphere, FVector(0, 0, Bottom - I.Width * 0.08f), FVector(I.Width * 0.8f, I.Width * 0.8f, I.Width * 0.4f), YJJShapes::Surface(Isle, IslandColor::RockWarm, YJJShapes::Shade(IslandColor::RockWarm, 0.88f)), false);
			YJJShapes::Add(Isle, Isle->GetRootComponent(), EYJJShape::Sphere, FVector(0, 0, Bottom - I.Width * 0.3f), FVector(I.Width * 0.42f, I.Width * 0.42f, I.Width * 0.3f), YJJShapes::Surface(Isle, IslandColor::RockWarm, YJJShapes::Shade(IslandColor::RockWarm, 0.88f)), false);
		}
		UE_LOG(LogYJJFantasy, Log, TEXT("AI 성소 섬 %d개 배치 (메시 폭 %.0f)"), UE_ARRAY_COUNT(AIIslands), MeshWidth);
	}

	// 하늘 고래 둘: 하나는 섬들을 크게 감싸 돌고, 하나는 더 멀리서 반대쪽으로
	{
		struct FWhale { float Radius, Z, Period, Length, Start; };
		const FWhale Whales[] = { { 13000, 3200, 170, 4200, 200 }, { 24000, 7500, 260, 6500, 20 } };
		for (const FWhale& Wh : Whales)
		{
			AYJJSkyWhale* Whale = SpawnDeferred<AYJJSkyWhale>(GetWorld(), W(0, 0, Wh.Z));
			Whale->Setup(W(0, 0, Wh.Z), Wh.Radius, Wh.Period, Wh.Length, Wh.Start);
			Finish(Whale);
		}
	}

	// 먼 거대 수정
	AddCrystals(W(20000, 24000, 3000), 9000.f, FLinearColor(0.45f, 0.75f, 1.f), 9, false);
	AddCrystals(W(-26000, -2000, 9000), 6000.f, FLinearColor(0.8f, 0.55f, 1.f), 10, false);

	// 작은 떠다니는 바위 무리
	for (int32 i = 0; i < 40; ++i)
	{
		const float A = YJJShapes::Hash(i * 17) * TWO_PI;
		const float D = 9000.f + YJJShapes::Hash(i * 19) * 9000.f;
		const float Z = -2000.f + YJJShapes::Hash(i * 23) * 9000.f;
		AYJJProp* Rock = NewProp(W(FMath::Cos(A) * D, FMath::Sin(A) * D, Z), FRotator(YJJShapes::Hash(i) * 40.f, YJJShapes::Hash(i + 1) * 360.f, 0));
		const float S = 150.f + YJJShapes::Hash(i * 29) * 500.f;
		// 잔디 모자를 쓴 둥근 바위
		YJJShapes::Add(Rock, Rock->GetRootComponent(), EYJJShape::Sphere, FVector(0, 0, -S * 0.2f), FVector(S * 1.9f, S * 1.9f, S * 1.4f), YJJShapes::Surface(Rock, IslandColor::RockWarm, YJJShapes::Shade(IslandColor::RockWarm, 0.85f)), false);
		YJJShapes::Add(Rock, Rock->GetRootComponent(), EYJJShape::Sphere, FVector(0, 0, S * 0.32f), FVector(S * 1.7f, S * 1.7f, S * 0.6f), YJJShapes::Surface(Rock, IslandColor::GrassHub, YJJShapes::Shade(IslandColor::GrassHub, 0.85f)), false);
	}

	// 몽실몽실한 뭉게구름: 섬 사이와 아래에 떠 있다 (구 여러 개를 뭉쳐서)
	for (int32 i = 0; i < 46; ++i)
	{
		const float A = YJJShapes::Hash(i * 41 + 3) * TWO_PI;
		const float D = 3500.f + YJJShapes::Hash(i * 43 + 5) * 22000.f;
		// 절반은 섬보다 아래(발밑 구름), 절반은 위아래로 고르게
		const float Z = (i % 2 == 0) ? -2500.f - YJJShapes::Hash(i * 47) * 3000.f : -1500.f + YJJShapes::Hash(i * 53) * 9000.f;
		const FVector P = W(FMath::Cos(A) * D, FMath::Sin(A) * D, Z);
		// 섬과 겹치면 건너뛴다
		if (FVector::Dist2D(P, W(0, 0, 0)) < 3200.f)
		{
			continue;
		}
		AYJJProp* Cloud = NewProp(P, FRotator(0, YJJShapes::Hash(i * 59) * 360.f, 0));
		UMaterialInterface* Mat = YJJShapes::Surface(Cloud, FLinearColor(1.f, 1.f, 1.f), FLinearColor(0.78f, 0.84f, 0.98f), 0.35f, 1.f);
		const float S = 500.f + YJJShapes::Hash(i * 61) * 1300.f;
		const int32 Puffs = 4 + i % 4;
		for (int32 k = 0; k < Puffs; ++k)
		{
			const float X = (k - (Puffs - 1) * 0.5f) * S * 0.55f;
			const float PS = S * (k == Puffs / 2 ? 1.f : 0.6f + 0.35f * YJJShapes::Hash(i * 67 + k));
			const float Y = (YJJShapes::Hash(i * 71 + k) - 0.5f) * S * 0.4f;
			YJJShapes::Add(Cloud, Cloud->GetRootComponent(), EYJJShape::Sphere, FVector(X, Y, PS * 0.15f), FVector(PS, PS * 0.85f, PS * 0.65f), Mat, false);
			// 셋 중 하나는 가운데 덩어리에 멍한 얼굴
			if (k == Puffs / 2 && i % 3 == 0)
			{
				YJJShapes::AddFace(Cloud, Cloud->GetRootComponent(), FVector(X, Y - PS * 0.415f, PS * 0.2f), FRotator(0, -90.f, 0), PS * 0.32f, (i / 3) % 3);
			}
		}
		// 납작한 바닥
		YJJShapes::Add(Cloud, Cloud->GetRootComponent(), EYJJShape::Cylinder, FVector(0, 0, -S * 0.05f), FVector(S * Puffs * 0.55f, S * 0.7f, S * 0.12f), Mat, false);
	}

	// 발밑 만화 구름 바다: 볼류메트릭 구름 대신 납작한 구름 덩어리를 넓게 깐다
	{
		AYJJProp* Sea = NewProp(W(0, 0, -3600));
		UMaterialInterface* Mat = YJJShapes::Surface(Sea, FLinearColor(1.f, 1.f, 1.f), FLinearColor(0.8f, 0.86f, 1.f), 0.3f, 1.f);
		UInstancedStaticMeshComponent* Puffs = YJJShapes::AddInstanced(Sea, Sea->GetRootComponent(), EYJJShape::Sphere, Mat, false);
		Puffs->SetCastShadow(false);
		for (int32 i = 0; i < 220; ++i)
		{
			const float A = YJJShapes::Hash(i * 131 + 7) * TWO_PI;
			const float D = 2000.f + FMath::Sqrt(YJJShapes::Hash(i * 137 + 9)) * 42000.f;
			const float S = 2600.f + YJJShapes::Hash(i * 139) * 3800.f;
			const float Z = (YJJShapes::Hash(i * 149) - 0.5f) * 900.f;
			Puffs->AddInstance(FTransform(FRotator(0, A * 57.f, 0), FVector(FMath::Cos(A) * D, FMath::Sin(A) * D, Z), FVector(S / 100.f, S * 0.8f / 100.f, S * 0.32f / 100.f)));
		}
	}
}
