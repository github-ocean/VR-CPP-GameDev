 // Fill out your copyright notice in the Description page of Project Settings.

#include "VRCharacter.h"

// Sets default values
AVRCharacter::AVRCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRRoot"));
	VRRoot->SetupAttachment(GetRootComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VRRoot);

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DestinationMarker"));
	DestinationMarker->SetupAttachment(GetRootComponent());

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComponent"));
	PostProcessComponent->SetupAttachment(GetRootComponent());

	LeftController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftController"));
	LeftController->SetupAttachment(VRRoot);
	LeftController->SetTrackingSource(EControllerHand::Left);
	LeftController->bDisplayDeviceModel = true;

	RightController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightController"));
	RightController->SetupAttachment(VRRoot);
	RightController->SetTrackingSource(EControllerHand::Right);
	RightController->bDisplayDeviceModel = true;

	TeleportPath = CreateDefaultSubobject<USplineComponent>(TEXT("TeleportPath"));
	TeleportPath->SetupAttachment(RightController);

}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();

	DestinationMarker->SetVisibility(false);

	if (BlinkerMaterialBase != nullptr)
	{
		BlinkerMaterialInstance = UMaterialInstanceDynamic::Create(BlinkerMaterialBase, this);
		PostProcessComponent->AddOrUpdateBlendable(BlinkerMaterialInstance);
	}

}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector NewCameraOffset = Camera->GetComponentLocation() - GetActorLocation();
	NewCameraOffset.Z = 0;
	AddActorWorldOffset(NewCameraOffset);
	VRRoot->AddWorldOffset(-NewCameraOffset);

	UpdateDestinationMarker();
	UpdateBlinker();
}

bool AVRCharacter::FindTeleportDestination( TArray<FVector> &OutPath, FVector& OutLocation)
{
	FVector Start = RightController->GetComponentLocation();
	FVector Look = RightController->GetForwardVector();
	//Look = Look.RotateAngleAxis(30, RightController->GetRightVector());

	FPredictProjectilePathParams Params(
		TeleportProjectileRadius,
		Start,
		Look * TeleportProjectileSpeed,
		TeleportSimulationTime,
		ECollisionChannel::ECC_Visibility,
		this
	);
	Params.DrawDebugType = EDrawDebugTrace::ForOneFrame;
	Params.bTraceComplex = true;

	FPredictProjectilePathResult Result;
	bool bHit = UGameplayStatics::PredictProjectilePath(this, Params, Result);
	
	if (!bHit) return false;

	for (FPredictProjectilePathPointData PointData : Result.PathData)
	{
		OutPath.Add(PointData.Location);
	}

	FNavLocation NavLocation;
	bool bOnNavMesh = UNavigationSystemV1::GetCurrent(GetWorld())->ProjectPointToNavigation(Result.HitResult.Location, NavLocation, TeleportProjectionExtent);
	if (!bOnNavMesh) return false;

	OutLocation = NavLocation.Location;

	return true;
}

void AVRCharacter::UpdateDestinationMarker()
{
	TArray<FVector> Path;
	FVector Location;
	bool bHasDestination = FindTeleportDestination(Path, Location);
	

	if (bHasDestination)
	{
		DestinationMarker->SetVisibility(true);
		DestinationMarker->SetWorldLocation(Location);
		
		DrawTeleportPath(Path);
	}
	else
	{
		DestinationMarker->SetVisibility(false);
	}
}

void AVRCharacter::DrawTeleportPath(const TArray<FVector> &Path)
{
	UpdateSpline(Path);

	for (int32 i = 0; i < Path.Num(); ++i)
	{
		if (TeleportPathMeshPool.Num() <= i)
		{
			UStaticMeshComponent* DynamicMesh = NewObject<UStaticMeshComponent>(this);
			DynamicMesh->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
			DynamicMesh->SetStaticMesh(TeleportArchMesh);
			DynamicMesh->SetMaterial(0, TeleportArchMaterial);
			DynamicMesh->RegisterComponent();
			TeleportPathMeshPool.Add(DynamicMesh);
		}

		UStaticMeshComponent* DynamicMesh = TeleportPathMeshPool[i];
		DynamicMesh->SetWorldLocation(Path[i]);
	}
}

void AVRCharacter::UpdateSpline(const TArray<FVector>& Path)
{
	TeleportPath->ClearSplinePoints(false);
	
	for (int32 i = 0; i < Path.Num(); ++i)
	{
		FVector LocalPosition = TeleportPath->GetComponentTransform().InverseTransformPosition(Path[i]);
		FSplinePoint Point(i, LocalPosition, ESplinePointType::Curve);
		TeleportPath->AddPoint(Point, false);
	}

	TeleportPath->UpdateSpline();
}

FVector2D AVRCharacter::GetBlinkerCenter()
{
	FVector MovementDirection = GetVelocity().GetSafeNormal();
	if (MovementDirection.IsNearlyZero())
	{
		return FVector2D(0.5, 0.5);
	}

	FVector WorldStationaryLocation;
	if (FVector::DotProduct(Camera->GetForwardVector(), MovementDirection))
	{
		WorldStationaryLocation = Camera->GetComponentLocation() + MovementDirection * 1000;
	}
	else
	{
		WorldStationaryLocation = Camera->GetComponentLocation() - MovementDirection * 1000;
	}

	APlayerController* Pc = Cast<APlayerController>(GetController());
	if (Pc == nullptr)
	{
		return FVector2D(0.5, 0.5);
	}

	FVector2D ScreenStationaryLocation;
	Pc->ProjectWorldLocationToScreen(WorldStationaryLocation, ScreenStationaryLocation);

	int32 SizeX, SizeY;
	Pc->GetViewportSize(SizeX, SizeY);
	ScreenStationaryLocation.X /= SizeX;
	ScreenStationaryLocation.Y /= SizeY;

	return ScreenStationaryLocation;
}

void AVRCharacter::UpdateBlinker()
{
	if (RadiusVsVelocity == nullptr) return;

	float Speed = GetVelocity().Size();
	float Radius = RadiusVsVelocity->GetFloatValue(Speed);

	BlinkerMaterialInstance->SetScalarParameterValue(TEXT("Radius"), Radius);

	FVector2D Center = GetBlinkerCenter();
	BlinkerMaterialInstance->SetVectorParameterValue(TEXT("Center"), FLinearColor(Center.X, Center.Y, 0));
}

// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("Forward"), this, &AVRCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("Right"), this, &AVRCharacter::MoveRight);
	PlayerInputComponent->BindAction(TEXT("Teleport"), IE_Pressed, this, &AVRCharacter::BegineTeleport);

}

void AVRCharacter::MoveForward(float Throttle)
{
	AddMovementInput(Camera->GetForwardVector() * Throttle);
}

void AVRCharacter::MoveRight(float Throttle)
{
	AddMovementInput(Camera->GetRightVector() * Throttle);
}

void AVRCharacter::BegineTeleport() 
{
	StartFade(0, 1);

	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &AVRCharacter::FinishTeleport, TeleportFadeTime);
}

void AVRCharacter::FinishTeleport()
{
	SetActorLocation(DestinationMarker->GetComponentLocation());

	StartFade(1, 0);
}

void AVRCharacter::StartFade(float FromAlpha, float ToAlpha)
{
	APlayerController* pc = Cast<APlayerController>(GetController());
	if (pc != nullptr)
	{
		pc->PlayerCameraManager->StartCameraFade(FromAlpha, ToAlpha, TeleportFadeTime, FLinearColor::Black);
	}
}