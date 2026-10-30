# Generate MSIX logo assets (PNG) from res/PE.ico
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File gen_assets.ps1
#
param(
    [string] $IcoPath = (Join-Path $PSScriptRoot '..\src\PowerEconomizer\res\PE.ico'),
    [string] $OutDir  = (Join-Path $PSScriptRoot 'Assets')
)

$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName PresentationCore
Add-Type -AssemblyName WindowsBase

$ico = (Resolve-Path $IcoPath).Path
$decoder = [System.Windows.Media.Imaging.BitmapDecoder]::Create(
    [System.Uri]::new($ico),
    [System.Windows.Media.Imaging.BitmapCreateOptions]::PreservePixelFormat,
    [System.Windows.Media.Imaging.BitmapCacheOption]::OnLoad)
$frame = $decoder.Frames[0]

if (-not (Test-Path $OutDir)) {
    New-Item -ItemType Directory -Path $OutDir | Out-Null
}

function Write-Png {
    param(
        [System.Windows.Media.Imaging.BitmapSource] $Source,
        [int] $Width,
        [int] $Height,
        [double] $LogoRatio,   # logo size / min(width, height)
        [string] $Path
    )
    if (Test-Path $Path) {
        Write-Host $Path
        return
    }

    $side = [Math]::Min($Width, $Height) * $LogoRatio
    $visual = [System.Windows.Media.DrawingVisual]::new()
    $dc = $visual.RenderOpen()
    $dc.DrawImage($Source, [System.Windows.Rect]::new(
        ($Width - $side) / 2, ($Height - $side) / 2, $side, $side))
    $dc.Close()

    $rtb = [System.Windows.Media.Imaging.RenderTargetBitmap]::new(
        $Width, $Height, 96, 96, [System.Windows.Media.PixelFormats]::Pbgra32)
    $rtb.Render($visual)

    $encoder = [System.Windows.Media.Imaging.PngBitmapEncoder]::new()
    $encoder.Frames.Add([System.Windows.Media.Imaging.BitmapFrame]::Create($rtb))
    $fs = [System.IO.File]::Open($Path, [System.IO.FileMode]::Create)
    try { $encoder.Save($fs) } finally { $fs.Dispose() }

    Write-Host "$((Get-Item $Path).Name) ${Width}x${Height}"
}

# name, width, height, logo ratio
$assets = @(
    @('Square44x44Logo.png',   44,  44,  1.0),
    @('Square71x71Logo.png',   71,  71,  1.0),
    @('Square150x150Logo.png', 150, 150, 1.0),
    @('Square310x310Logo.png', 310, 310, 1.0),
    @('StoreLogo.png',         50,  50,  1.0),
    @('Wide310x150Logo.png',   310, 150, 0.8)
)

foreach ($a in $assets) {
    Write-Png -Source $frame -Width $a[1] -Height $a[2] -LogoRatio $a[3] `
        -Path (Join-Path $OutDir $a[0])
}

Write-Host "Assets written to $OutDir"
