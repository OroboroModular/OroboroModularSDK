<#
.SYNOPSIS
    Builds the module in this folder (a Rust crate, or a Faust source) for
    Windows and Linux, each for x86-64 and ARM64, into one module file, in
    Docker.

.DESCRIPTION
    Run it in a module's folder (where its Cargo.toml, or its one .dsp, is):

        cd my-module
        ..\OroboroModularSDK\scripts\build-all.ps1

    The module file goes to build\<Name>.oromodule (or -Out). The first time
    it makes the build image (docker\Dockerfile, oromod-build:<oromod's
    version>), which takes a while; -Rebuild makes it again. The folder the
    module and the SDK are both in is mounted, so a path from the module to
    the SDK's crate holds in the container. docs\native-modules.md, "Every
    platform at once".

.PARAMETER Out
    The folder the module file goes to (build).

.PARAMETER Rebuild
    Make the build image again (after updating the SDK, say).
#>
param(
    [string]$Out = 'build',
    [switch]$Rebuild
)
# (no 'Stop': Docker writes its progress to stderr, which Windows PowerShell
# takes for errors once it's redirected; its exit codes say how it went)

$sdk = Split-Path -Parent $PSScriptRoot
$project = (Get-Location).Path
$sources = @(Get-ChildItem -Path $project -Filter '*.dsp' -File)
if (-not (Test-Path (Join-Path $project 'Cargo.toml')) -and $sources.Count -ne 1) {
    throw "No Cargo.toml here and not one .dsp: run it in a module's folder."
}
if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
    throw "Docker isn't installed (docker.com)."
}

# oromod's version: its source's (the SDK's repository), else the oromod the
# published SDK comes with
$source = Join-Path $sdk 'oromod\Cargo.toml'
if (Test-Path $source) {
    $version = (Get-Content $source | Where-Object { $_ -match '^version\s*=\s*"(.+)"' } | Select-Object -First 1) -replace '^version\s*=\s*"(.+)"', '$1'
} else {
    $version = ((& (Join-Path $sdk 'bin\oromod.exe') --version) -split ' ')[1]
}
$image = "oromod-build:$version"
$have = docker image ls -q $image
if ($Rebuild -or -not $have) {
    Write-Host "Making the build image $image (the first time takes a while)"
    docker build --platform linux/amd64 -f (Join-Path $sdk 'docker\Dockerfile') --build-arg "OROMOD_VERSION=$version" -t $image $sdk
    if ($LASTEXITCODE -ne 0) { throw "Making the build image failed." }
}

# the folder both are in
$a = $project.TrimEnd('\').Split('\')
$b = $sdk.TrimEnd('\').Split('\')
$n = 0
while ($n -lt $a.Length -and $n -lt $b.Length -and $a[$n] -ieq $b[$n]) { $n++ }
if ($n -eq 0) { throw "The module and the SDK are on different drives: put them in one folder." }
$root = ($a[0..($n - 1)] -join '\')
if ($n -eq 1) { $root += '\' }
$rel = ($a | Select-Object -Skip $n) -join '/'

docker run --rm --platform linux/amd64 -v "${root}:/src" -w "/src/$rel" $image build-all $Out
if ($LASTEXITCODE -ne 0) { throw "The build failed." }
