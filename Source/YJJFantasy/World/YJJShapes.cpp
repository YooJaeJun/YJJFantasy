#include "World/YJJShapes.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "YJJAssetPaths.h"

TMap<FString, TWeakObjectPtr<UObject>> YJJShapes::AssetCache;
TArray<YJJShapes::FCollider>* YJJShapes::ColliderSink = nullptr;

UMaterialInstanceDynamic* YJJShapes::MakeMID(UObject* Outer, const TCHAR* Path)
{
	UMaterialInterface* Parent = LoadCached<UMaterialInterface>(Path);
	if (!Parent)
	{
		Parent = LoadCached<UMaterialInterface>(YJJAssetPaths::FallbackMaterial);
	}
	return Parent ? UMaterialInstanceDynamic::Create(Parent, Outer) : nullptr;
}

EYJJShape YJJShapes::ShapeFromName(const FString& Name)
{
	if (Name == TEXT("Sphere"))
	{
		return EYJJShape::Sphere;
	}
	if (Name == TEXT("Cylinder"))
	{
		return EYJJShape::Cylinder;
	}
	if (Name == TEXT("Cone"))
	{
		return EYJJShape::Cone;
	}
	if (Name == TEXT("Plane"))
	{
		return EYJJShape::Plane;
	}
	return EYJJShape::Cube;
}

UStaticMesh* YJJShapes::Mesh(EYJJShape Shape)
{
	switch (Shape)
	{
	case EYJJShape::Cube: return LoadCached<UStaticMesh>(YJJAssetPaths::Cube);
	case EYJJShape::Sphere: return LoadCached<UStaticMesh>(YJJAssetPaths::Sphere);
	case EYJJShape::Cylinder: return LoadCached<UStaticMesh>(YJJAssetPaths::Cylinder);
	case EYJJShape::Cone: return LoadCached<UStaticMesh>(YJJAssetPaths::Cone);
	case EYJJShape::Plane: return LoadCached<UStaticMesh>(YJJAssetPaths::Plane);
	}
	return nullptr;
}

UMaterialInstanceDynamic* YJJShapes::Surface(UObject* Outer, const FLinearColor& Top, const FLinearColor& Side, float Emissive, float Roughness)
{
	UMaterialInstanceDynamic* MID = MakeMID(Outer, YJJAssetPaths::SurfaceMaterial);
	if (MID)
	{
		MID->SetVectorParameterValue(TEXT("TopColor"), Top);
		MID->SetVectorParameterValue(TEXT("SideColor"), Side);
		MID->SetVectorParameterValue(TEXT("Color"), Top); // 머티리얼이 없을 때 쓰는 엔진 기본 머티리얼용
		MID->SetScalarParameterValue(TEXT("Emissive"), Emissive);
		MID->SetScalarParameterValue(TEXT("Roughness"), Roughness);
	}
	return MID;
}

UMaterialInstanceDynamic* YJJShapes::Solid(UObject* Outer, const FLinearColor& Color, float Emissive, float Roughness)
{
	return Surface(Outer, Color, Color, Emissive, Roughness);
}

UMaterialInstanceDynamic* YJJShapes::Glow(UObject* Outer, const FLinearColor& Color, float Intensity, float Core, float Rim)
{
	UMaterialInstanceDynamic* MID = MakeMID(Outer, YJJAssetPaths::GlowMaterial);
	if (MID)
	{
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		MID->SetScalarParameterValue(TEXT("Intensity"), Intensity);
		MID->SetScalarParameterValue(TEXT("Core"), Core);
		MID->SetScalarParameterValue(TEXT("Rim"), Rim);
		MID->SetScalarParameterValue(TEXT("Fade"), 1.f);
	}
	return MID;
}

UMaterialInstanceDynamic* YJJShapes::Water(UObject* Outer, float Speed, float Opacity)
{
	UMaterialInstanceDynamic* MID = MakeMID(Outer, YJJAssetPaths::WaterMaterial);
	if (MID)
	{
		MID->SetScalarParameterValue(TEXT("Speed"), Speed);
		MID->SetScalarParameterValue(TEXT("Opacity"), Opacity);
	}
	return MID;
}

UStaticMeshComponent* YJJShapes::Add(AActor* Owner, USceneComponent* Parent, EYJJShape Shape, const FVector& Location, const FVector& Size,
	UMaterialInterface* Material, bool bCollide, const FRotator& Rotation)
{
	check(Owner);
	UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(Owner);
	Comp->SetStaticMesh(Mesh(Shape));
	Comp->SetMobility(Parent ? Parent->Mobility.GetValue() : EComponentMobility::Movable);
	if (Parent)
	{
		Comp->SetupAttachment(Parent);
	}
	Comp->SetRelativeLocation(Location);
	Comp->SetRelativeRotation(Rotation);
	Comp->SetRelativeScale3D(Size / BaseShapeSize);
	if (Material)
	{
		Comp->SetMaterial(0, Material);
	}
	if (bCollide)
	{
		Comp->SetCollisionProfileName(TEXT("BlockAll"));
	}
	else
	{
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCanEverAffectNavigation(false);
	}
	// 반투명/가산 머티리얼은 그림자를 만들지 않는다
	if (Material && Material->GetBlendMode() != BLEND_Opaque && Material->GetBlendMode() != BLEND_Masked)
	{
		Comp->SetCastShadow(false);
		Comp->bAffectDistanceFieldLighting = false;
		Comp->bAffectDynamicIndirectLighting = false;
	}
	Comp->RegisterComponent();
	Owner->AddInstanceComponent(Comp);
	if (bCollide && ColliderSink)
	{
		ColliderSink->Add({ Shape, Comp->GetComponentTransform() });
	}
	return Comp;
}

void YJJShapes::SetColliderSink(TArray<FCollider>* Sink)
{
	ColliderSink = Sink;
}

bool YJJShapes::RayCast(const FCollider& Collider, const FVector& Start, const FVector& End, float& OutT, FVector& OutNormal)
{
	// 도형의 로컬 공간(지름 100, 가운데 피벗)으로 옮기면 크기·회전이 사라져 단위 도형과의 교차가 된다.
	// 선형 변환이라 선분의 매개변수 t는 그대로다.
	const FTransform& X = Collider.Transform;
	const FVector P = X.InverseTransformPosition(Start);
	const FVector D = X.InverseTransformPosition(End) - P;
	const float R = BaseShapeSize * 0.5f;
	float BestT = NoHit;
	FVector BestN = FVector::ZeroVector;

	auto Consider = [&](float T, const FVector& N)
	{
		if (T >= 0.f && T <= 1.f && T < BestT)
		{
			BestT = T;
			BestN = N;
		}
	};

	switch (Collider.Shape)
	{
	case EYJJShape::Cube:
	case EYJJShape::Plane:
	{
		const float HalfZ = Collider.Shape == EYJJShape::Plane ? PlaneHalfThickness : R;
		const FVector Half(R, R, HalfZ);
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			for (int32 Sign = -1; Sign <= 1; Sign += 2)
			{
				if (FMath::IsNearlyZero(D[Axis]))
				{
					continue;
				}
				const float T = (Sign * Half[Axis] - P[Axis]) / D[Axis];
				const FVector Q = P + D * T;
				const int32 A1 = (Axis + 1) % 3;
				const int32 A2 = (Axis + 2) % 3;
				if (FMath::Abs(Q[A1]) <= Half[A1] && FMath::Abs(Q[A2]) <= Half[A2])
				{
					FVector N = FVector::ZeroVector;
					N[Axis] = Sign;
					Consider(T, N);
				}
			}
		}
		break;
	}
	case EYJJShape::Sphere:
	{
		const float A = D.SizeSquared();
		const float B = 2.f * FVector::DotProduct(P, D);
		const float C = P.SizeSquared() - R * R;
		const float Disc = B * B - 4.f * A * C;
		if (Disc >= 0.f && A > 0.f)
		{
			const float T = (-B - FMath::Sqrt(Disc)) / (2.f * A);
			Consider(T, (P + D * T).GetSafeNormal());
		}
		break;
	}
	case EYJJShape::Cylinder:
	{
		// 윗뚜껑·아랫뚜껑
		if (!FMath::IsNearlyZero(D.Z))
		{
			for (int32 Sign = -1; Sign <= 1; Sign += 2)
			{
				const float T = (Sign * R - P.Z) / D.Z;
				const FVector Q = P + D * T;
				if (Q.X * Q.X + Q.Y * Q.Y <= R * R)
				{
					Consider(T, FVector(0, 0, Sign));
				}
			}
		}
		// 옆면
		const float A = D.X * D.X + D.Y * D.Y;
		const float B = 2.f * (P.X * D.X + P.Y * D.Y);
		const float C = P.X * P.X + P.Y * P.Y - R * R;
		const float Disc = B * B - 4.f * A * C;
		if (A > KINDA_SMALL_NUMBER && Disc >= 0.f)
		{
			const float T = (-B - FMath::Sqrt(Disc)) / (2.f * A);
			const FVector Q = P + D * T;
			if (FMath::Abs(Q.Z) <= R)
			{
				Consider(T, FVector(Q.X, Q.Y, 0).GetSafeNormal());
			}
		}
		break;
	}
	case EYJJShape::Cone:
	{
		// 밑면 z=-50 반지름 50, 꼭짓점 z=+50. 옆면: x²+y² = (k(50 - z))², k = 0.5
		const float K = ConeSlope;
		if (!FMath::IsNearlyZero(D.Z))
		{
			const float T = (-R - P.Z) / D.Z;
			const FVector Q = P + D * T;
			if (Q.X * Q.X + Q.Y * Q.Y <= R * R)
			{
				Consider(T, FVector(0, 0, -1));
			}
		}
		const float H0 = R - P.Z;
		const float A = D.X * D.X + D.Y * D.Y - K * K * D.Z * D.Z;
		const float B = 2.f * (P.X * D.X + P.Y * D.Y + K * K * H0 * D.Z);
		const float C = P.X * P.X + P.Y * P.Y - K * K * H0 * H0;
		const float Disc = B * B - 4.f * A * C;
		if (!FMath::IsNearlyZero(A) && Disc >= 0.f)
		{
			const float S = FMath::Sqrt(Disc);
			for (const float T : { (-B - S) / (2.f * A), (-B + S) / (2.f * A) })
			{
				const FVector Q = P + D * T;
				if (Q.Z >= -R && Q.Z <= R)
				{
					const float Rad = FMath::Sqrt(Q.X * Q.X + Q.Y * Q.Y);
					const FVector N = FVector(Q.X / FMath::Max(Rad, MinConeRadius), Q.Y / FMath::Max(Rad, MinConeRadius), K).GetSafeNormal();
					Consider(T, N);
				}
			}
		}
		break;
	}
	}

	if (BestT > 1.f)
	{
		return false;
	}
	OutT = BestT;
	// 법선은 역전치로: 회전 x (법선 / 스케일)
	const FVector S = X.GetScale3D();
	OutNormal = X.GetRotation().RotateVector(FVector(BestN.X / S.X, BestN.Y / S.Y, BestN.Z / S.Z)).GetSafeNormal();
	return true;
}

UStaticMesh* YJJShapes::AIMesh(const TCHAR* Name)
{
	// build_content.py(Interchange)가 /AI/<이름>/<이름>/StaticMeshes/<이름> 으로 임포트한다
	const FString Path = FString::Printf(TEXT("%s/%s/%s/StaticMeshes/%s.%s"), YJJAssetPaths::AIModelDir, Name, Name, Name, Name);
	return LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
}

void YJJShapes::AddFace(AActor* Owner, USceneComponent* Parent, const FVector& Location, const FRotator& Rotation, float Width, int32 Mood)
{
	UMaterialInterface* Ink = Solid(Owner, Face.InkColor, 0.f, Face.InkRoughness);
	UMaterialInterface* Pink = Solid(Owner, Face.BlushColor, Face.BlushEmissive);
	const float W = Width;
	auto At = [&](float Y, float Z) { return Location + Rotation.RotateVector(FVector(0.f, Y * W, Z * W)); };
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		FVector EyeSize = Face.EyeDot * W; // 세로 점
		if (Mood == 1)
		{
			EyeSize = Face.EyeSleepy * W; // 졸린 가로 선
		}
		else if (Mood == 2)
		{
			EyeSize = Face.EyeRound * W; // 동그란 눈
		}
		Add(Owner, Parent, EYJJShape::Sphere, At(Side * Face.EyeSpread, Face.EyeUp), EyeSize, Ink, false, Rotation);
		Add(Owner, Parent, EYJJShape::Sphere, At(Side * Face.BlushSpread, Face.BlushUp), Face.BlushSize * W, Pink, false, Rotation);
		// ω 입: 작은 반원 두 개를 기울여 붙인다
		Add(Owner, Parent, EYJJShape::Sphere, At(Side * Face.MouthSpread, Face.MouthUp), Face.MouthSize * W, Ink, false, Rotation + FRotator(0.f, 0.f, Side * Face.MouthTilt));
	}
}

UStaticMesh* YJJShapes::CharacterMesh(const TCHAR* Name)
{
	const FString Path = FString::Printf(TEXT("/Game/YJJFantasy/Characters/%s/%s/StaticMeshes/%s.%s"), Name, Name, Name, Name);
	return LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
}

UStaticMeshComponent* YJJShapes::AddAI(AActor* Owner, USceneComponent* Parent, UStaticMesh* AI, const FVector& Location, float Size,
	bool bBottomPivot, const FRotator& Rotation)
{
	check(Owner && AI);
	const FBoxSphereBounds B = AI->GetBounds();
	const float Width = FMath::Max(1.f, 2.f * FMath::Max(B.BoxExtent.X, B.BoxExtent.Y));
	const float S = Size / Width;
	const FVector Pivot = bBottomPivot ? B.Origin - FVector(0, 0, B.BoxExtent.Z) : B.Origin;

	UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(Owner);
	Comp->SetStaticMesh(AI);
	Comp->SetMobility(Parent ? Parent->Mobility.GetValue() : EComponentMobility::Movable);
	if (Parent)
	{
		Comp->SetupAttachment(Parent);
	}
	Comp->SetRelativeScale3D(FVector(S));
	Comp->SetRelativeRotation(Rotation);
	Comp->SetRelativeLocation(Location - Rotation.RotateVector(Pivot * S));
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Comp->SetCanEverAffectNavigation(false);
	Comp->RegisterComponent();
	Owner->AddInstanceComponent(Comp);
	return Comp;
}

UInstancedStaticMeshComponent* YJJShapes::AddInstanced(AActor* Owner, USceneComponent* Parent, EYJJShape Shape, UMaterialInterface* Material, bool bCollide)
{
	check(Owner);
	UInstancedStaticMeshComponent* Comp = NewObject<UInstancedStaticMeshComponent>(Owner);
	Comp->SetStaticMesh(Mesh(Shape));
	Comp->SetMobility(Parent ? Parent->Mobility.GetValue() : EComponentMobility::Movable);
	if (Parent)
	{
		Comp->SetupAttachment(Parent);
	}
	if (Material)
	{
		Comp->SetMaterial(0, Material);
	}
	if (bCollide)
	{
		Comp->SetCollisionProfileName(TEXT("BlockAll"));
	}
	else
	{
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCanEverAffectNavigation(false);
	}
	Comp->RegisterComponent();
	Owner->AddInstanceComponent(Comp);
	return Comp;
}

float YJJShapes::Hash(int32 Seed)
{
	// PCG 해시 (Jarzynski & Olano, 2020)
	uint32 X = static_cast<uint32>(Seed) * PcgMultiplier + PcgIncrement;
	X = ((X >> ((X >> PcgShiftSelect) + PcgShiftBase)) ^ X) * PcgOutputMultiplier;
	X = (X >> PcgFinalShift) ^ X;
	return static_cast<float>(X & HashMask) / static_cast<float>(HashMask);
}

FLinearColor YJJShapes::Shade(const FLinearColor& Color, float Scale)
{
	return FLinearColor(Color.R * Scale, Color.G * Scale, Color.B * Scale, Color.A);
}
