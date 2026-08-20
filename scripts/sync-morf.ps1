# Rebuild the local, versioned deployment dependency from its neighbouring
# source repository. The copy lets the agent package itself without relying on
# a machine-wide Python installation.
$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$base = if ($env:MORF_SRC_BASE) { $env:MORF_SRC_BASE } else { Split-Path -Parent $root }
$source = if (Test-Path "$base\morfDeploy") { "$base\morfDeploy" } else { "$base\morfDeploy_travail" }
$sourcePackage = Join-Path $source "morfdeploy"
$destination = Join-Path $root "third_party\morf\morfdeploy"

if (-not (Test-Path $sourcePackage)) { throw "morfDeploy source is missing: $sourcePackage" }
if ((Resolve-Path $destination -ErrorAction SilentlyContinue) -and
    -not ((Resolve-Path $destination).Path.StartsWith((Resolve-Path $root).Path))) {
    throw "refusing to replace a deployment copy outside this repository" 
}

Remove-Item -LiteralPath $destination -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $destination | Out-Null
Copy-Item -Recurse "$sourcePackage\*" $destination
Copy-Item (Join-Path $source "VERSION") (Join-Path $destination "VERSION")
Get-ChildItem -LiteralPath $destination -Recurse -Directory -Filter "__pycache__" |
    Remove-Item -Recurse -Force
Write-Output "morfdeploy synchronized."
