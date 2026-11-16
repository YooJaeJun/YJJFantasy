#include "World/YJJSkyWhale.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "World/YJJShapes.h"

AYJJSkyWhale::AYJJSkyWhale()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);
}

void AYJJSkyWhale::Setup(const FVector& InCenter, float InRadius, float InPeriod, float InLength, float InStartAngle)
{
	Center = InCenter;
	Radius = InRadius;
	Period = InPeriod;
	Length = InLength;
	Angle = FMath::DegreesToRadians(InStartAngle);
}

void AYJJSkyWhale::BeginPlay()
{
	Super::BeginPlay();
	Body = NewObject<USceneComponent>(this);
	Body->SetMobility(EComponentMobility::Movable);
	Body->SetupAttachment(RootComponent);
	Body->RegisterComponent();
	AddInstanceComponent(Body);
	if (UStaticMesh* Mesh = YJJShapes::AIMesh(TEXT("SM_SkyWhale")))
	{
		UStaticMeshComponent* Comp = YJJShapes::AddAI(this, Body, Mesh, FVector::ZeroVector, Length, false, FRotator(0, MeshYawOffset, 0));
		Comp->SetCastShadow(false); // 멀리 떠 있어서 섬에 그림자를 드리우면 어색하다
	}
	else
	{
		SetActorHiddenInGame(true);
		SetActorTickEnabled(false);
	}
	Tick(0.f);
}

void AYJJSkyWhale::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Time += DeltaTime;
	Angle += DeltaTime * TWO_PI / Period;

	// 원 궤도 + 느린 출렁임
	const float Bob = FMath::Sin(Time * 0.35f) * 450.f;
	const FVector P = Center + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, Bob);
	// 진행 방향 = 원의 접선 (반시계)
	const float Yaw = FMath::RadiansToDegrees(Angle) + 90.f;
	const float Pitch = FMath::Cos(Time * 0.35f) * 6.f; // 오를 때 고개를 든다
	const float Roll = -8.f;                             // 원 안쪽으로 기울기
	SetActorLocationAndRotation(P, FRotator(Pitch, Yaw, Roll));

	// 헤엄치는 몸짓: 몸 전체를 살짝 위아래로 흔든다
	if (Body)
	{
		Body->SetRelativeRotation(FRotator(FMath::Sin(Time * 1.1f) * 3.f, FMath::Sin(Time * 0.55f) * 2.f, 0));
	}
}
