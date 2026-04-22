# Raccourci : Gather texte (cible Game). Voir RunLocalizationStep.ps1 pour toutes les etapes.
#
# Usage :
#   powershell -NoProfile -ExecutionPolicy Bypass -File "...\GatherLocalization.ps1"
#   ...\GatherLocalization.ps1 -RepairOnly
#   ...\GatherLocalization.ps1 -EngineRoot "D:\UE\UE_5.7"

[CmdletBinding()]
param(
	[string] $EngineRoot = "C:\Program Files\Epic Games\UE_5.7",
	[switch] $RepairOnly
)

$ErrorActionPreference = "Stop"
$Runner = Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "RunLocalizationStep.ps1"
if ($RepairOnly) {
	& $Runner -Step RepairGatherIni -EngineRoot $EngineRoot
	exit $LASTEXITCODE
}
& $Runner -Step Gather -EngineRoot $EngineRoot
exit $LASTEXITCODE
