// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GameEvent.h"
#include<concurrent_queue.h>
#include "ResourceManager.generated.h"

UCLASS()
class PROJECT_API AResourceManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AResourceManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere)
		class AMain* Main;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSubclassOf<AActor>OilActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSubclassOf<AActor>WaterActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSubclassOf<AActor>IronActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSubclassOf<AActor>FoodActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSubclassOf<AActor>WoodActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSubclassOf<AActor>GameEventActor;

	AActor* resources[50];
	int resource_amount[50]{};	//resource.cpp�� ����⺸�� ���⼭ ���� ��������
	char resource_type[50]{};	//type�� ��������
	char workCitizens[50]{};		// ��� ���ϴ°͵� ��������

	//resourceManager�� ��Ȱ�� �ʱ� ������ �Ұ� ������ gameEvent�� ���� �������� ������ gameEvent�� �ڿ�ȹ���� �� �����̴ϱ�
	AActor* GameEvent[200];
	typedef struct s_resource {
		int Resource_id;
		FVector Location;
		int amount; 
		char resourcetype;
	}s_resource;
	typedef struct s_event {
		int e_id;
		FVector Location;
	};


	concurrency::concurrent_queue<s_resource> resourceCreateQueue;
	concurrency::concurrent_queue<s_event> EventCreateQueue;
	concurrency::concurrent_queue<int> EventRemoveQueue;

	bool eventreset;
	class FSocketThread* Network;
	void Spawn_Resource(int Resource_id, FVector Location, int amount, char resourcetype);
	void SetResourceAmount(int Resource_id, int amount);
	void SetResourcePlacement(int Resource_id, char work_citizen);
	void Spawn_Event(int e_id, FVector Location);
	void remove_Event(int e_id);
	void Set_Resource_Queue(int Resource_id, FVector Location, int amount, char resourcetype);
	void Set_Event_Queue(int e_id, FVector Location);
	void Set_Remove_Event_Queue(int e_id);
};
