// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "CoreMinimal.h"

class FUICommandList;

/** Owns the level-editor toolbar entry and its UI command list. */
class FMyikaMCPToolbarExtension : public TSharedFromThis<FMyikaMCPToolbarExtension>
{
public:
	void Register();
	void Unregister();

private:
	void OnStart();
	void OnStop();
	void OnRestart();
	void OnOpenSettings();
	bool IsRunning() const;

	TSharedPtr<FUICommandList> CommandList;
	bool bRegistered = false;
};
