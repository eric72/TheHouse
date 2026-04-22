# Lance une etape du pipeline de localisation Unreal (cible Game) via UnrealEditor-Cmd + -LiveCoding=false.
# Meme mecanisme que le Localization Dashboard, sans blocage Live Coding quand l'editeur est ouvert.
#
# Usage :
#   powershell -NoProfile -ExecutionPolicy Bypass -File "...\RunLocalizationStep.ps1" -Step Gather
# Etapes : Gather | Compile | Export | Import | ImportDialogue | ExportDialogueScript | ImportDialogueScript | Reports | RepairGatherIni
# Option :
#   -EngineRoot "C:\Program Files\Epic Games\UE_5.7"

[CmdletBinding()]
param(
	[Parameter(Mandatory = $true, HelpMessage = "Etape du pipeline (voir commentaires en tete de script).")]
	[ValidateSet('Gather', 'Compile', 'Export', 'Import', 'ImportDialogue', 'ExportDialogueScript', 'ImportDialogueScript', 'Reports', 'RepairGatherIni')]
	[string] $Step,
	[string] $EngineRoot = "C:\Program Files\Epic Games\UE_5.7"
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
$UProject = Join-Path $ProjectRoot "TheHouse.uproject"
$EditorCmd = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"

$IniByStep = @{
	Gather               = 'Config/Localization/Game_Gather.ini'
	Compile              = 'Config/Localization/Game_Compile.ini'
	Export               = 'Config/Localization/Game_Export.ini'
	Import               = 'Config/Localization/Game_Import.ini'
	ImportDialogue       = 'Config/Localization/Game_ImportDialogue.ini'
	ExportDialogueScript = 'Config/Localization/Game_ExportDialogueScript.ini'
	ImportDialogueScript = 'Config/Localization/Game_ImportDialogueScript.ini'
	Reports              = 'Config/Localization/Game_GenerateReports.ini'
}

function Repair-GameGatherIni {
	param([string]$IniPath)
	$text = [System.IO.File]::ReadAllText($IniPath)
	$before = $text
	$text = [regex]::Replace($text,
		'(?m)^(\[GatherTextStep0\]\r?\nCommandletClass=GatherTextFromSource\r?\n)(?!SearchDirectoryPaths=)',
		'${1}SearchDirectoryPaths=%LOCPROJECTROOT%Source`r`nSearchDirectoryPaths=%LOCPROJECTROOT%Plugins`r`n')
	$text = [regex]::Replace($text,
		'(?m)^(\[GatherTextStep1\]\r?\nCommandletClass=GatherTextFromAssets\r?\n)(?!IncludePathFilters=)',
		'${1}IncludePathFilters=Content/*`r`n')
	if ($text -ne $before) {
		[System.IO.File]::WriteAllText($IniPath, $text, [System.Text.UTF8Encoding]::new($false))
		Write-Host "Game_Gather.ini : cles obligatoires reinjectees (le dashboard les avait supprimees)." -ForegroundColor Yellow
	}
}

if ($Step -eq 'RepairGatherIni') {
	$gatherFull = Join-Path $ProjectRoot "Config/Localization/Game_Gather.ini"
	if (-not (Test-Path $gatherFull)) {
		Write-Error "Fichier manquant : Config/Localization/Game_Gather.ini - cree la cible Game dans le Localization Dashboard."
	}
	Repair-GameGatherIni -IniPath $gatherFull
	Write-Host "RepairGatherIni : OK." -ForegroundColor Green
	exit 0
}

$relIni = $IniByStep[$Step]
$iniFull = Join-Path $ProjectRoot $relIni
if (-not (Test-Path $iniFull)) {
	Write-Error "Fichier manquant : $relIni - genere la cible Game dans le Localization Dashboard."
}
if (-not (Test-Path $EditorCmd)) {
	Write-Error "Moteur introuvable : $EditorCmd (passe -EngineRoot ...)"
}

if ($Step -eq 'Gather') {
	Repair-GameGatherIni -IniPath (Join-Path $ProjectRoot "Config/Localization/Game_Gather.ini")
}

Set-Location $ProjectRoot
Write-Host "Localization step [$Step] -> $relIni" -ForegroundColor Cyan
& $EditorCmd $UProject -run=GatherText "-config=$relIni" -unattended -nop4 -nosplash -NullRHI -LiveCoding=false -log
$code = $LASTEXITCODE
if ($code -ne 0) {
	exit $code
}
Write-Host "OK : step [$Step] termine." -ForegroundColor Green
