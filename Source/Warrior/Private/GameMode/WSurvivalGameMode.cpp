// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMode/WSurvivalGameMode.h"
#include "Characters/WEnemyCharacter.h"
#include "Engine/AssetManager.h"
#include "Engine/TargetPoint.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "WFunctionLibrary.h"

#include "WGameInstance.h"
#include "SaveGame/WGameSaveSubsystem.h"

#include "AbilitySystemInterface.h"
#include "AbilitySystem/WAttributeSet.h"
#include "Components/Combat/PawnCombatComponent.h"
#include "Components/Inventory/WInventoryComponent.h"

#include "WDebugHelper.h"


void AWSurvivalGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	if (UWGameInstance* GameInstance = GetGameInstance<UWGameInstance>())
	{
		// 从存档读取玩家选择的难度（选难度 UI 通过 SaveCurrentGameDifficulty 写入 PlayerData.Difficulty）
		auto LoadDifficultyFromSave = [&]()
		{
			if (UWGameSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UWGameSaveSubsystem>())
			{
				FWPlayerSaveData LoadedData;
				if (SaveSubsystem->TryLoadGame(LoadedData, GameInstance->ActiveSaveSlot))
				{
					CurrentGameDifficulty = LoadedData.Difficulty;
				}
			}
		};

		if (GameInstance->bWantsContinue)
		{
			if (UWGameSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UWGameSaveSubsystem>())
			{
				FWPlayerSaveData LoadedData;

				if (SaveSubsystem->TryLoadGame(LoadedData, GameInstance->ActiveSaveSlot))
				{
					CurrentWaveCount = LoadedData.WaveCount;
					CurrentGameDifficulty = LoadedData.Difficulty;

					// 把角色成长数据交给 HeroCharacter：它出生后从这份数据还原属性/装备
					GameInstance->PendingLoadData = LoadedData;
					GameInstance->bHasPendingPlayerData = true;
				}
				else
				{
					// 无存档却点了「继续」：退回新开局，并清理跨关卡残留标志，避免套用旧/空数据
					CurrentWaveCount = 1;
					GameInstance->bWantsContinue = false;
					GameInstance->bHasPendingPlayerData = false;
				}
			}
			else
			{
				CurrentWaveCount = 1;
				GameInstance->bWantsContinue = false;
				GameInstance->bHasPendingPlayerData = false;
			}
		}
		else
		{
			// 新游戏：从第 1 波开始，但仍读取玩家选择的难度（否则会落到枚举默认值=最低档）
			CurrentWaveCount = 1;
			LoadDifficultyFromSave();
		}
	}
}

void AWSurvivalGameMode::SaveCurrentProgress()
{
	if (UWGameInstance* GameInstance = GetGameInstance<UWGameInstance>())
	{
		if (UWGameSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UWGameSaveSubsystem>())
		{
			FWPlayerSaveData Data;
			Data.WaveCount = CurrentWaveCount;
			Data.Difficulty = CurrentGameDifficulty;

			// 收集英雄成长属性与装备（B 档）
			if (APawn* HeroPawn = UGameplayStatics::GetPlayerPawn(this, 0))
			{
				if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(HeroPawn))
				{
					if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
					{
						if (const UWAttributeSet* AS = ASC->GetSet<UWAttributeSet>())
						{
							Data.MaxHealth = AS->GetMaxHealth();
							Data.MaxRage = AS->GetMaxRage();
							Data.AttackPower = AS->GetAttackPower();
							Data.DefensePower = AS->GetDefensePower();
							Data.CurrentHealth = AS->GetCurrentHealth();
							Data.CurrentRage = AS->GetCurrentRage();
						}
					}
				}

			if (UPawnCombatComponent* PCC = HeroPawn->FindComponentByClass<UPawnCombatComponent>())
			{
				Data.EquippedWeaponTag = PCC->CurrentEquippedWeaponTag;
			}

			// 收集背包物品（Step 12）
			if (UWInventoryComponent* Inv = HeroPawn->FindComponentByClass<UWInventoryComponent>())
			{
				Inv->ExportToSaveData(Data);
			}
			}

			SaveSubsystem->SaveGame(Data, GameInstance->ActiveSaveSlot);
		}
	}
}

void AWSurvivalGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!ensureMsgf(EnemyWaveSpawnerDataTable, TEXT("Forgot to assign a valid data table in survival game mode blueprint"))) return;

	SetCurrentSurvivalGameModeState(EWSurvivalGameModeState::WaitSpawnNewWave);

	TotalWaveSpawn = EnemyWaveSpawnerDataTable->GetRowNames().Num();

	PreLoadNextWaveEnemies();
}

void AWSurvivalGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!EnemyWaveSpawnerDataTable) return;

	if (CurrentGameModeState == EWSurvivalGameModeState::WaitSpawnNewWave)
	{
		TimePassedSinceStart += DeltaTime;

		if (TimePassedSinceStart >= SpawnNewWaveWaitTime)
		{
			TimePassedSinceStart = 0.f;
			SetCurrentSurvivalGameModeState(EWSurvivalGameModeState::SpawningNewWave);
		}
	}

	if (CurrentGameModeState == EWSurvivalGameModeState::SpawningNewWave)
	{
		TimePassedSinceStart += DeltaTime;

		if (TimePassedSinceStart >= SpawnEnemiesDelayTime)
		{
			// TODO: Handle spawn new enemies
			CurrentSpawnedEnemiesCounter += TrySpawnWaveEnemies();

			TimePassedSinceStart = 0.f;
			SetCurrentSurvivalGameModeState(EWSurvivalGameModeState::InProgress);
		}
	}

	if (CurrentGameModeState == EWSurvivalGameModeState::WaveCompleted)
	{
		TimePassedSinceStart += DeltaTime;

		if (TimePassedSinceStart >= WaveCompletedWaitTime)
		{
			TimePassedSinceStart = 0.f;

			CurrentWaveCount++;

			// 一波打完：存下"下一波"编号，下次继续时直接跳到这一波
			SaveCurrentProgress();

			if (HasFinishedAllWaves())
			{
				SetCurrentSurvivalGameModeState(EWSurvivalGameModeState::AllWavesDone);
			}
			else
			{
				SetCurrentSurvivalGameModeState(EWSurvivalGameModeState::WaitSpawnNewWave);

				PreLoadNextWaveEnemies();
			}
		}
	}
}

void AWSurvivalGameMode::SetCurrentSurvivalGameModeState(EWSurvivalGameModeState InState)
{
	CurrentGameModeState = InState;

	// 每进入一波战斗就把当前进度落盘（中途退出可从此波续上）
	if (InState == EWSurvivalGameModeState::InProgress)
	{
		SaveCurrentProgress();
	}

	OnSurvivalGameModeStateChanged.Broadcast(CurrentGameModeState);
}

bool AWSurvivalGameMode::HasFinishedAllWaves() const
{
	return CurrentWaveCount > TotalWaveSpawn;
}

void AWSurvivalGameMode::PreLoadNextWaveEnemies()
{
	if (HasFinishedAllWaves()) return;

	PreLoadedEnemyClassMap.Empty();

	for (const FWEnemyWaveSpawnerInfo& SpawnerInfo : GetCurrentWaveSpawnerTableRow()->EnemyWaveSpawnerDefinitions)
	{
		if (SpawnerInfo.EnemyClassToSpawn.IsNull()) continue;

		UAssetManager::GetStreamableManager().RequestAsyncLoad(
			SpawnerInfo.EnemyClassToSpawn.ToSoftObjectPath(),
			FStreamableDelegate::CreateLambda(
				[SpawnerInfo, this]() {
					if (UClass* LoadedEnemyClass = SpawnerInfo.EnemyClassToSpawn.Get())
					{
						PreLoadedEnemyClassMap.Emplace(SpawnerInfo.EnemyClassToSpawn, LoadedEnemyClass);
						// Debug::Print(LoadedEnemyClass->GetName() + TEXT(" is loaded"));
					}
				}));
	}
}

const FWEnemyWaveSpawnerTableRow* AWSurvivalGameMode::GetCurrentWaveSpawnerTableRow() const
{
	const FName RowName = FName(TEXT("Wave") + FString::FromInt(CurrentWaveCount));

	const FWEnemyWaveSpawnerTableRow* FoundRow = EnemyWaveSpawnerDataTable->FindRow<FWEnemyWaveSpawnerTableRow>(RowName, FString());

	checkf(FoundRow, TEXT("Could not find a valid row under the name %s in the data table"), *RowName.ToString());

	return FoundRow;
}

int32 AWSurvivalGameMode::TrySpawnWaveEnemies()
{
	if (TargetPointsArray.IsEmpty())
	{
		UGameplayStatics::GetAllActorsOfClass(this, ATargetPoint::StaticClass(), TargetPointsArray);
	}

	if (!ensureMsgf(!TargetPointsArray.IsEmpty(), TEXT("No valid target point found in level : %s for spawning enemies"), *GetWorld()->GetName())) return 0;

	uint32 EnemiesSpawnedThisTime = 0;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	for (const FWEnemyWaveSpawnerInfo& SpawnerInfo : GetCurrentWaveSpawnerTableRow()->EnemyWaveSpawnerDefinitions)
	{
		if (SpawnerInfo.EnemyClassToSpawn.IsNull()) continue;

		const int32 NumToSpawn = FMath::RandRange(SpawnerInfo.MinPerSpawnCount, SpawnerInfo.MaxPerSpawnCount);

		UClass* LoadedEnemyClass = PreLoadedEnemyClassMap.FindChecked(SpawnerInfo.EnemyClassToSpawn);

		for (int32 i = 0; i < NumToSpawn; i++)
		{
			const int32 RandomTargetPointIndex = FMath::RandRange(0, TargetPointsArray.Num() - 1);
			const AActor* TargetPoint		   = TargetPointsArray[RandomTargetPointIndex];
			const FVector SpawnOrigin		   = TargetPoint->GetActorLocation();
			const FRotator SpawnRotation	   = TargetPoint->GetActorForwardVector().ToOrientationRotator();

			FVector RandomLocation;
			UNavigationSystemV1::K2_GetRandomLocationInNavigableRadius(this, SpawnOrigin, RandomLocation, 400.f);

			RandomLocation += FVector(0.f, 0.f, 150.f);

			AWEnemyCharacter* SpawnedEnemy = GetWorld()->SpawnActor<AWEnemyCharacter>(LoadedEnemyClass, RandomLocation, SpawnRotation, SpawnParams);

			if (SpawnedEnemy)
			{
                SpawnedEnemy->OnDestroyed.AddUniqueDynamic(this, &ThisClass::OnEnemyDestroyed);

				EnemiesSpawnedThisTime++;
				TotalSpawnedEnemiesThisWaveCounter++;
			}

			if (!ShouldKeepSpawnEnemies())
			{
				return EnemiesSpawnedThisTime;
			}
		}
	}

	return EnemiesSpawnedThisTime;
}

bool AWSurvivalGameMode::ShouldKeepSpawnEnemies() const
{
	return TotalSpawnedEnemiesThisWaveCounter < GetCurrentWaveSpawnerTableRow()->TotalEnemyToSpawnThisWave;
}

void AWSurvivalGameMode::OnEnemyDestroyed(AActor* DestroyedActor)
{
    CurrentSpawnedEnemiesCounter--;

    if (ShouldKeepSpawnEnemies())
    {
        CurrentSpawnedEnemiesCounter += TrySpawnWaveEnemies();
    }
    else if (CurrentSpawnedEnemiesCounter == 0)
    {
        TotalSpawnedEnemiesThisWaveCounter = 0;
        CurrentSpawnedEnemiesCounter = 0;

        SetCurrentSurvivalGameModeState(EWSurvivalGameModeState::WaveCompleted);
    }
}

void AWSurvivalGameMode::RegisterSpawnedEnemies(const TArray<AWEnemyCharacter*>& InEnemiesToRegister)
{
	for (AWEnemyCharacter* SpawnedEnemy : InEnemiesToRegister)
	{
		if (SpawnedEnemy)
		{
			CurrentSpawnedEnemiesCounter++;

			SpawnedEnemy->OnDestroyed.AddUniqueDynamic(this, &ThisClass::OnEnemyDestroyed);
		}
	}
}