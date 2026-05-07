// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MyikaMCPSettings.generated.h"

UENUM()
enum class EMyikaMCPLogLevel : uint8
{
	Quiet,
	Normal,
	Verbose
};

UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "Myika MCP"))
class MYIKAMCP_API UMyikaMCPSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMyikaMCPSettings();

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	/**
	 * Host the TCP listener binds. "127.0.0.1" = local-only (default; safest).
	 * "0.0.0.0" = all interfaces (LAN-reachable; no auth, so trust your network).
	 * "0.0.0.0" is what you want if your Myika desktop app or another machine
	 * needs to talk to this editor.
	 */
	UPROPERTY(EditAnywhere, config, Category = "Myika MCP")
	FString Host = TEXT("127.0.0.1");

	/** TCP port for the Myika MCP listener. */
	UPROPERTY(EditAnywhere, config, Category = "Myika MCP", meta = (ClampMin = 1024, ClampMax = 65535))
	int32 Port = 13379;

	/** If true, the server starts automatically when the editor finishes loading. */
	UPROPERTY(EditAnywhere, config, Category = "Myika MCP")
	bool bAutoStartOnEditorLaunch = true;

	UPROPERTY(EditAnywhere, config, Category = "Myika MCP")
	EMyikaMCPLogLevel LogLevel = EMyikaMCPLogLevel::Normal;

	/** If true, Phase 2 stub commands (pcg_regen, spawn_npc, lightning_strike) are visible. */
	UPROPERTY(EditAnywhere, config, Category = "Myika MCP")
	bool bAllowPhase2Stubs = true;
};
