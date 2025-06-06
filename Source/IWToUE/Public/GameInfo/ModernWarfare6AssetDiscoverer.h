﻿#pragma once

#include "Database/CoDDatabaseService.h"
#include "Interface/IGameAssetDiscoverer.h"
#include "Interface/IMemoryReader.h"
#include "WraithX/CoDAssetType.h"
#include "WraithX/GameProcess.h"

class UWraithSettings;
class UWraithSettingsManager;

class FModernWarfare6AssetDiscoverer : public IGameAssetDiscoverer
{
public:
	FModernWarfare6AssetDiscoverer();
	virtual ~FModernWarfare6AssetDiscoverer() override = default;

	virtual bool Initialize(IMemoryReader* InReader,
	                        const CoDAssets::FCoDGameProcess& InProcessInfo,
	                        TSharedPtr<LocateGameInfo::FParasyteBaseState> InParasyteState) override;

	virtual CoDAssets::ESupportedGames GetGameType() const override { return GameType; }
	virtual CoDAssets::ESupportedGameFlags GetGameFlags() const override { return GameFlag; }

	virtual TSharedPtr<FXSub> GetDecryptor() const override { return XSubDecrypt; }
	virtual FCoDCDNDownloader* GetCDNDownloader() const override { return CDNDownloader.Get(); }

	virtual TArray<FAssetPoolDefinition> GetAssetPools() const override;

	virtual int32 DiscoverAssetsInPool(const FAssetPoolDefinition& PoolDefinition,
	                                   FAssetDiscoveredDelegate OnAssetDiscovered) override;

	virtual bool LoadStringTableEntry(uint64 Index, FString& OutString) override;

private:
	// --- Asset Pool Processing Logic ---

	// Reads the FXAssetPool64 structure
	bool ReadAssetPoolHeader(int32 PoolIdentifier, FXAssetPool64& OutPoolHeader);
	// Reads the FXAsset64 structure
	bool ReadAssetNode(uint64 AssetNodePtr, FXAsset64& OutAssetNode);
	// Specific asset processing functions called by DiscoverAssetsInPool
	void DiscoverModelAssets(FXAsset64 AssetNode, FAssetDiscoveredDelegate OnAssetDiscovered);
	void DiscoverImageAssets(FXAsset64 AssetNode, FAssetDiscoveredDelegate OnAssetDiscovered);
	void DiscoverAnimAssets(FXAsset64 AssetNode, FAssetDiscoveredDelegate OnAssetDiscovered);
	void DiscoverMaterialAssets(FXAsset64 AssetNode, FAssetDiscoveredDelegate OnAssetDiscovered);
	void DiscoverSoundAssets(FXAsset64 AssetNode, FAssetDiscoveredDelegate OnAssetDiscovered);
	void DiscoverMapAssets(FXAsset64 AssetNode, FAssetDiscoveredDelegate OnAssetDiscovered);
	
	TSharedPtr<LocateGameInfo::FParasyteBaseState> ParasyteState;
	CoDAssets::ESupportedGames GameType = CoDAssets::ESupportedGames::None;
	CoDAssets::ESupportedGameFlags GameFlag = CoDAssets::ESupportedGameFlags::None;
	TSharedPtr<FXSub> XSubDecrypt;
	TUniquePtr<FCoDCDNDownloader> CDNDownloader;
};
