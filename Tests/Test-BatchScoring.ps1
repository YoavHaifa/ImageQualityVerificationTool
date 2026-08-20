<#
.SYNOPSIS
    Simple regression test for IQV batch scoring.

.DESCRIPTION
    Runs IQV_tool.exe in batch mode against a DICOM root directory, then either:
      - saves the resulting per-case output as the "known good" baseline for the
        current gConfig version (-SaveBaseline), or
      - compares the freshly produced output against that saved baseline (default).

    Baselines are kept out of the source repo entirely: they live on disk next to
    the tool's own results, named "<LogRoot>_<Version>" (e.g. "d:\IQV_Log_1.0"),
    where LogRoot/Version are read from the app's own ReconTest.State.xml so this
    script never needs its own copy of those settings.

.PARAMETER RootDir
    Root directory containing the DICOM case(s) to score.

.PARAMETER ExePath
    Path to IQV_tool.exe. Defaults to the x64 Debug build output.

.PARAMETER SaveBaseline
    Save the current run's results as the baseline instead of comparing to it.
    Overwrites any existing baseline for the current version.

.EXAMPLE
    .\Test-BatchScoring.ps1 -RootDir "D:\IQ_Data\gilad-to-yoav" -SaveBaseline
    .\Test-BatchScoring.ps1 -RootDir "D:\IQ_Data\gilad-to-yoav"
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$RootDir,

    [string]$ExePath = "D:\SW_IR\ImageQualityVerificationTool\x64\Debug\IQV_tool.exe",

    [switch]$SaveBaseline
)

function Read-AppSetting {
    param([string]$ExePath, [string]$Name, [string]$Default)

    $xmlPath = Join-Path (Split-Path $ExePath -Parent) "ReconTest.State.xml"
    if (Test-Path $xmlPath) {
        [xml]$xml = Get-Content $xmlPath
        $node = $xml.def.$Name
        if ($node) {
            return $node.ToString().Trim()
        }
    }
    return $Default
}

if (-not (Test-Path $ExePath)) {
    Write-Error "IQV_tool.exe not found at: $ExePath"
    exit 1
}
if (-not (Test-Path $RootDir)) {
    Write-Error "Root directory not found: $RootDir"
    exit 1
}

$logRoot = Read-AppSetting -ExePath $ExePath -Name "log_root" -Default "d:\IQV_Log"
$version = Read-AppSetting -ExePath $ExePath -Name "version" -Default "1.0"
$baselineDir = "${logRoot}_${version}"

Write-Output "Log root : $logRoot"
Write-Output "Version  : $version"
Write-Output "Baseline : $baselineDir"

Write-Output "`nRunning batch scoring on: $RootDir"
$proc = Start-Process -FilePath $ExePath -ArgumentList "`"$RootDir`"" -PassThru -Wait
if ($proc.ExitCode -ne 0) {
    Write-Error "IQV_tool.exe exited with code $($proc.ExitCode)"
    exit 1
}

if ($SaveBaseline) {
    if (Test-Path $baselineDir) {
        Write-Output "Overwriting existing baseline at $baselineDir"
        Remove-Item -Path $baselineDir -Recurse -Force
    }
    Copy-Item -Path $logRoot -Destination $baselineDir -Recurse
    Write-Output "Baseline saved to $baselineDir"
    exit 0
}

if (-not (Test-Path $baselineDir)) {
    Write-Error "No baseline found at $baselineDir - run with -SaveBaseline first."
    exit 1
}

Write-Output "`nComparing $logRoot against baseline $baselineDir ..."

$baselineFiles = Get-ChildItem -Path $baselineDir -Recurse -File
$nChecked = 0
$nMismatched = 0
$nMissing = 0

foreach ($bf in $baselineFiles) {
    $relPath = $bf.FullName.Substring($baselineDir.Length).TrimStart('\')
    $currentFile = Join-Path $logRoot $relPath
    $nChecked++

    if (-not (Test-Path $currentFile)) {
        Write-Output "MISSING : $relPath"
        $nMissing++
        continue
    }

    $baselineContent = Get-Content $bf.FullName -Raw
    $currentContent = Get-Content $currentFile -Raw
    if ($baselineContent -ne $currentContent) {
        Write-Output "MISMATCH: $relPath"
        $nMismatched++
    }
}

$currentFiles = Get-ChildItem -Path $logRoot -Recurse -File
$nExtra = 0
foreach ($cf in $currentFiles) {
    $relPath = $cf.FullName.Substring($logRoot.Length).TrimStart('\')
    $baselineFile = Join-Path $baselineDir $relPath
    if (-not (Test-Path $baselineFile)) {
        Write-Output "EXTRA   : $relPath"
        $nExtra++
    }
}

Write-Output "`nChecked $nChecked baseline file(s): $nMismatched mismatch(es), $nMissing missing, $nExtra extra file(s) not in baseline."

if ($nMismatched -gt 0 -or $nMissing -gt 0 -or $nExtra -gt 0) {
    Write-Output "RESULT: FAIL"
    exit 1
}

Write-Output "RESULT: PASS"
exit 0
