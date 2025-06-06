﻿#pragma once

#include "UObject/Object.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include <tlhelp32.h>
#include <Psapi.h>
#include <string>
#include "Windows/HideWindowsPlatformTypes.h"

#include "SQLiteDatabase.h"
#include "Interface/IGameAssetDiscoverer.h"
#include "Structures/CodAssets.h"

class FXSub;
class IGameAssetDiscoverer;
class IMemoryReader;
class FCoDCDNDownloader;

struct FCoDAsset;

namespace LocateGameInfo
{
	struct FParasyteBaseState;
}

// Parasytes XAsset Structure.
struct FXAsset64
{
	// The asset header.
	uint64_t Header;
	// Whether or not this asset is a temp slot.
	uint64_t Temp;
	// The next xasset in the list.
	uint64_t Next;
	// The previous xasset in the list.
	uint64_t Previous;
};

// Parasytes XAsset Pool Structure.
struct FXAssetPool64
{
	// The start of the asset chain.
	uint64 Root;
	// The end of the asset chain.
	uint64 End;
	// The asset hash table for this pool.
	uint64 LookupTable;
	// Storage for asset headers for this pool.
	uint64 HeaderMemory;
	// Storage for asset entries for this pool.
	uint64 AssetMemory;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAssetLoadingProgressDelegate, float);
DECLARE_MULTICAST_DELEGATE(FOnAssetLoadingCompleteDelegate);

class FGameProcess : public TSharedFromThis<FGameProcess>
{
public:
	FGameProcess();
	~FGameProcess();

	// --- Initialization and Status ---
	// Finds process, creates reader/discoverer
	bool Initialize();
	bool IsInitialized() const { return bIsInitialized; }
	bool IsGameRunning();

	TSharedPtr<IMemoryReader>& GetMemoryReader() { return MemoryReader; }

	// --- Asset Loading ---
	void StartAssetDiscovery();
	bool IsDiscoveringAssets() const { return bIsDiscovering; }
	void SetTotalAssetCnt(int32 InCnt) { TotalDiscoveredAssets = InCnt; }

	FORCEINLINE TArray<TSharedPtr<FCoDAsset>>& GetLoadedAssets() { return LoadedAssets; }

	FORCEINLINE CoDAssets::ESupportedGames GetCurrentGameType() const
	{
		return AssetDiscoverer.IsValid() ? AssetDiscoverer->GetGameType() : CoDAssets::ESupportedGames::None;
	}

	FORCEINLINE CoDAssets::ESupportedGameFlags GetCurrentGameFlag() const
	{
		return AssetDiscoverer.IsValid() ? AssetDiscoverer->GetGameFlags() : CoDAssets::ESupportedGameFlags::None;
	}

	FORCEINLINE FCoDCDNDownloader* GetCDNDownloader()
	{
		return AssetDiscoverer.IsValid() ? AssetDiscoverer->GetCDNDownloader() : nullptr;
	}

	TSharedPtr<FXSub> GetDecrypt();

	// --- Delegates ---
	// Renamed delegate for clarity
	FOnAssetLoadingProgressDelegate OnAssetLoadingProgress;
	FOnAssetLoadingCompleteDelegate OnAssetLoadingComplete;

private:
	// --- Helper Methods ---
	bool FindTargetProcess();
	bool OpenProcessHandleAndReader();
	bool LocateGameInfoViaParasyte();
	bool CreateAndInitializeDiscoverer();

	// --- Callbacks ---
	void HandleAssetDiscovered(TSharedPtr<FCoDAsset> DiscoveredAsset);
	void HandleDiscoveryComplete();

	HANDLE ProcessHandle{nullptr};
	FString ProcessPath;
	DWORD ProcessId{0};

	CoDAssets::FCoDGameProcess ProcessInfo{};

	TSharedPtr<LocateGameInfo::FParasyteBaseState> ParasyteBaseState;
	FSQLiteDatabase AssetNameDB;

	TArray<TSharedPtr<FCoDAsset>> LoadedAssets;
	FCriticalSection LoadedAssetsLock;
	FCriticalSection ProgressLock;

	float CurrentLoadingProgress = 0.f;

	TSharedPtr<FXSub> XSubDecrypt;
	TUniquePtr<FCoDCDNDownloader> CDNDownloader;

	CoDAssets::ESupportedGames GameType = CoDAssets::ESupportedGames::None;
	CoDAssets::ESupportedGameFlags GameFlag = CoDAssets::ESupportedGameFlags::None;

	// --- Internal State ---
	DWORD TargetProcessId = 0;
	CoDAssets::FCoDGameProcess TargetProcessInfo{};
	FString TargetProcessPath;

public:
	TSharedPtr<LocateGameInfo::FParasyteBaseState> ParasyteState;

private:
	TSharedPtr<IGameAssetDiscoverer> AssetDiscoverer;

	TSharedPtr<IMemoryReader> MemoryReader;

	float TotalDiscoveredAssets = 0.0f;
	float CurrentDiscoveryProgressCount = 0.0f;

	// --- Async Task Management ---
	FAsyncTask<class FAssetDiscoveryTask>* DiscoveryTask = nullptr;

	std::atomic<bool> bIsInitialized = false;
	std::atomic<bool> bIsDiscovering = false;

	friend class FAssetDiscoveryTask;
};
