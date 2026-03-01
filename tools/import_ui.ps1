param(
    [Parameter(Mandatory = $true)]
    [string]$Source,

    [string]$Destination = "main/ui/editor/generated"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-UiRoot {
    param([string]$Path)

    $full = (Resolve-Path $Path).Path
    if (Test-Path (Join-Path $full "ui.h")) {
        return $full
    }

    $uiSubdir = Join-Path $full "ui"
    if (Test-Path (Join-Path $uiSubdir "ui.h")) {
        return $uiSubdir
    }

    $found = Get-ChildItem -Path $full -Recurse -File -Filter "ui.h" | Select-Object -First 1
    if ($null -ne $found) {
        return Split-Path -Parent $found.FullName
    }

    # LVGL Editor (LVGL XML) exports typically generate <project>_lvgl_ui.h
    $lvglEditorHeader = Get-ChildItem -Path $full -Recurse -File -Filter "*_lvgl_ui.h" | Select-Object -First 1
    if ($null -ne $lvglEditorHeader) {
        return Split-Path -Parent $lvglEditorHeader.FullName
    }

    throw "Could not find ui.h or *_lvgl_ui.h under '$Path'. Export your project as C files first."
}

if (-not (Test-Path $Source)) {
    throw "Source path does not exist: $Source"
}

$uiRoot = Resolve-UiRoot -Path $Source
New-Item -ItemType Directory -Force -Path $Destination | Out-Null
$dest = (Resolve-Path -Path $Destination).Path

Write-Host "UI source: $uiRoot"
Write-Host "Destination: $dest"

$files = Get-ChildItem -Path $uiRoot -Recurse -File |
    Where-Object { $_.Extension -in @(".c", ".h") }

if ($files.Count -eq 0) {
    throw "No C or header files found in '$uiRoot'"
}

foreach ($f in $files) {
    $relative = $f.FullName.Substring($uiRoot.Length).TrimStart('\', '/')
    $targetFile = Join-Path $dest $relative
    $targetDir = Split-Path -Parent $targetFile
    New-Item -ItemType Directory -Force -Path $targetDir | Out-Null
    Copy-Item -Path $f.FullName -Destination $targetFile -Force
}

if (-not (Test-Path (Join-Path $uiRoot "ui.h"))) {
    $mainHeader = Get-ChildItem -Path $dest -Recurse -File -Filter "*_lvgl_ui.h" | Select-Object -First 1
    if ($null -ne $mainHeader) {
        $headerName = $mainHeader.Name
        $baseName = [System.IO.Path]::GetFileNameWithoutExtension($headerName)
        $initFn = "${baseName}_init"

        @"
#ifndef UI_H
#define UI_H

#ifdef __cplusplus
extern "C" {
#endif

void ui_init(void);

#ifdef __cplusplus
}
#endif

#endif
"@ | Set-Content -Path (Join-Path $dest "ui.h")

        @"
#include "ui.h"
#include <stddef.h>
#include "$headerName"

void ui_init(void)
{
    $initFn(NULL);
}
"@ | Set-Content -Path (Join-Path $dest "ui.c")

        Write-Host "Created UI compatibility wrapper: ui.h/ui.c -> $initFn"
    }
}

Write-Host "Imported $($files.Count) files."
Write-Host "If needed, enable CONFIG_GAUGE_USE_EDITOR_UI in menuconfig."
