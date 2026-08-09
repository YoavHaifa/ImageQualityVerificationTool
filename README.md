# IQV Tool (Image Quality Verification Tool)

A native C++/MFC Windows desktop application for detecting and scoring **ring artifacts** in Arineta CT (computed tomography) scan volumes. It loads a CT dataset, locates the rotation center, builds averaged "wide" slices, scores every slice for ring-artifact severity, and lets the user step through the worst-scoring slices ("peaks") for visual review.

## Solution layout

`IQV_tool.sln` builds three projects:

| Project | Location | Role |
|---|---|---|
| `DemoApp` | `DemoApp/` | The application executable (MFC dialog app) |
| `yUtils` | `../yUtils/` (sibling repo) | Shared utilities: DICOM I/O, file logging, math/dialog helpers |
| `ImageRLib` | `../ImageRLib/` (sibling repo) | Imaging library: volumes, ROI, segmentation, image comparison |

> **Note:** `yUtils` and `ImageRLib` are separate repositories expected to be checked out as siblings of this repo (e.g. `D:\SW_IR\yUtils`, `D:\SW_IR\ImageRLib`), since `DemoApp` references them via relative paths (`..\..\yUtils\...`, `..\..\ImageRLib\...`).

## Key components (`DemoApp/`)

- **`ArinetaImages.h/.cpp`** (`CArinetaImages`) — loads Arineta DICOM CT volumes, computes the rotation center, and builds averaged "wide" volumes from consecutive slices.
- **`RadiusImage.h/.cpp`** — builds a per-pixel radial-distance map from the rotation center, used to sample ring positions.
- **`RingsScorer.h/.cpp`** / **`ImageRingsScorer.h/.cpp`** (`CRingsScorer`) — scores each slice for ring-artifact severity and ranks "peaks" (worst slices) for navigation (`DisplayMaxPeak` / `NextPeak` / `PrevPeak`).
- **`Config.h/.cpp`** (`CConfig gConfig`) — global configuration: intensity thresholds (`mMinThreshold`/`mMaxThreshold`, CT bias 1024), erode level, slice-averaging width, score-graph output directory. Supports save/load to file.
- **`DemoAppDlg.h/.cpp`** (`CDemoAppDlg`) — main MFC dialog UI: opens datasets, displays the image viewer, ROI tools, color mapping, and peak navigation buttons (Max/Next/Prev, Add ROI, Shared, etc.).
- **`DemoApp.cpp`** — application entry point (`CDemoAppApp theApp`); initializes config/logging and shows the main dialog.

Supporting shared libraries (outside this repo):
- **`yUtils`** — `CFileLogger` (global `gfLog`) file logging, DICOM tag helpers, generic dialog/math utilities.
- **`ImageRLib`** — `CArchivesImages`, `CTSharedImage`, `CDataCoordinates`, masking/segmentation, and image-comparison primitives used by the viewer.

## Build

1. Ensure the sibling repos `yUtils` and `ImageRLib` are checked out next to this repo.
2. Open `IQV_tool.sln` in Visual Studio (toolset v145, Windows 10 SDK).
3. Build the `Debug` or `Release` configuration for `x86` or `x64`.
4. Run `DemoApp` as the startup project.

No NuGet/external package dependencies — this is a pure native C++ project.

## Typical flow

1. Load a CT DICOM/binary dataset via the dialog.
2. The tool computes the rotation center and builds averaged wide slices.
3. `CRingsScorer` scores every slice for ring-artifact severity and orders peaks by severity.
4. The user reviews the worst slices via the Max/Next/Prev peak buttons; results are logged (`gfLog`) and score graphs are written to the configured directory (default `d:\Log\IQV_Graphs`).
