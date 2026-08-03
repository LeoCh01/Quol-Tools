param(
    [Parameter(Mandatory = $true)][string]$ImagePath
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
    [Windows.Graphics.Imaging.BitmapDecoder, Windows.Graphics, ContentType = WindowsRuntime] | Out-Null

    if (-not (Test-Path -LiteralPath $ImagePath)) {
        [Console]::Error.WriteLine('OCR_ERROR: image not found')
        exit 1
    }

    $stream = [System.IO.WindowsRuntimeStreamExtensions]::AsRandomAccessStream([System.IO.File]::OpenRead($ImagePath))
    $decoder = Await ([Windows.Graphics.Imaging.BitmapDecoder]::CreateAsync($stream)) ([Windows.Graphics.Imaging.BitmapDecoder])
    $soft = Await ($decoder.GetSoftwareBitmapAsync()) ([Windows.Graphics.Imaging.SoftwareBitmap])

    $engine = [Windows.Media.Ocr.OcrEngine]::TryCreateFromUserProfileLanguages()
    if (-not $engine) {
        [Console]::Error.WriteLine('OCR_ERROR: no OCR engine available')
        exit 1
    }

    $result = Await ($engine.RecognizeAsync($soft)) ([Windows.Media.Ocr.OcrResult])

    $lines = foreach ($line in $result.Lines) { $line.Text }
    $output = $lines -join "`n"

    [Console]::OutputEncoding = [System.Text.Encoding]::UTF8
    [Console]::Out.WriteLine($output)
} catch {
    [Console]::Error.WriteLine('OCR_ERROR: ' + $_.Exception.Message)
    exit 1
}