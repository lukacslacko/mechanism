#pragma once
#include "CoreMinimal.h"

// The runtime's display period, independent of application frame rate / ASW.
// Zero until a successful xrWaitFrame; read safely from the game thread.
MATERIALHALLXR_API float GetHallXRRefreshRate();
