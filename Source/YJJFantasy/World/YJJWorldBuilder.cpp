#include "World/YJJWorldBuilder.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Data/YJJData.h"
#include "Data/YJJLoc.h"
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
#include "World/YJJWorldLayout.h"
#include "YJJFantasy.h"

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
	return W(FYJJData::Tune(TEXT("World.SpawnX")), FYJJData::Tune(TEXT("World.SpawnY")), FYJJData::Tune(TEXT("World.SpawnZ")));
}

float AYJJWorldBuilder::GetSpawnYaw()
{
	return FYJJData::Tune(TEXT("World.SpawnYaw"));
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
		GetWorld()->SpawnActor<AYJJEnvironment>(AYJJEnvironment::StaticClass(), FTransform(W(0.f, 0.f, YJJWorldLayout::EnvironmentZ)));
	}

	YJJShapes::SetColliderSink(&Colliders);
	// 섬 -> 언덕·지형 -> 풀·꽃 -> 나머지 배치 (풀이 지형 위에만 돋게) -> 먼 풍경
	BuildIslands();
	PlaceAll(true);
	DecorateIslands();
	PlaceAll(false);
	BuildVista();
	YJJShapes::SetColliderSink(nullptr);

	GetWorld()->SpawnActor<AYJJMotes>(AYJJMotes::StaticClass(), FTransform::Identity);

	// 영웅: 시작 위치 + 낙사 높이
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (AYJJHero* Hero = Cast<AYJJHero>(PC->GetPawn()))
		{
			Hero->KillZ = BaseZ - FYJJData::Tune(TEXT("World.KillDepth"));
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
	UInstancedStaticMeshComponent* Pebbles = YJJShapes::AddInstanced(Island, R, EYJJShape::Sphere, YJJShapes::Surface(Island, FLinearColor(0.75f, 0.73f, 0.7f), FYJJData::Color(TEXT("RockWarm"))), false);
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

// ============================================================ 섬 구성 (Islands.csv + Placements.csv)

void AYJJWorldBuilder::BuildIslands()
{
	for (const FName& Key : FYJJData::IslandKeys())
	{
		const FYJJIslandRow* Row = FYJJData::Island(Key);
		if (!Row)
		{
			continue;
		}
		AYJJProp* Island = BuildIsland(W(Row->X, Row->Y, Row->Z), Row->Radius, FYJJData::Color(Row->Grass), FYJJData::Color(Row->Rock), Row->Seed, true, false);
		IslandProps.Add(Key, Island);
	}
}

void AYJJWorldBuilder::DecorateIslands()
{
	for (const FName& Key : FYJJData::IslandKeys())
	{
		const FYJJIslandRow* Row = FYJJData::Island(Key);
		const TObjectPtr<AYJJProp>* Island = IslandProps.Find(Key);
		if (!Row || !Island)
		{
			continue;
		}
		Decorate(Island->Get(), Row->Radius, FYJJData::Color(Row->Grass), Row->Seed, Row->FlowerCount, FYJJData::Colors(Row->FlowerColors));
		AddZone(FYJJLoc::Get(*Row->NameKey), FYJJLoc::Get(*Row->SubtitleKey), W(Row->X, Row->Y, Row->Z), Row->ZoneRadius);
	}
}

bool AYJJWorldBuilder::IsEarlyPlacement(const FYJJPlacementRow& Row)
{
	// 꾸미기(풀·꽃) 전에 놓아야 풀이 그 위에 돋는 것
	return Row.Type == TEXT("Hill") || Row.Type == TEXT("Terrain");
}

void AYJJWorldBuilder::PlaceAll(bool bEarly)
{
	for (const FYJJPlacementRow* Row : FYJJData::Placements())
	{
		if (IsEarlyPlacement(*Row) == bEarly)
		{
			Place(*Row);
		}
	}
}

FVector AYJJWorldBuilder::PlacementOrigin(const FYJJPlacementRow& Row) const
{
	if (Row.Island == TEXT("World"))
	{
		return W(0.f, 0.f, 0.f);
	}
	const FYJJIslandRow* Island = FYJJData::Island(FName(*Row.Island));
	return Island ? W(Island->X, Island->Y, Island->Z) : W(0.f, 0.f, 0.f);
}

void AYJJWorldBuilder::Place(const FYJJPlacementRow& Row)
{
	const FVector Origin = PlacementOrigin(Row);
	const FVector Raw = Origin + FVector(Row.X, Row.Y, Row.Z);
	const FVector P = Row.Ground ? OnGround(Raw, Row.Lift) : Raw;
	const FRotator Rot(Row.Pitch, Row.Yaw, Row.Roll);
	const FLinearColor Color = Row.Color.IsEmpty() ? FLinearColor::White : FYJJData::Color(Row.Color);
	const FString& T = Row.Type;

	if (T == TEXT("Hill"))
	{
		const TObjectPtr<AYJJProp>* Island = IslandProps.Find(FName(*Row.Island));
		const FYJJIslandRow* IslandRow = FYJJData::Island(FName(*Row.Island));
		if (Island && IslandRow)
		{
			AddHill(Island->Get(), FVector(Row.X, Row.Y, Row.Z), Row.Size, Row.A, FYJJData::Color(IslandRow->Grass), FYJJData::Color(IslandRow->Rock));
		}
	}
	else if (T == TEXT("Terrain") || T == TEXT("Shape"))
	{
		AddShape(Row, Origin);
	}
	else if (T == TEXT("Shrine"))
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<AYJJShrine>(AYJJShrine::StaticClass(), FTransform(P), Params);
	}
	else if (T == TEXT("Tree"))
	{
		AddTree(P, Row.Scale, Row.Seed, Color);
	}
	else if (T == TEXT("Pine"))
	{
		AddPine(P, Row.Scale, Color);
	}
	else if (T == TEXT("Rock"))
	{
		AddRock(P, Row.Size, Row.Seed, Color);
	}
	else if (T == TEXT("Pillar"))
	{
		AddPillar(P, Row.Size, Row.A, Row.B != 0.f, Color);
	}
	else if (T == TEXT("Arch"))
	{
		AddArch(P, Row.Yaw, Row.Size, Row.A, Color);
	}
	else if (T == TEXT("Stone"))
	{
		AddStone(P, Row.Size, Color, FYJJData::Color(Row.Color2), Row.Seed);
	}
	else if (T == TEXT("Crystals"))
	{
		AddCrystals(P, Row.Size, Color, Row.Seed, Row.B != 0.f);
	}
	else if (T == TEXT("AICrystal"))
	{
		// AI 수정 무리 (없으면 도형 수정, 높이 A). AI 모델만 Lift 만큼 묻는다
		if (UStaticMesh* Cluster = YJJShapes::AIMesh(TEXT("SM_CrystalCluster")))
		{
			AYJJProp* Prop = NewProp(P, FRotator(0, Row.Yaw, 0));
			YJJShapes::Add(Prop, Prop->GetRootComponent(), EYJJShape::Cylinder, YJJWorldLayout::AICrystalColliderCenter, YJJWorldLayout::AICrystalColliderSize, nullptr, true)->SetVisibility(false);
			YJJShapes::AddAI(Prop, Prop->GetRootComponent(), Cluster, FVector::ZeroVector, Row.Size, true);
		}
		else
		{
			AddCrystals(P - FVector(0, 0, Row.Lift), Row.A, Color, Row.Seed, false);
		}
	}
	else if (T == TEXT("AIWindmill"))
	{
		if (UStaticMesh* Windmill = YJJShapes::AIMesh(TEXT("SM_Windmill")))
		{
			AYJJProp* Mill = NewProp(P, FRotator(0, Row.Yaw, 0));
			YJJShapes::Add(Mill, Mill->GetRootComponent(), EYJJShape::Cylinder, YJJWorldLayout::WindmillColliderCenter, YJJWorldLayout::WindmillColliderSize, nullptr, true)->SetVisibility(false);
			YJJShapes::AddAI(Mill, Mill->GetRootComponent(), Windmill, FVector::ZeroVector, Row.Size, true);
		}
	}
	else if (T == TEXT("Waterfall"))
	{
		AddWaterfall(P, Row.Yaw, Row.Size, Row.A);
	}
	else if (T == TEXT("Stardust"))
	{
		AddStardust(P);
	}
	else if (T == TEXT("StardustLine"))
	{
		AddStardustLine(P, Origin + FVector(Row.X2, Row.Y2, Row.Z2), Row.Count, Row.A);
	}
	else if (T == TEXT("StardustRing"))
	{
		AddStardustRing(P, Row.Size, Row.Count);
	}
	else if (T == TEXT("Chest"))
	{
		AddChest(P, Row.Yaw);
	}
	else if (T == TEXT("Lantern"))
	{
		AYJJLantern* Lantern = SpawnDeferred<AYJJLantern>(GetWorld(), P, FRotator(0, Row.Yaw, 0));
		Lantern->Setup(FYJJLoc::Get(*Row.Text), Color);
		Finish(Lantern);
	}
	else if (T == TEXT("Mob"))
	{
		AYJJSlime* Slime = SpawnDeferred<AYJJSlime>(GetWorld(), P);
		Slime->Setup(Color, Row.A, Row.Scale);
		Finish(Slime);
	}
	else if (T == TEXT("BouncePad"))
	{
		AYJJBouncePad* Pad = SpawnDeferred<AYJJBouncePad>(GetWorld(), P);
		Pad->Setup(Row.A, Row.Scale, Color);
		Finish(Pad);
	}
	else if (T == TEXT("Updraft"))
	{
		AYJJUpdraft* Draft = SpawnDeferred<AYJJUpdraft>(GetWorld(), P);
		Draft->Setup(Row.Size, Row.A);
		Finish(Draft);
	}
	else if (T == TEXT("Platform"))
	{
		AYJJMovingPlatform* Platform = SpawnDeferred<AYJJMovingPlatform>(GetWorld(), P);
		Platform->Setup(FVector(Row.X2, Row.Y2, Row.Z2), Row.A, Row.Size, Row.B);
		Finish(Platform);
	}
	else if (T == TEXT("AIShrineIsland"))
	{
		AddAIShrineIsland(P, Row.Size, Row.Yaw);
	}
	else if (T == TEXT("Whale"))
	{
		AYJJSkyWhale* Whale = SpawnDeferred<AYJJSkyWhale>(GetWorld(), P);
		Whale->Setup(P, Row.A, Row.B, Row.Size, Row.Yaw);
		Finish(Whale);
	}
	else if (T == TEXT("Ring"))
	{
		AddRing(Row, P);
	}
	else
	{
		UE_LOG(LogYJJFantasy, Warning, TEXT("[World] Placements.csv %s: 모르는 종류 '%s'"), *Row.Island, *T);
	}
}

void AYJJWorldBuilder::AddShape(const FYJJPlacementRow& Row, const FVector& Origin)
{
	const TObjectPtr<AYJJProp>* IslandProp = IslandProps.Find(FName(*Row.Island));
	AYJJProp* Holder = IslandProp ? IslandProp->Get() : NewProp(Origin);
	const FVector Local = Origin + FVector(Row.X, Row.Y, Row.Z) - Holder->GetActorLocation();
	UMaterialInterface* Mat = nullptr;
	if (Row.Mat == TEXT("Water"))
	{
		Mat = YJJShapes::Water(Holder, Row.A, Row.B);
	}
	else if (Row.Mat == TEXT("Solid"))
	{
		Mat = YJJShapes::Solid(Holder, FYJJData::Color(Row.Color), Row.Emissive, Row.Rough);
	}
	else
	{
		Mat = YJJShapes::Surface(Holder, FYJJData::Color(Row.Color), FYJJData::Color(Row.Color2), Row.Emissive, Row.Rough);
	}
	YJJShapes::Add(Holder, Holder->GetRootComponent(), YJJShapes::ShapeFromName(Row.Text), Local, FVector(Row.SX, Row.SY, Row.SZ), Mat, Row.Collide != 0, FRotator(Row.Pitch, Row.Yaw, Row.Roll));
}

void AYJJWorldBuilder::AddRing(const FYJJPlacementRow& Row, const FVector& Center)
{
	// 하늘에 떠 있는 거대한 고리 유적: Count 칸 중 Text 의 칸은 무너진 틈
	AYJJProp* Ring = NewProp(Center, FRotator(Row.Pitch, Row.Yaw, Row.Roll));
	UMaterialInterface* RingMat = YJJShapes::Surface(Ring, FYJJData::Color(Row.Color), FYJJData::Color(Row.Color2), Row.Emissive);
	TArray<FString> GapText;
	Row.Text.ParseIntoArray(GapText, TEXT(";"));
	TSet<int32> Gaps;
	for (const FString& G : GapText)
	{
		Gaps.Add(FCString::Atoi(*G));
	}
	for (int32 i = 0; i < Row.Count; ++i)
	{
		if (Gaps.Contains(i))
		{
			continue;
		}
		const float A = i * (TWO_PI / Row.Count);
		YJJShapes::Add(Ring, Ring->GetRootComponent(), EYJJShape::Cube, FVector(FMath::Cos(A) * Row.Size, FMath::Sin(A) * Row.Size, 0), FVector(Row.SX, Row.SY, Row.SZ), RingMat, false,
			FRotator(0, FMath::RadiansToDegrees(A) + YJJWorldLayout::RingBlockTurn, 0));
	}
	YJJShapes::Add(Ring, Ring->GetRootComponent(), EYJJShape::Sphere, FVector::ZeroVector, FVector(YJJWorldLayout::RingCoreSize),
		YJJShapes::Glow(Ring, FYJJData::Color(TEXT("RingCore")), YJJWorldLayout::RingCoreIntensity, YJJWorldLayout::RingCoreCore, YJJWorldLayout::RingCoreRim), false);
}

void AYJJWorldBuilder::AddAIShrineIsland(const FVector& Location, float Width, float Yaw)
{
	// AI로 만든 떠 있는 성소 섬 (TRELLIS.2). 크기는 메시 경계로 맞춘다
	UStaticMesh* ShrineMesh = LoadObject<UStaticMesh>(nullptr, YJJAssetPaths::AIFloatingShrine, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!ShrineMesh)
	{
		return;
	}
	AYJJProp* Isle = NewProp(Location, FRotator(0, Yaw, 0));
	UStaticMeshComponent* Comp = YJJShapes::AddAI(Isle, Isle->GetRootComponent(), ShrineMesh, FVector::ZeroVector, Width, false);
	// 사실적인 AI 텍스처 대신 만화 재질: 위를 보는 면은 잔디, 옆은 따뜻한 바위 (모양은 AI 그대로)
	UMaterialInterface* Toon = YJJShapes::Surface(Isle, FYJJData::Color(TEXT("GrassMeadow")), FYJJData::Color(TEXT("RockWarm")));
	for (int32 Slot = 0; Slot < Comp->GetNumMaterials(); ++Slot)
	{
		Comp->SetMaterial(Slot, Toon);
	}
	// 바닥: 다른 섬들처럼 둥근 바위 방울
	const float Bottom = -ShrineMesh->GetBounds().BoxExtent.Z * Comp->GetRelativeScale3D().Z;
	UMaterialInterface* Rock = YJJShapes::Surface(Isle, FYJJData::Color(TEXT("RockWarm")), YJJShapes::Shade(FYJJData::Color(TEXT("RockWarm")), YJJWorldLayout::AIIslandSideShade));
	for (const FVector4& Blob : YJJWorldLayout::AIIslandBottomBlobs)
	{
		YJJShapes::Add(Isle, Isle->GetRootComponent(), EYJJShape::Sphere, FVector(0, 0, Bottom - Width * Blob.X), FVector(Width * Blob.Y, Width * Blob.Y, Width * Blob.Z), Rock, false);
	}
}

void AYJJWorldBuilder::BuildVista()
{
	// 먼 풍경: 닿을 수 없는 큰 섬들 (VistaIslands.csv). 고리·고래·거대 수정·AI 섬은 Placements.csv 의 World 줄
	for (const FYJJVistaIslandRow* F : FYJJData::VistaIslands())
	{
		const FLinearColor Grass = FYJJData::Color((F->Seed % 2) ? TEXT("GrassMeadow") : TEXT("GrassForest"));
		BuildIsland(W(F->X, F->Y, F->Z), F->Radius, Grass, FYJJData::Color(TEXT("RockWarm")), F->Seed, false, false);
		if (F->Trees)
		{
			// 멀리 보이는 실루엣만 (큰 나무 몇 그루)
			for (int32 i = 0; i < YJJWorldLayout::FarTreeCount; ++i)
			{
				const float A = YJJShapes::Hash(F->Seed * YJJWorldLayout::HashFarTreeAngle + i) * TWO_PI;
				const float D = YJJShapes::Hash(F->Seed * YJJWorldLayout::HashFarTreeDist + i) * F->Radius * YJJWorldLayout::FarTreeSpread;
				AddTree(W(F->X + FMath::Cos(A) * D, F->Y + FMath::Sin(A) * D, F->Z), YJJWorldLayout::FarTreeScaleMin + YJJShapes::Hash(i) * YJJWorldLayout::FarTreeScaleRange, F->Seed + i,
					FYJJData::Color(i % 2 ? TEXT("LeafPink") : TEXT("LeafBright")));
			}
		}
		if (F->Seed % YJJWorldLayout::FarWaterfallEvery == 0)
		{
			AddWaterfall(W(F->X + F->Radius * YJJWorldLayout::FarWaterfallEdge, F->Y, F->Z), 0.f, YJJWorldLayout::FarWaterfallWidth, YJJWorldLayout::FarWaterfallDrop);
		}
	}

	// 작은 떠다니는 바위 무리 (잔디 모자를 쓴 둥근 바위)
	for (int32 i = 0; i < YJJWorldLayout::FloatingRockCount; ++i)
	{
		const float A = YJJShapes::Hash(i * YJJWorldLayout::HashRockAngle) * TWO_PI;
		const float D = YJJWorldLayout::FloatingRockDistMin + YJJShapes::Hash(i * YJJWorldLayout::HashRockDist) * YJJWorldLayout::FloatingRockDistRange;
		const float Z = YJJWorldLayout::FloatingRockZMin + YJJShapes::Hash(i * YJJWorldLayout::HashRockZ) * YJJWorldLayout::FloatingRockZRange;
		AYJJProp* Rock = NewProp(W(FMath::Cos(A) * D, FMath::Sin(A) * D, Z), FRotator(YJJShapes::Hash(i) * YJJWorldLayout::FloatingRockTiltMax, YJJShapes::Hash(i + 1) * YJJWorldLayout::FullTurn, 0));
		const float S = YJJWorldLayout::FloatingRockSizeMin + YJJShapes::Hash(i * YJJWorldLayout::HashRockSize) * YJJWorldLayout::FloatingRockSizeRange;
		const FLinearColor RockColor = FYJJData::Color(TEXT("RockWarm"));
		const FLinearColor GrassColor = FYJJData::Color(TEXT("GrassHub"));
		YJJShapes::Add(Rock, Rock->GetRootComponent(), EYJJShape::Sphere, YJJWorldLayout::FloatingRockBody.Offset * S, YJJWorldLayout::FloatingRockBody.Size * S, YJJShapes::Surface(Rock, RockColor, YJJShapes::Shade(RockColor, YJJWorldLayout::FloatingRockSideShade)), false);
		YJJShapes::Add(Rock, Rock->GetRootComponent(), EYJJShape::Sphere, YJJWorldLayout::FloatingRockCap.Offset * S, YJJWorldLayout::FloatingRockCap.Size * S, YJJShapes::Surface(Rock, GrassColor, YJJShapes::Shade(GrassColor, YJJWorldLayout::FloatingRockSideShade)), false);
	}

	// 몽실몽실한 뭉게구름: 섬 사이와 아래에 떠 있다 (구 여러 개를 뭉쳐서)
	const FLinearColor CloudTop = FYJJData::Color(TEXT("CloudTop"));
	for (int32 i = 0; i < YJJWorldLayout::CloudCount; ++i)
	{
		const float A = YJJShapes::Hash(i * YJJWorldLayout::HashCloudAngle + YJJWorldLayout::HashCloudAngleSalt) * TWO_PI;
		const float D = YJJWorldLayout::CloudDistMin + YJJShapes::Hash(i * YJJWorldLayout::HashCloudDist + YJJWorldLayout::HashCloudDistSalt) * YJJWorldLayout::CloudDistRange;
		// 절반은 섬보다 아래(발밑 구름), 절반은 위아래로 고르게
		const float Z = (i % 2 == 0) ? YJJWorldLayout::CloudLowZ - YJJShapes::Hash(i * YJJWorldLayout::HashCloudLow) * YJJWorldLayout::CloudLowRange
		                             : YJJWorldLayout::CloudHighZ + YJJShapes::Hash(i * YJJWorldLayout::HashCloudHigh) * YJJWorldLayout::CloudHighRange;
		const FVector P = W(FMath::Cos(A) * D, FMath::Sin(A) * D, Z);
		if (FVector::Dist2D(P, W(0.f, 0.f, 0.f)) < YJJWorldLayout::CloudKeepOut)
		{
			continue;
		}
		AYJJProp* Cloud = NewProp(P, FRotator(0, YJJShapes::Hash(i * YJJWorldLayout::HashCloudYaw) * YJJWorldLayout::FullTurn, 0));
		UMaterialInterface* Mat = YJJShapes::Surface(Cloud, CloudTop, FYJJData::Color(TEXT("CloudPuffSide")), YJJWorldLayout::CloudPuffEmissive, 1.f);
		const float S = YJJWorldLayout::CloudSizeMin + YJJShapes::Hash(i * YJJWorldLayout::HashCloudSize) * YJJWorldLayout::CloudSizeRange;
		const int32 Puffs = YJJWorldLayout::CloudPuffsMin + i % YJJWorldLayout::CloudPuffsVariety;
		for (int32 k = 0; k < Puffs; ++k)
		{
			const float X = (k - (Puffs - 1) * 0.5f) * S * YJJWorldLayout::CloudPuff.Spacing;
			const float PS = S * (k == Puffs / 2 ? 1.f : YJJWorldLayout::CloudPuff.SideMin + YJJWorldLayout::CloudPuff.SideRange * YJJShapes::Hash(i * YJJWorldLayout::HashCloudPuff + k));
			const float Y = (YJJShapes::Hash(i * YJJWorldLayout::HashCloudPuffY + k) - 0.5f) * S * YJJWorldLayout::CloudPuff.Jitter;
			YJJShapes::Add(Cloud, Cloud->GetRootComponent(), EYJJShape::Sphere, FVector(X, Y, PS * YJJWorldLayout::CloudPuff.Lift), FVector(PS, PS * YJJWorldLayout::CloudPuff.Depth, PS * YJJWorldLayout::CloudPuff.Height), Mat, false);
			// 셋 중 하나는 가운데 덩어리에 멍한 얼굴
			if (k == Puffs / 2 && i % YJJWorldLayout::CloudFaceEvery == 0)
			{
				YJJShapes::AddFace(Cloud, Cloud->GetRootComponent(), FVector(X, Y - PS * YJJWorldLayout::CloudPuff.FaceOut, PS * YJJWorldLayout::CloudPuff.FaceUp), FRotator(0, -YJJWorldLayout::QuarterTurn, 0), PS * YJJWorldLayout::CloudPuff.FaceWidth,
					(i / YJJWorldLayout::CloudFaceEvery) % YJJShapes::FaceMoodCount);
			}
		}
		// 납작한 바닥
		YJJShapes::Add(Cloud, Cloud->GetRootComponent(), EYJJShape::Cylinder, FVector(0, 0, -S * YJJWorldLayout::CloudPuff.BaseDrop), FVector(S * Puffs * YJJWorldLayout::CloudPuff.Spacing, S * YJJWorldLayout::CloudPuff.BaseDepth, S * YJJWorldLayout::CloudPuff.BaseHeight), Mat, false);
	}

	// 발밑 만화 구름 바다: 볼류메트릭 구름 대신 납작한 구름 덩어리를 넓게 깐다
	AYJJProp* Sea = NewProp(W(0.f, 0.f, YJJWorldLayout::CloudSeaZ));
	UMaterialInterface* SeaMat = YJJShapes::Surface(Sea, CloudTop, FYJJData::Color(TEXT("CloudSeaSide")), YJJWorldLayout::CloudEmissive, 1.f);
	UInstancedStaticMeshComponent* SeaPuffs = YJJShapes::AddInstanced(Sea, Sea->GetRootComponent(), EYJJShape::Sphere, SeaMat, false);
	SeaPuffs->SetCastShadow(false);
	for (int32 i = 0; i < YJJWorldLayout::CloudSeaCount; ++i)
	{
		const float A = YJJShapes::Hash(i * YJJWorldLayout::HashSeaAngle + YJJWorldLayout::HashSeaAngleSalt) * TWO_PI;
		const float D = YJJWorldLayout::CloudSeaDistMin + FMath::Sqrt(YJJShapes::Hash(i * YJJWorldLayout::HashSeaDist + YJJWorldLayout::HashSeaDistSalt)) * YJJWorldLayout::CloudSeaDistRange;
		const float S = (YJJWorldLayout::CloudSeaSizeMin + YJJShapes::Hash(i * YJJWorldLayout::HashSeaSize) * YJJWorldLayout::CloudSeaSizeRange) / YJJShapes::BaseShapeSize;
		const float Z = (YJJShapes::Hash(i * YJJWorldLayout::HashSeaZ) - 0.5f) * YJJWorldLayout::CloudSeaBumpZ;
		SeaPuffs->AddInstance(FTransform(FRotator(0, FMath::RadiansToDegrees(A), 0), FVector(FMath::Cos(A) * D, FMath::Sin(A) * D, Z), FVector(S, S * YJJWorldLayout::CloudSeaFlatY, S * YJJWorldLayout::CloudSeaFlatZ)));
	}
}
