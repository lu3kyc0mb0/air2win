param(
    [string]$QtRoot = "$PSScriptRoot/../work/Qt/6.8.3/mingw_64",
    [string]$QtCompilerBin = "$PSScriptRoot/../work/Qt/Tools/mingw1310_64/bin",
    [string]$UcrtCompilerBin = $env:AIR2WIN_UCRT_COMPILER,
    [string]$Python = 'python',
    [switch]$SkipReceiver,
    [switch]$Package
)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path "$PSScriptRoot/..").Path
$savedPath = $env:PATH
$savedPkgConfig = $env:PKG_CONFIG_PATH
if (!$UcrtCompilerBin) {
    foreach ($candidate in @('F:/Program Files/codeblocks/MinGW/bin', 'C:/Program Files/CodeBlocks/MinGW/bin')) {
        if (Test-Path "$candidate/gcc.exe") { $UcrtCompilerBin = $candidate; break }
    }
}
function Run-Checked([string]$Tool, [string[]]$Arguments) {
    & $Tool @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Tool failed with exit code $LASTEXITCODE" }
}
Push-Location $projectRoot
try {
    $env:PATH = "$projectRoot/work/python-tools/cmake/data/bin;$projectRoot/work/python-tools/bin;$QtCompilerBin;$QtRoot/bin;$savedPath"
    Run-Checked 'cmake' @('-S', '.', '-B', 'build-ui', '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release', "-DCMAKE_PREFIX_PATH=$QtRoot")
    Run-Checked 'cmake' @('--build', 'build-ui', '-j', '6')
    Run-Checked 'ctest' @('--test-dir', 'build-ui', '--output-on-failure')
    if (!$SkipReceiver) {
        if (!$UcrtCompilerBin -or !(Test-Path "$UcrtCompilerBin/gcc.exe")) {
            throw 'Pass -UcrtCompilerBin pointing to a MinGW UCRT compiler (separate from Qt MinGW).'
        }
        Run-Checked $Python @('scripts/patch-uxplay.py')
        $env:PATH = "$UcrtCompilerBin;$projectRoot/work/media-deps/ucrt64/bin;$env:PATH"
        $env:PKG_CONFIG_PATH = "$projectRoot/work/media-deps/ucrt64/lib/pkgconfig"
        Run-Checked 'cmake' @('-S', 'work/uxplay', '-B', 'work/build-uxplay', '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release', '-DNO_MARCH_NATIVE=ON', "-DCMAKE_PREFIX_PATH=$projectRoot/work/media-deps/ucrt64")
        Run-Checked 'cmake' @('--build', 'work/build-uxplay', '-j', '6')
    }
    if ($Package) {
        $env:PATH = "$QtCompilerBin;$QtRoot/bin;$savedPath"
        New-Item -ItemType Directory -Force dist/Air2Win | Out-Null
        Copy-Item build-ui/Air2Win.exe dist/Air2Win/Air2Win.exe
        Run-Checked "$QtRoot/bin/windeployqt.exe" @('--release', '--verbose', '0', '--no-translations', '--no-system-d3d-compiler', '--no-opengl-sw', '--qmldir', "$projectRoot/qml", "$projectRoot/dist/Air2Win/Air2Win.exe")
        if (!$SkipReceiver) {
            Run-Checked $Python @('scripts/package-runtime.py', '--objdump', "$UcrtCompilerBin/objdump.exe")
        }
        Copy-Item LICENSE,THIRD_PARTY_NOTICES.md dist/Air2Win
        Copy-Item docs/QUICKSTART.md dist/Air2Win/START-HERE.md
        New-Item -ItemType Directory -Force dist/Air2Win/licenses | Out-Null
        Copy-Item licenses/qt dist/Air2Win/licenses -Recurse -Force
        if (Test-Path "$QtRoot/licenses") { Copy-Item "$QtRoot/licenses/*" dist/Air2Win/licenses }
        Write-Output "Portable application: $projectRoot/dist/Air2Win/Air2Win.exe"
    }
} finally {
    $env:PATH = $savedPath
    $env:PKG_CONFIG_PATH = $savedPkgConfig
    Pop-Location
}
