// Fill out your copyright notice in the Description page of Project Settings.


#include "CitizenManager.h"
#include "Kismet/GameplayStatics.h"
#include "Main.h"
#include "NetworkingThread.h"


// Sets default values
ACitizenManager::ACitizenManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;
    UWorld* worldref = GetWorld();
    if (worldref == nullptr) {
        return;
    }

    AActor* actor = UGameplayStatics::GetActorOfClass(worldref, AMain::StaticClass());
    if (actor == nullptr) {
        return;
    }
    actor->GetWorld();
    Main = Cast<AMain>(actor);
    Network = nullptr;
}

// Called when the game starts or when spawned
void ACitizenManager::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ACitizenManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    if (Network == nullptr)
    {
        if(Main == nullptr)
        {
            UE_LOG(LogTemp, Log, TEXT("Manager connect"));
        }
        Network = reinterpret_cast<FSocketThread*>(Main->Network);
        Network->_CitizenManager = this;
        UE_LOG(LogTemp, Log, TEXT("Manager connect"));
    }

    sc_packet_citizencreate CreateCItizen;
    while (!CitizenQueue.empty()) {
        if (CitizenQueue.try_pop(CreateCItizen))
        {
            Spawn_Citizen(CreateCItizen);
        }
    }


    sc_packet_armytraining create_army;
    while (!Army_Queue.empty())
    {
        if (Army_Queue.try_pop(create_army))
        {
            Spawn_Army(create_army);
        }
    }

    sc_packet_citizenmove move_citizen;
    while (!Citizen_Move_Queue.empty())
    {
        if (Citizen_Move_Queue.try_pop(move_citizen))
        {
            FRotator Rotation = (FVector(move_citizen.rx, move_citizen.ry, move_citizen.rz)).GetSafeNormal().Rotation();
            Set_Citizen_Location(move_citizen.citizenid - CITIZENSTART, FVector(move_citizen.x, move_citizen.y, move_citizen.z),Rotation ,move_citizen.citizenstate);
        }
    }

    sc_packet_armymove move_army;
       
    while (!Army_Move_Queue.empty())
    {
        if (Army_Move_Queue.try_pop(move_army))
        {
            FRotator Rotation = (FVector(move_army.rx, move_army.ry, move_army.rz)).GetSafeNormal().Rotation();
            Set_Army_Location(move_army.a_id - ARMYSTART, FVector(move_army.x, move_army.y, move_army.z), Rotation, move_army.a_state);
        }
    }
    sc_packet_citizenremove remove_citizen;
    while (!Citizen_Remove_Queue.empty())
    {
        if (Citizen_Remove_Queue.try_pop(remove_citizen))
        {
            Remove_Citizen(remove_citizen.citizenid - CITIZENSTART);
        }
    }

	sc_packet_armyattack attack_army;
	while (!Army_Attack_Queue.empty())
	{
		if (Army_Attack_Queue.try_pop(attack_army))
		{
            FRotator Rotation = (FVector(attack_army.rx, attack_army.ry, attack_army.rz)).GetSafeNormal().Rotation();
            Set_Army_Attack(attack_army.army_id - ARMYSTART, Rotation, attack_army.a_state);
		}
	}


}

void ACitizenManager::Spawn_Citizen(sc_packet_citizencreate CreateCItizen)
{
	int citizen_id = CreateCItizen.citizenid - CITIZENSTART;
	FVector Location = FVector(CreateCItizen.x, CreateCItizen.y, CreateCItizen.z);
        
    FActorSpawnParameters SpawnInfo;
    UWorld* uworld = GetWorld();
    if (uworld == nullptr)
        return;
    if (citizen[citizen_id] == nullptr)
    {
        if (Network->my_id == (citizen_id / 200))
            citizen[citizen_id] = uworld->SpawnActor<ACitizen>(MyCitizen_MODEL, Location, FRotator(0.0f, 0.0f, 0.0f), SpawnInfo);
        else  if (Network->my_id != (citizen_id / 200))
            citizen[citizen_id] = uworld->SpawnActor<ACitizen>(EnemyCitizen_MODEL, Location, FRotator(0.0f, 0.0f, 0.0f), SpawnInfo);
    }
    else
    {
        citizen[citizen_id]->SetActorHiddenInGame(false);
        citizen[citizen_id]->SetActorLocation(Location);
    }

    ACitizen* citi = reinterpret_cast<ACitizen*>(citizen[citizen_id]);
    citi->_id = citizen_id;
}

void ACitizenManager::Set_Citizen_Location(int citizen_id, FVector Location, FRotator Rotate, char citizenstate)
{
    if(citizen[citizen_id] != nullptr)
    {
        if (citizen[citizen_id]->GetWorld() && citizen[citizen_id]->IsValidLowLevel())
        {
            citizen[citizen_id]->SetActorLocation(Location);
            citizen[citizen_id]->SetActorRotation(Rotate);
            ACitizen* citi = reinterpret_cast<ACitizen*>(citizen[citizen_id]);
            citi->state = citizenstate;
            citi->CitizenRotation = Rotate;
        }
    }
}

void ACitizenManager::Remove_Citizen(int citizen_id)
{
    if (citizen[citizen_id] != nullptr)
    {
        citizen[citizen_id]->Destroy();
        citizen[citizen_id] = nullptr;
    }
}

void ACitizenManager::Spawn_Army(sc_packet_armytraining spawn_packet)
{
    sc_packet_armytraining* packet = reinterpret_cast<sc_packet_armytraining*>(&spawn_packet);
    citizen[packet->c_id1 - CITIZENSTART]->SetActorHiddenInGame(true);
    citizen[packet->c_id2 - CITIZENSTART]->SetActorHiddenInGame(true);
    citizen[packet->c_id3 - CITIZENSTART]->SetActorHiddenInGame(true);
    citizen[packet->c_id4 - CITIZENSTART]->SetActorHiddenInGame(true);
    citizen[packet->c_id5 - CITIZENSTART]->SetActorHiddenInGame(true);
    FVector Location{ packet->x,packet->y,packet->z };
    FActorSpawnParameters SpawnInfo;
    UWorld* uworld = GetWorld();
    if(packet->_army_type == 0)
    {
        army[packet->army_id - ARMYSTART] = uworld->SpawnActor<AActor>(ARMY_MODEL, Location, FRotator(0.0f, 0.0f, 0.0f), SpawnInfo);
        army[packet->army_id - ARMYSTART]->Tags.Add(TEXT("Army"));
        army[packet->army_id - ARMYSTART]->Tags.Add(*FString::FromInt(0));
        
    }
    else if (packet->_army_type == 1)
    {
        army[packet->army_id - ARMYSTART] = uworld->SpawnActor<AActor>(ARMY_MODEL1, Location, FRotator(0.0f, 0.0f, 0.0f), SpawnInfo);
        army[packet->army_id - ARMYSTART]->Tags.Add(TEXT("Army"));
        army[packet->army_id - ARMYSTART]->Tags.Add(*FString::FromInt(packet->_army_type));

    }
    else if (packet->_army_type == 2)
    {
        army[packet->army_id - ARMYSTART] = uworld->SpawnActor<AActor>(ARMY_MODEL2, Location, FRotator(0.0f, 0.0f, 0.0f), SpawnInfo);
        army[packet->army_id - ARMYSTART]->Tags.Add(TEXT("Army"));
        army[packet->army_id - ARMYSTART]->Tags.Add(*FString::FromInt(packet->_army_type));

    }
    
    army[packet->army_id - ARMYSTART]->Tags.Add(*FString::FromInt(packet->army_id - ARMYSTART));
    army[packet->army_id - ARMYSTART]->Tags.Add(*FString::FromInt((packet->army_id - ARMYSTART) / (ARMYMAX / 5)));
    AArmy* army_class = reinterpret_cast<AArmy*>(army[packet->army_id - ARMYSTART]);
    army_class->Army_HP = packet->hp;
}

void ACitizenManager::Set_Army_Location(int a_id, FVector Location, FRotator Rotate, char a_state)
{
    if (army[a_id] != nullptr)
    {
        if (army[a_id]->GetWorld() && army[a_id]->IsValidLowLevel())
        {
            army[a_id]->SetActorLocation(Location);
            army[a_id]->SetActorRotation(Rotate);
            AArmy* a = reinterpret_cast<AArmy*>(army[a_id]);
            a->state = a_state;
        }
    }

}

void ACitizenManager::Set_Army_Disband(int a_id)
{
    if (army[a_id] != nullptr)
    {
        AArmy* army_class = reinterpret_cast<AArmy*>(army[a_id]);
        army_class->state = 4;
    }

}

void ACitizenManager::Set_Army_Hp(int Hp, int a_id)
{
    if (army[a_id] != nullptr)
    {
        AArmy* army_class = reinterpret_cast<AArmy*>(army[a_id]);
        army_class->Army_HP = Hp;
    }
}

void ACitizenManager::Army_Dead(int a_id)
{
    if (army[a_id] != nullptr)
    {
        AArmy* army_class = reinterpret_cast<AArmy*>(army[a_id]);
        army_class->Army_HP = 0;
        army_class->state = 3;
    }
}

void ACitizenManager::Set_Army_Attack(int a_id, FRotator Rotate, int a_state)
{
    if (army[a_id] != nullptr)
    {
        army[a_id]->SetActorRotation(Rotate);
        AArmy* a = reinterpret_cast<AArmy*>(army[a_id]);
        a->state = a_state;
    }
}

void ACitizenManager::Set_Army_Queue(void* packet)
{
    sc_packet_armytraining* p_army_packet = reinterpret_cast<sc_packet_armytraining*>(packet);
    Army_Queue.push(*p_army_packet);
}

void ACitizenManager::Set_Citizen_Move_Queue(sc_packet_citizenmove* packet)
{
    Citizen_Move_Queue.push(*packet);
}

void ACitizenManager::Set_Army_Move_Queue(sc_packet_armymove* packet)
{
    Army_Move_Queue.push(*packet);
}

void ACitizenManager::SetCitizenQueue(sc_packet_citizencreate* packet)
{
	CitizenQueue.push(*packet);
}

void ACitizenManager::Set_Citizen_Remove_Queue(sc_packet_citizenremove* packet)
{
    Citizen_Remove_Queue.push(*packet);
}

void ACitizenManager::Set_Army_Attack_Queue(sc_packet_armyattack* packet)
{
    Army_Attack_Queue.push(*packet);
}
