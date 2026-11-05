// Copyright (c) 2026 Sanjyot Dahale. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<bool> CVarChronoDrawPath(
	TEXT("Chrono.DrawPath"),
	false,
	TEXT("Draw the temporal buffer's recorded path and the blast AoE. 0 = off, 1 = on"));