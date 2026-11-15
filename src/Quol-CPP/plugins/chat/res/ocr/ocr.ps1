param(
    [Parameter(Mandatory = $true)][string]$ImagePath,
    [string]$Language = ''
)

$ErrorActionPreference = 'Stop'

try {
    Add-Type -AssemblyName System.Runtime.WindowsRuntime

    $asTaskGeneric = ([System.WindowsRuntimeSystemExtensions].GetMethods() |
        Where-Object {
            $_.Name -eq 'AsTask' -and
            $_.GetParameters().Count -eq 1 -and
            $_.GetParameters()[0].ParameterType.Name -eq 'IAsyncOperation`1'
        })[0]

    function Await($WinRtTask, $ResultType) {
        $asTask = $asTaskGeneric.MakeGenericMethod($ResultType)
        $netTask = $asTask.Invoke($null, @($WinRtTask))
        $netTask.Wait(-1) | Out-Null
        $netTask.Result
    }

    [Windows.Media.Ocr.OcrEngine, Windows.Foundation, ContentType = WindowsRuntime] | Out-Null
    [Windows.Globalization.Language, Windows.Globalization, ContentType = WindowsRuntime] | Out-Null
    [Windows.Graphics.Imaging.BitmapDecoder, Windows.Graphics, ContentType = WindowsRuntime] | Out-Null
    [Windows.Graphics.Imaging.BitmapTransform, Windows.Graphics, ContentType = WindowsRuntime] | Out-Null
    [Windows.Graphics.Imaging.BitmapPixelFormat, Windows.Graphics, ContentType = WindowsRuntime] | Out-Null
    [Windows.Graphics.Imaging.BitmapAlphaMode, Windows.Graphics, ContentType = WindowsRuntime] | Out-Null
    [Windows.Graphics.Imaging.ExifOrientationMode, Windows.Graphics, ContentType = WindowsRuntime] | Out-Null
    [Windows.Graphics.Imaging.ColorManagementMode, Windows.Graphics, ContentType = WindowsRuntime] | Out-Null

    if (-not (Test-Path -LiteralPath $ImagePath)) {
        [Console]::Error.WriteLine('OCR_ERROR: image not found')
        exit 1
    }

    $stream = [System.IO.WindowsRuntimeStreamExtensions]::AsRandomAccessStream([System.IO.File]::OpenRead($ImagePath))
    $decoder = Await ([Windows.Graphics.Imaging.BitmapDecoder]::CreateAsync($stream)) ([Windows.Graphics.Imaging.BitmapDecoder])

    # the legacy OCR engine drops small text; upscale narrow crops so it has enough pixels to work with
    $scale = 1.0
    if ($decoder.PixelWidth -gt 0 -and $decoder.PixelWidth -lt 1500) {
        $scale = [Math]::Min(3.0, 1500.0 / $decoder.PixelWidth)
    }
    $maxDim = [Windows.Media.Ocr.OcrEngine]::MaxImageDimension
    if ($maxDim -gt 0 -and $decoder.PixelWidth * $scale -gt $maxDim) {
        $scale = $maxDim / $decoder.PixelWidth
    }

    if ($scale -gt 1.05) {
        $transform = New-Object Windows.Graphics.Imaging.BitmapTransform
        $transform.ScaledWidth = [int32][Math]::Ceiling($decoder.PixelWidth * $scale)
        $transform.ScaledHeight = [int32][Math]::Ceiling($decoder.PixelHeight * $scale)
        $soft = Await ($decoder.GetSoftwareBitmapAsync(
            [Windows.Graphics.Imaging.BitmapPixelFormat]::Bgra8,
            [Windows.Graphics.Imaging.BitmapAlphaMode]::Premultiplied,
            $transform,
            [Windows.Graphics.Imaging.ExifOrientationMode]::RespectExifOrientation,
            [Windows.Graphics.Imaging.ColorManagementMode]::DoNotColorManage
        )) ([Windows.Graphics.Imaging.SoftwareBitmap])
    } else {
        $soft = Await ($decoder.GetSoftwareBitmapAsync()) ([Windows.Graphics.Imaging.SoftwareBitmap])
    }

    $engine = $null
    if ($Language) {
        try {
            $engine = [Windows.Media.Ocr.OcrEngine]::TryCreateFromLanguage((New-Object Windows.Globalization.Language($Language)))
        } catch {
            $engine = $null
        }
    }
    if (-not $engine) {
        $engine = [Windows.Media.Ocr.OcrEngine]::TryCreateFromUserProfileLanguages()
    }
    if (-not $engine) {
        $availableLangs = [Windows.Media.Ocr.OcrEngine]::AvailableRecognizerLanguages
        if ($availableLangs.Count -gt 0) {
            $engine = [Windows.Media.Ocr.OcrEngine]::TryCreateFromLanguage($availableLangs[0])
        }
    }
    if (-not $engine) {
        [Console]::Error.WriteLine('OCR_ERROR: no OCR engine available')
        exit 1
    }

    $result = Await ($engine.RecognizeAsync($soft)) ([Windows.Media.Ocr.OcrResult])

    $lines = @()
    foreach ($line in $result.Lines) {
        $words = @()
        foreach ($word in $line.Words) {
            # report rects in the coordinates of the image we were given, not the upscaled one
            $words += [pscustomobject]@{
                text = $word.Text
                rect = @(
                    [int][Math]::Round($word.BoundingRect.X / $scale),
                    [int][Math]::Round($word.BoundingRect.Y / $scale),
                    [int][Math]::Round($word.BoundingRect.Width / $scale),
                    [int][Math]::Round($word.BoundingRect.Height / $scale)
                )
            }
        }
        $lines += [pscustomobject]@{ text = $line.Text; words = $words }
    }

    $output = [pscustomobject]@{ lines = $lines } | ConvertTo-Json -Depth 6 -Compress

    [Console]::OutputEncoding = [System.Text.Encoding]::UTF8
    [Console]::Out.WriteLine($output)
} catch {
    [Console]::Error.WriteLine('OCR_ERROR: ' + $_.Exception.Message)
    exit 1
}
