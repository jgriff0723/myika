// Copyright (c) Myika AI. All rights reserved.

#include "Backends/AnthropicDirectBackend.h"
#include "MyikaMCPChatLog.h"
#include "MyikaMCPChatSettings.h"
#include "MyikaMCPCommandRegistry.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	TSharedRef<FJsonObject> BuildToolDef(const FMyikaMCPCommandRegistry::FCommandInfo& Info)
	{
		// Anthropic tool format: { name, description, input_schema }.
		TSharedRef<FJsonObject> Tool = MakeShared<FJsonObject>();
		Tool->SetStringField(TEXT("name"), Info.Name);
		if (!Info.Description.IsEmpty())
		{
			Tool->SetStringField(TEXT("description"), Info.Description);
		}
		Tool->SetObjectField(TEXT("input_schema"),
			Info.Schema.IsValid() ? Info.Schema : MakeShared<FJsonObject>());
		return Tool;
	}

	TSharedRef<FJsonObject> SerializeMessage(const FMyikaChatMessage& M)
	{
		TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetStringField(TEXT("role"), M.Role);

		TArray<TSharedPtr<FJsonValue>> ContentArr;
		for (const FMyikaChatContentBlock& C : M.Content)
		{
			TSharedRef<FJsonObject> CB = MakeShared<FJsonObject>();
			switch (C.Kind)
			{
			case FMyikaChatContentBlock::EKind::Text:
				CB->SetStringField(TEXT("type"), TEXT("text"));
				CB->SetStringField(TEXT("text"), C.Text);
				break;
			case FMyikaChatContentBlock::EKind::ToolUse:
				CB->SetStringField(TEXT("type"), TEXT("tool_use"));
				CB->SetStringField(TEXT("id"), C.ToolUseId);
				CB->SetStringField(TEXT("name"), C.ToolName);
				CB->SetObjectField(TEXT("input"),
					C.ToolInput.IsValid() ? C.ToolInput : MakeShared<FJsonObject>());
				break;
			case FMyikaChatContentBlock::EKind::ToolResult:
				CB->SetStringField(TEXT("type"), TEXT("tool_result"));
				CB->SetStringField(TEXT("tool_use_id"), C.ToolUseId);
				CB->SetStringField(TEXT("content"), C.Text);
				if (C.bToolResultIsError)
				{
					CB->SetBoolField(TEXT("is_error"), true);
				}
				break;
			}
			ContentArr.Add(MakeShared<FJsonValueObject>(CB));
		}
		Obj->SetArrayField(TEXT("content"), ContentArr);
		return Obj;
	}

	FString CompactJson(const TSharedRef<FJsonObject>& Obj)
	{
		FString Out;
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);
		FJsonSerializer::Serialize(Obj, Writer);
		return Out;
	}
}

FAnthropicDirectBackend::FAnthropicDirectBackend() = default;

FAnthropicDirectBackend::~FAnthropicDirectBackend()
{
	if (InflightRequest.IsValid())
	{
		InflightRequest->CancelRequest();
		InflightRequest.Reset();
	}
}

FString FAnthropicDirectBackend::GetDisplayName() const
{
	const UMyikaMCPChatSettings* S = GetDefault<UMyikaMCPChatSettings>();
	return FString::Printf(TEXT("Anthropic API (%s)"), S ? *S->AnthropicModel : TEXT("?"));
}

void FAnthropicDirectBackend::NewChat()
{
	if (InflightRequest.IsValid())
	{
		InflightRequest->CancelRequest();
		InflightRequest.Reset();
	}
	Messages.Reset();
	ToolDefs.Reset();
	AccumulatedAssistantText.Reset();
	IterationsThisTurn = 0;
	bBusy = false;
}

void FAnthropicDirectBackend::SendUserMessage(const FString& Text)
{
	if (bBusy)
	{
		OnError.Broadcast(TEXT("Backend is busy with an in-flight turn."));
		return;
	}
	if (UMyikaMCPChatSettings::GetEffectiveAnthropicApiKey().IsEmpty())
	{
		OnError.Broadcast(TEXT("ANTHROPIC_API_KEY is not set (settings field is empty and env var is unset)."));
		return;
	}

	Messages.Add(FMyikaChatMessage::UserText(Text));
	StartTurn();
}

void FAnthropicDirectBackend::StartTurn()
{
	bBusy = true;
	IterationsThisTurn = 0;
	AccumulatedAssistantText.Reset();
	IssueRequest();
}

void FAnthropicDirectBackend::IssueRequest()
{
	const UMyikaMCPChatSettings* S = GetDefault<UMyikaMCPChatSettings>();
	if (!S)
	{
		Fail(TEXT("MyikaMCPChat settings unavailable."));
		return;
	}

	// Rebuild the tools array from the registry every turn — handlers can be added
	// or removed at any time (e.g. Phase 2 stub setting toggled).
	ToolDefs.Reset();
	for (const FMyikaMCPCommandRegistry::FCommandInfo& Info : FMyikaMCPCommandRegistry::Get().ListWithMetadata())
	{
		ToolDefs.Add(BuildToolDef(Info));
	}

	// Body.
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("model"), S->AnthropicModel);
	Root->SetNumberField(TEXT("max_tokens"), S->AnthropicMaxTokens);
	const FString System = UMyikaMCPChatSettings::GetEffectiveSystemPrompt();
	if (!System.IsEmpty())
	{
		Root->SetStringField(TEXT("system"), System);
	}

	TArray<TSharedPtr<FJsonValue>> Msgs;
	for (const FMyikaChatMessage& M : Messages)
	{
		Msgs.Add(MakeShared<FJsonValueObject>(SerializeMessage(M)));
	}
	Root->SetArrayField(TEXT("messages"), Msgs);

	if (ToolDefs.Num() > 0)
	{
		TArray<TSharedPtr<FJsonValue>> Tools;
		for (const TSharedRef<FJsonObject>& T : ToolDefs)
		{
			Tools.Add(MakeShared<FJsonValueObject>(T));
		}
		Root->SetArrayField(TEXT("tools"), Tools);
	}

	const FString Body = CompactJson(Root);

	InflightRequest = FHttpModule::Get().CreateRequest();
	InflightRequest->SetURL(S->AnthropicEndpoint);
	InflightRequest->SetVerb(TEXT("POST"));
	InflightRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	InflightRequest->SetHeader(TEXT("x-api-key"), UMyikaMCPChatSettings::GetEffectiveAnthropicApiKey());
	InflightRequest->SetHeader(TEXT("anthropic-version"), S->AnthropicVersion);
	InflightRequest->SetTimeout(180.0f);
	InflightRequest->SetContentAsString(Body);

	TSharedPtr<FAnthropicDirectBackend> Self = AsShared();
	InflightRequest->OnProcessRequestComplete().BindLambda(
		[Self](FHttpRequestPtr, FHttpResponsePtr Resp, bool bConnectedSuccessfully)
		{
			const int32 Status = Resp.IsValid() ? Resp->GetResponseCode() : 0;
			const FString ResponseBody = Resp.IsValid() ? Resp->GetContentAsString() : FString();
			Self->OnHttpComplete(ResponseBody, Status, bConnectedSuccessfully);
		});

	if (!InflightRequest->ProcessRequest())
	{
		Fail(TEXT("Failed to start HTTP request."));
		return;
	}
}

void FAnthropicDirectBackend::OnHttpComplete(const FString& Body, int32 StatusCode, bool bConnectedSuccessfully)
{
	InflightRequest.Reset();

	if (!bConnectedSuccessfully)
	{
		Fail(TEXT("Network error: could not reach api.anthropic.com"));
		return;
	}

	TSharedPtr<FJsonObject> Json;
	TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(Body);
	if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
	{
		Fail(FString::Printf(TEXT("Invalid JSON response (status %d): %s"), StatusCode, *Body.Left(512)));
		return;
	}
	if (StatusCode < 200 || StatusCode >= 300)
	{
		FString Msg;
		const TSharedPtr<FJsonObject>* ErrorObj = nullptr;
		if (Json->TryGetObjectField(TEXT("error"), ErrorObj) && ErrorObj && (*ErrorObj).IsValid())
		{
			(*ErrorObj)->TryGetStringField(TEXT("message"), Msg);
		}
		Fail(FString::Printf(TEXT("HTTP %d: %s"), StatusCode, Msg.IsEmpty() ? *Body.Left(512) : *Msg));
		return;
	}

	// Parse content blocks.
	FString StopReason;
	Json->TryGetStringField(TEXT("stop_reason"), StopReason);

	TArray<FMyikaChatContentBlock> AssistantBlocks;
	const TArray<TSharedPtr<FJsonValue>>* ContentArr = nullptr;
	if (Json->TryGetArrayField(TEXT("content"), ContentArr) && ContentArr)
	{
		for (const TSharedPtr<FJsonValue>& V : *ContentArr)
		{
			const TSharedPtr<FJsonObject>* BlockObj = nullptr;
			if (!V->TryGetObject(BlockObj) || !BlockObj || !(*BlockObj).IsValid()) continue;
			const TSharedPtr<FJsonObject>& B = *BlockObj;

			FString Type;
			B->TryGetStringField(TEXT("type"), Type);

			FMyikaChatContentBlock CB;
			if (Type == TEXT("text"))
			{
				CB.Kind = FMyikaChatContentBlock::EKind::Text;
				B->TryGetStringField(TEXT("text"), CB.Text);
				if (!CB.Text.IsEmpty())
				{
					AccumulatedAssistantText += CB.Text;
					OnAssistantText.Broadcast(CB.Text);
				}
			}
			else if (Type == TEXT("tool_use"))
			{
				CB.Kind = FMyikaChatContentBlock::EKind::ToolUse;
				B->TryGetStringField(TEXT("id"), CB.ToolUseId);
				B->TryGetStringField(TEXT("name"), CB.ToolName);
				const TSharedPtr<FJsonObject>* InputObj = nullptr;
				if (B->TryGetObjectField(TEXT("input"), InputObj) && InputObj && (*InputObj).IsValid())
				{
					CB.ToolInput = *InputObj;
				}
				else
				{
					CB.ToolInput = MakeShared<FJsonObject>();
				}

				FMyikaChatToolCall Call;
				Call.Id = CB.ToolUseId;
				Call.Name = CB.ToolName;
				Call.Input = CB.ToolInput;
				OnAssistantTool.Broadcast(Call);
			}
			else
			{
				continue;
			}
			AssistantBlocks.Add(CB);
		}
	}

	// Append assistant message to history (so further turns see it).
	{
		FMyikaChatMessage AsstMsg;
		AsstMsg.Role = TEXT("assistant");
		AsstMsg.Content = AssistantBlocks;
		Messages.Add(MoveTemp(AsstMsg));
	}

	if (StopReason == TEXT("tool_use"))
	{
		HandleToolUseBlocks(AssistantBlocks);
	}
	else
	{
		Finish(StopReason, AccumulatedAssistantText);
	}
}

void FAnthropicDirectBackend::HandleToolUseBlocks(const TArray<FMyikaChatContentBlock>& AssistantBlocks)
{
	const UMyikaMCPChatSettings* S = GetDefault<UMyikaMCPChatSettings>();
	const int32 Cap = S ? S->MaxToolIterations : 10;
	if (++IterationsThisTurn > Cap)
	{
		Fail(FString::Printf(TEXT("Hit MaxToolIterations cap (%d) — aborting tool-use loop."), Cap));
		return;
	}

	// Build the tool_result user message.
	FMyikaChatMessage Reply;
	Reply.Role = TEXT("user");

	for (const FMyikaChatContentBlock& B : AssistantBlocks)
	{
		if (B.Kind != FMyikaChatContentBlock::EKind::ToolUse) continue;

		const TSharedRef<FJsonObject> RegistryResponse =
			FMyikaMCPCommandRegistry::Get().Dispatch(B.ToolUseId, B.ToolName, B.ToolInput);

		// Format the registry response as a JSON string for tool_result.content.
		FString ResultText;
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&ResultText);
		FJsonSerializer::Serialize(RegistryResponse, Writer);

		bool bOk = true;
		RegistryResponse->TryGetBoolField(TEXT("ok"), bOk);

		FMyikaChatContentBlock TR;
		TR.Kind = FMyikaChatContentBlock::EKind::ToolResult;
		TR.ToolUseId = B.ToolUseId;
		TR.Text = ResultText;
		TR.bToolResultIsError = !bOk;
		Reply.Content.Add(MoveTemp(TR));
	}

	Messages.Add(MoveTemp(Reply));
	IssueRequest();
}

void FAnthropicDirectBackend::Fail(const FString& Message)
{
	bBusy = false;
	UE_LOG(LogMyikaMCPChat, Warning, TEXT("AnthropicDirect: %s"), *Message);
	OnError.Broadcast(Message);
}

void FAnthropicDirectBackend::Finish(const FString& StopReason, const FString& AssistantText)
{
	bBusy = false;
	OnTurnComplete.Broadcast(AssistantText, StopReason);
}
