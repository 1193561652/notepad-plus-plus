#pragma once

#include "MISC/PluginsManager/PluginUpdatePlan.h"

class PluginUpdateExecutor
{
public:
    static bool apply(const PluginUpdatePlan& plan,
                      QString* error = nullptr);
};
