# Regenerates VS Code / Cursor C++ IntelliSense files (compileCommands_*.json, c_cpp_properties.json)
# via UnrealBuildTool. Safe to run on any machine after clone — uses EngineAssociation from TheHouse.uproject.
# À relancer après ajout de fichiers/dossiers sous Source/TheHouse (sinon squiggles / includes introuvables dans l’IDE).
#
# Optional environment variables (first match wins):
#   UE_ENGINE_ROOT, UNREAL_ENGINE_ROOT — folder that contains Engine\ (e.g. C:\Program Files\Epic Games\UE_5.7)

$ErrorActionPreference = 'Stop'

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$UProject = Join-Path $ProjectRoot 'TheHouse.uproject'
if (-not (Test-Path $UProject)) {
    Write-Error "TheHouse.uproject not found next to Scripts: $UProject"
}

$association = (Get-Content -Raw $UProject | ConvertFrom-Json).EngineAssociation
if (-not $association) {
    Write-Error "EngineAssociation missing in TheHouse.uproject"
}

$folderName = "UE_$association"
$candidates = @(
    $env:UE_ENGINE_ROOT
    $env:UNREAL_ENGINE_ROOT
    (Join-Path ${env:ProgramFiles} "Epic Games\$folderName")
    if (${env:ProgramFiles(x86)}) { Join-Path ${env:ProgramFiles(x86)} "Epic Games\$folderName" }
) | Where-Object { $_ -and $_.Trim() -ne '' }

$engineRoot = $null
foreach ($c in $candidates) {
    $ubt = Join-Path $c 'Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll'
    if (Test-Path -LiteralPath $ubt) {
        $engineRoot = $c
        break
    }
}

if (-not $engineRoot) {
    Write-Host @"

Could not find Unreal Engine $association (folder like Epic Games\$folderName).

Install UE $association from Epic Launcher, or set one of:
  UE_ENGINE_ROOT
  UNREAL_ENGINE_ROOT

to the directory that contains Engine\ (not Engine itself).

"@ -ForegroundColor Yellow
    exit 1
}

$ubtDll = Join-Path $engineRoot 'Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll'
Write-Host "Using engine: $engineRoot" -ForegroundColor Cyan

Push-Location $ProjectRoot
try {
    & dotnet $ubtDll -projectfiles -vscode "-project=$UProject" -game -engine
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
finally {
    Pop-Location
}

Write-Host @"

Done. In Cursor / VS Code:
  1. Ctrl+Shift+P -> C/C++: Reset IntelliSense Database
  2. Ctrl+Shift+P -> Developer: Reload Window

"@ -ForegroundColor Green
