$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path "$PSScriptRoot/..").Path
$app = Start-Process -FilePath "$projectRoot/dist/Air2Win/Air2Win.exe" -ArgumentList '--receiver-check' -WindowStyle Hidden -PassThru
$child = $null
try {
    $deadline = (Get-Date).AddSeconds(12)
    while ((Get-Date) -lt $deadline -and !$app.HasExited) {
        $child = Get-CimInstance Win32_Process -Filter "ParentProcessId = $($app.Id)" | Where-Object Name -eq 'uxplay.exe' | Select-Object -First 1
        if ($child) { break }
        Start-Sleep -Milliseconds 100
    }
    if (!$child) { throw 'Receiver child was not observed.' }
    # Allow QProcess::started to assign its Windows Job Object.
    Start-Sleep -Milliseconds 150
    if (!$app.HasExited) { Stop-Process -Id $app.Id -Force }
    $deadline = (Get-Date).AddSeconds(3)
    while ((Get-Date) -lt $deadline -and (Get-Process -Id $child.ProcessId -ErrorAction SilentlyContinue)) {
        Start-Sleep -Milliseconds 100
    }
    if (Get-Process -Id $child.ProcessId -ErrorAction SilentlyContinue) {
        throw 'Receiver survived termination of its parent.'
    }
    Write-Output 'Forced parent exit leaves no receiver process: OK'
} finally {
    if (!$app.HasExited) { Stop-Process -Id $app.Id -Force -ErrorAction SilentlyContinue }
}
