# Raccourci : Compile texte (cible Game). Voir RunLocalizationStep.ps1 pour toutes les etapes.
#
# Usage :
#   powershell -NoProfile -ExecutionPolicy Bypass -File "...\CompileLocalization.ps1"
#   ...\CompileLocalization.ps1 -EngineRoot "D:\UE\UE_5.7"

[CmdletBinding()]
param(
	[string] $EngineRoot = "C:\Program Files\Epic Games\UE_5.7"
)

$ErrorActionPreference = "Stop"
$Runner = Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "RunLocalizationStep.ps1"
& $Runner -Step Compile -EngineRoot $EngineRoot
exit $LASTEXITCODE
