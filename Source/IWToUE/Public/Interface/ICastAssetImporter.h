﻿#pragma once

#include "SeLogChannels.h"
#include "ObjectTools.h"
#include "AssetRegistry/AssetRegistryModule.h"

struct FCastScene;
struct FCastImportOptions;
struct FCastAnimationInfo;
struct FCastRoot;
struct FCastModelInfo;
struct FCastMaterialInfo;
class FCastTextureInfo;

DECLARE_DELEGATE_TwoParams(FOnCastImportProgress, float /*ProgressFraction*/, const FText& /*StatusText*/);

class ICastAssetImporter
{
public:
	virtual ~ICastAssetImporter() = default;

	virtual void SetProgressDelegate(const FOnCastImportProgress& InProgressDelegate)
	{
		ProgressDelegate = InProgressDelegate;
	}

	static FString NoIllegalSigns(const FString& InString);

protected:
	void ReportProgress(float Percent, const FText& Message);


	template <typename AssetType>
	static AssetType* CreateAsset(const FString& PackagePath, const FString& AssetName, bool bAllowReplace = false);

	FOnCastImportProgress ProgressDelegate;
};

class ICastTextureImporter : public ICastAssetImporter
{
public:
	virtual UTexture2D* ImportOrLoadTexture(FCastTextureInfo& TextureInfo, const FString& AbsoluteTextureFilePath,
	                                        const FString& AssetDestinationPath) = 0;
};

class IMaterialCreationStrategy
{
public:
	virtual ~IMaterialCreationStrategy() = default;
	virtual FString GetParentMaterialPath(const FCastMaterialInfo& MaterialInfo, bool& bOutIsMetallic) const = 0;
	virtual void ApplyAdditionalParameters(UMaterialInstanceConstant* MaterialInstance,
	                                       const FCastMaterialInfo& MaterialInfo, bool bIsMetallic) const = 0;
};

class ICastMaterialImporter : public ICastAssetImporter
{
public:
	virtual UMaterialInterface* CreateMaterialInstance(
		const FCastMaterialInfo& MaterialInfo,
		const FCastImportOptions& Options,
		UObject* ParentPackage) = 0;
	virtual void SetTextureParameter(UMaterialInstanceConstant* Instance, const FCastTextureInfo& TextureInfo) = 0;
};

class ICastMeshImporter : public ICastAssetImporter
{
public:
	virtual UStaticMesh* ImportStaticMesh(FCastScene& CastScene, const FCastImportOptions& Options,
	                                      UObject* InParent, FName Name, EObjectFlags Flags,
	                                      ICastMaterialImporter* MaterialImporter,
	                                      FString OriginalFilePath, TArray<UObject*>& OutCreatedObjects) = 0;
	virtual USkeletalMesh* ImportSkeletalMesh(FCastScene& CastScene, const FCastImportOptions& Options,
	                                          UObject* InParent, FName Name, EObjectFlags Flags,
	                                          ICastMaterialImporter* MaterialImporter, FString OriginalFilePath,
	                                          TArray<UObject*>& OutCreatedObjects) = 0;
};

class ICastAnimationImporter : public ICastAssetImporter
{
public:
	virtual UAnimSequence* ImportAnimation(const FCastAnimationInfo& AnimInfo, const FCastImportOptions& Options,
	                                       USkeleton* Skeleton, UObject* InParent, FName Name, EObjectFlags Flags) = 0;
};

template <typename AssetType>
AssetType* ICastAssetImporter::CreateAsset(const FString& PackagePath, const FString& AssetName, bool bAllowReplace)
{
	auto CoreAssetCreationLogic = [](const FString& InPackagePath, const FString& InAssetName,
	                                 bool bInAllowReplace) -> AssetType*
	{
		FString SanitizedAssetName = ObjectTools::SanitizeObjectName(InAssetName);
		FString FullPackagePath = FPaths::Combine(InPackagePath, SanitizedAssetName);
		UPackage* Package = ::CreatePackage(*FullPackagePath);

		if (!Package)
		{
			UE_LOG(LogCast, Error, TEXT("CoreLogic: Failed to create package: %s"), *FullPackagePath);
			return nullptr;
		}
		Package->FullyLoad();

		AssetType* ExistingAsset = FindObject<AssetType>(Package, *SanitizedAssetName);
		if (ExistingAsset && !bInAllowReplace)
		{
			UE_LOG(LogCast, Warning,
			       TEXT("CoreLogic: Asset '%s' already exists in package '%s' and replacement not allowed."),
			       *SanitizedAssetName, *FullPackagePath);
			return ExistingAsset;
		}
		if (ExistingAsset)
		{
			UE_LOG(LogCast, Log, TEXT("CoreLogic: Replacing existing asset: %s"), *FullPackagePath);
		}

		AssetType* ResultAsset = NewObject<AssetType>(Package, FName(*SanitizedAssetName), RF_Public | RF_Standalone);
		if (ResultAsset)
		{
			ResultAsset->MarkPackageDirty();
			FAssetRegistryModule::AssetCreated(ResultAsset);
			UE_LOG(LogCast, Log, TEXT("CoreLogic: Successfully created/replaced asset: %s"), *FullPackagePath);
		}
		else
		{
			UE_LOG(LogCast, Error, TEXT("CoreLogic: Failed to create asset object: %s in package %s"),
			       *SanitizedAssetName, *Package->GetName());
		}

		return ResultAsset;
	};

	if (IsInGameThread())
	{
		return CoreAssetCreationLogic(PackagePath, AssetName, bAllowReplace);
	}
	TPromise<AssetType*> Promise;
	TFuture<AssetType*> Future = Promise.GetFuture();

	AsyncTask(ENamedThreads::GameThread,
	          [InPackagePath = PackagePath,
		          InAssetName = AssetName,
		          bInAllowReplace = bAllowReplace,
		          CoreLogic = CoreAssetCreationLogic,
		          Promise = MoveTemp(Promise)]() mutable
	          {
		          AssetType* Result = CoreLogic(InPackagePath, InAssetName, bInAllowReplace);
		          Promise.SetValue(Result);
	          });

	AssetType* Result = Future.Get();
	return Result;
}
