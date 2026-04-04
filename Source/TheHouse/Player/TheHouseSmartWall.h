
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/Texture.h"
#include "TheHouseSmartWall.generated.h"

/**
 * A wall that automatically registers itself with the RTS Camera System.
 * It has the "SeeThroughWall" tag by default.
 */
UCLASS()
class THEHOUSE_API ATheHouseSmartWall : public AActor
{
	GENERATED_BODY()
	
public:	
	ATheHouseSmartWall();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

public:	
	/** Mesh du mur, toujours visible dans la section SmartWall|Mesh du BP. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SmartWall|Mesh")
	UStaticMeshComponent* WallMesh;

	/** Surface "cap" qui remplit la coupe quand le mur est réduit. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SmartWall|Occlusion")
	UStaticMeshComponent* WallFillCapMesh;

	/**
	 * Si le pivot du mesh est au centre (moitié du mur sous le sol), décale le WallMesh pour que la BASE
	 * soit à l’origine de l’acteur (pose au sol). Désactive si ton mesh a déjà le pivot en bas.
	 */
	UPROPERTY(EditAnywhere, Category = "SmartWall|Mesh")
	bool bAlignMeshBottomToActorOrigin = true;

	/** Décalage Z additionnel (cm) après alignement base, pour ajuster au sol / plinthe */
	UPROPERTY(EditAnywhere, Category = "SmartWall|Mesh")
	float MeshVerticalOffset = 0.f;

	/** Occlusion 0 = mur plein, 1 = réduit (style Sims) + option matériau CameraFade */
	UFUNCTION(BlueprintCallable, Category = "SmartWall|Occlusion")
	void SetWallOpacity(float Opacity);

	/** Nom du paramètre de fondu dans le matériau (CameraFade par défaut). */
	UPROPERTY(EditDefaultsOnly, Category = "SmartWall|Occlusion")
	FName MaterialFadeParameterName = "CameraFade";

	/**
	 * Demi-hauteur du mesh en cm (Extent.Z * |Scale.Z|), poussée sur le MID pour le graphe M_SmartWall
	 * (HeightFromBottom = Z_rel + WallHalfHeight pour que la coupe parte du pied, pas du centre).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "SmartWall|Occlusion")
	FName WallHalfHeightParamName = "WallHalfHeight";

	/**
	 * SECTION SmartWall|Occlusion
	 * Tout ce qui contrôle la "réduction" de mur façon Sims reste regroupé ici pour être facilement retrouvable en Blueprint.
	 */

	/** Si false (recommandé avec M_SmartWall : découpe Z + dither dans le matériau), seul CameraFade est poussé sur le MID. */
	UPROPERTY(EditAnywhere, Category = "SmartWall|Occlusion")
	bool bShrinkMeshHeightForOcclusion = false;

	/**
	 * Hauteur minimale réelle du mur à conserver lorsqu'il est "coupé" (en centimètres, monde).
	 * Exemple : 10.0 = on laisse un plinthe de 10 cm au maximum de l'occlusion.
	 * Si > 0, ce paramètre est utilisé pour calculer dynamiquement l'échelle Z minimale.
	 */
	UPROPERTY(EditAnywhere, Category = "SmartWall|Occlusion", meta = (ClampMin = "0.0"))
	float MinVisibleHeightCm = 10.f;

	/**
	 * Facteur d'échelle minimal sur Z au maximum de l'occlusion.
	 * Si MinVisibleHeightCm > 0, ce paramètre est calculé automatiquement à partir de la hauteur du mesh.
	 * Laisser à la valeur par défaut et utiliser plutôt MinVisibleHeightCm pour un comportement indépendant du mesh.
	 */
	UPROPERTY(EditAnywhere, Category = "SmartWall|Occlusion", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float MinWallHeightScaleWhenOccluded = 0.18f;

	/** Pivot au centre du mesh : compense en Z pour garder la base au sol */
	UPROPERTY(EditAnywhere, Category = "SmartWall|Occlusion")
	bool bCompensateCenterPivot = false;

	UPROPERTY(EditAnywhere, Category = "SmartWall|Occlusion")
	bool bDriveMaterialParameter = true;

private:
	void ApplyMeshBottomAlignment();
	void EnsureInitialMeshTransformCached();
	void PushWallMeshMaterialParams();
	void UpdateWallFillCap();
	bool ShouldEnableWallFillCap() const;
	void EnsureWallFillCapMaterialInstances();
	void PushWallFillCapMaterialParams();
	void UpdateWallFillCapTransform();

	UPROPERTY()
	bool bMeshTransformCached = false;

	UPROPERTY()
	FVector InitialWallMeshScale = FVector::OneVector;

	UPROPERTY()
	FVector InitialWallMeshLocation = FVector::ZeroVector;

	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> CachedDMIs;

	/** DMI séparés pour le cap afin de ne pas répliquer tout le comportement du mur. */
	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> CachedFillCapDMIs;

	/** Texture noire de fallback (quand aucun FillCapTexture n'est fournie). */
	UPROPERTY()
	UTexture* EngineBlackTexture = nullptr;

	/** Demi-épaisseur du cap, en cm (cache pour éviter des calculs partout). */
	UPROPERTY()
	float CachedFillCapHalfThicknessCm = 1.f;

	/** Active le remplissage (cap) pour simuler un mur "plein". Actif aussi si FillCapTexture est non null. */
	UPROPERTY(EditAnywhere, Category = "SmartWall|Occlusion", meta = (DisplayName = "Fill Cap - Enable"))
	bool bUseFillCap = false;

	/** Texture utilisée pour remplir la face de coupe. Si null et bUseFillCap=true, on utilise une texture noire. */
	UPROPERTY(EditAnywhere, Category = "SmartWall|Occlusion", meta = (DisplayName = "Fill Cap - Texture"))
	UTexture* FillCapTexture = nullptr;

	/** Épaisseur du cap, en cm. */
	UPROPERTY(EditAnywhere, Category = "SmartWall|Occlusion", meta = (ClampMin = "0.0", DisplayName = "Fill Cap - Thickness (cm)"))
	float FillCapThicknessCm = 2.0f;

	/** Recouvrement / biais vers le bas pour éviter le Z-fighting sur la coupe. */
	UPROPERTY(EditAnywhere, Category = "SmartWall|Occlusion", meta = (ClampMin = "0.0", DisplayName = "Fill Cap - Z Overlap (cm)"))
	float FillCapZOverlapCm = 0.2f;

	/** Utilise la texture comme "InteriorTexture" (par défaut) ou comme "MainTexture". */
	UPROPERTY(EditAnywhere, Category = "SmartWall|Occlusion", meta = (DisplayName = "Fill Cap - Use Interior Texture"))
	bool bFillCapUseInteriorTexture = true;

	/** Paramètre de texture pour l'extérieur dans M_SmartWall. */
	UPROPERTY(EditDefaultsOnly, Category = "SmartWall|Occlusion", meta = (DisplayName = "Fill Cap - Main Texture Param"))
	FName FillCapMainTextureParameterName = "MainTexture";

	/** Paramètre de texture pour l'intérieur dans M_SmartWall. */
	UPROPERTY(EditDefaultsOnly, Category = "SmartWall|Occlusion", meta = (DisplayName = "Fill Cap - Interior Texture Param"))
	FName FillCapInteriorTextureParameterName = "InteriorTexture";
};
