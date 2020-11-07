// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SplineComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "NavigationSystem.h"
#include "MotionControllerComponent.h"
#include "VRCharacter.generated.h"

UCLASS()
class VRCPPGAMEDEV_API AVRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AVRCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:

	UPROPERTY()
	class UCameraComponent* Camera;

	UPROPERTY()
	class UMotionControllerComponent* LeftController;

	UPROPERTY()
	class UMotionControllerComponent* RightController;

	UPROPERTY(VisibleAnywhere)
	class USceneComponent* VRRoot;

	UPROPERTY(VisibleAnywhere)
	class USplineComponent* TeleportPath;

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* DestinationMarker;

	UPROPERTY(EditAnywhere)
	float TeleportProjectileRadius = 10;

	UPROPERTY(EditAnywhere)
	float TeleportProjectileSpeed = 800;

	UPROPERTY(EditAnywhere)
		float TeleportSimulationTime = 1;

	UPROPERTY(EditAnywhere)
	float TeleportFadeTime = 1;

	UPROPERTY(EditAnywhere)
	FVector TeleportProjectionExtent = FVector(100, 100, 100);

	UPROPERTY()
	class UPostProcessComponent* PostProcessComponent;

	UPROPERTY(VisibleAnywhere)
	class UMaterialInterface* BlinkerMaterialBase;

	UPROPERTY()
	class UMaterialInstanceDynamic* BlinkerMaterialInstance;

	UPROPERTY(VisibleAnywhere)
	class UCurveFloat* RadiusVsVelocity;

	UPROPERTY()
	TArray<class UStaticMeshComponent*> TeleportPathMeshPool;

	UPROPERTY(EditDefaultsOnly)
	UStaticMesh* TeleportArchMesh;

	UPROPERTY(EditDefaultsOnly)
	UMaterialInterface* TeleportArchMaterial;

public:

	void MoveForward(float Throttle);
	void MoveRight(float Throttle);

	bool FindTeleportDestination(TArray<FVector> &OutPath, FVector& OutLocation);
	void UpdateDestinationMarker();

	FVector2D GetBlinkerCenter();
	void UpdateBlinker();

	void DrawTeleportPath(const TArray<FVector> &Path);
	void UpdateSpline(const TArray<FVector> &Path);

	void BegineTeleport();
	void FinishTeleport();

	void StartFade(float FromAlpha, float ToAlpha);

};
