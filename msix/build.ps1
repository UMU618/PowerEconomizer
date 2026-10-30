# Build the MSIX package for PowerEconomizer.
#
# Usage:
#   pwsh -File make_msix.ps1                 # pack only (Store submission)
#   pwsh -File make_msix.ps1 -Thumbprint XYZ # pack and sign (sideload testing)
#
# By default no signing happens: with the Store Publisher there is no matching
# cert locally, and Partner Center re-signs the uploaded package.
# When -Thumbprint is given, the cert subject must equal Identity/@Publisher in
# AppxManifest.xml (checked below).
#
param(
    [string] $Thumbprint,
    [string] $SdkBin,
    [switch] $SkipAssets
)

$ErrorActionPreference = 'Stop'
# Keep classic $LASTEXITCODE semantics for native commands.
Set-StrictMode -Version Latest
$PSNativeCommandUseErrorActionPreference = $false
$global:LASTEXITCODE = 0

$root = $PSScriptRoot
$stage = Join-Path $root 'package'
$out = Join-Path $root 'PowerEconomizer.msix'
$manifestSrc = Join-Path $root 'AppxManifest.xml'
$payload = @(
    Join-Path $root 'PowerEconomizer.exe'
    Join-Path $root 'PowerEconomizer.toml'
    $manifestSrc
)

function Fail([string] $msg) {
    Write-Error $msg
    exit 1
}

# --- locate the Windows SDK tools -------------------------------------------
$makeAppx = Get-Command makeappx.exe -ErrorAction SilentlyContinue |
    Select-Object -First 1 -ExpandProperty Source

if (-not $makeAppx) {
    if (-not $SdkBin) {
        $winkits = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin'
        if (Test-Path $winkits) {
            $SdkBin = Get-ChildItem $winkits -Directory -Filter '10.0.*' |
                Sort-Object { [version]$_.Name } -Descending |
                ForEach-Object { Join-Path $_.FullName 'x64' } |
                Where-Object { Test-Path (Join-Path $_ 'makeappx.exe') } |
                Select-Object -First 1
        }
        if (-not $SdkBin) { Fail 'makeappx.exe not found. Install a Windows 10 SDK, or pass -SdkBin.' }
    }
    $makeAppx = Join-Path $SdkBin 'makeappx.exe'
    if (-not (Test-Path $makeAppx)) { Fail "makeappx.exe not found in '$SdkBin'." }
}
$signtool = Join-Path (Split-Path $makeAppx) 'signtool.exe'
if (-not (Test-Path $signtool)) { Fail "signtool.exe not found next to '$makeAppx'." }
Write-Host "SDK: $(Split-Path $makeAppx)"

foreach ($f in $payload) {
    if (-not (Test-Path $f)) { Fail "Missing payload file: $f" }
}

# --- 1. logo assets ---------------------------------------------------------
if (-not $SkipAssets) {
    & "$root\gen_assets.ps1"
    if ($LASTEXITCODE) { Fail 'Asset generation failed.' }
}

# --- 2. stage the payload ---------------------------------------------------
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Path $stage | Out-Null
Copy-Item $payload -Destination $stage
Copy-Item (Join-Path $root 'Assets') -Destination $stage -Recurse
Write-Host "Staged $(@(Get-ChildItem $stage -Recurse -File).Count) file(s) in '$stage'."

# --- 3. pack ----------------------------------------------------------------
if (Test-Path $out) { Remove-Item $out -Force }
& $makeAppx pack /d $stage /p $out /o
if ($LASTEXITCODE) { Fail 'makeappx pack failed.' }

# --- 4. sign ----------------------------------------------------------------
if (-not $Thumbprint) {
    Write-Host ""
    Write-Host "Done (unsigned): $out" -ForegroundColor Green
    Write-Host "Submit this package to Partner Center; it will be signed there."
    exit 0
}

$cert = @('Cert:\CurrentUser\My', 'Cert:\LocalMachine\My') |
    ForEach-Object { Get-ChildItem $_ -ErrorAction SilentlyContinue } |
    Where-Object { $_.Thumbprint -ieq $Thumbprint } |
    Select-Object -First 1
if (-not $cert) { Fail "No private-key certificate with thumbprint '$Thumbprint' in CurrentUser\\My or LocalMachine\\My." }

[xml] $manifest = Get-Content $manifestSrc -Raw
$publisher = $manifest.Package.Identity.Publisher
if ($cert.Subject -ne $publisher) {
    Fail "Publisher mismatch: certificate is '$($cert.Subject)', AppxManifest.xml declares '$publisher'. They must be identical."
}

# MSIX signing only supports SHA256 digest; SHA512 fails with SignerSign() 0x8007000B.
& $signtool sign /sha1 $Thumbprint /fd SHA256 /t http://timestamp.digicert.com/ $out
if ($LASTEXITCODE) { Fail 'signtool sign failed.' }

Write-Host ""
Write-Host "Done (signed): $out" -ForegroundColor Green
Write-Host "To install, trust the cert first:"
Write-Host "  Import-Certificate -FilePath <cert>.cer -CertStoreLocation Cert:\LocalMachine\TrustedPeople"
Write-Host "  Add-AppxPackage `"$out`""
