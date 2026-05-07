// Copyright (c) Myika AI. All rights reserved.

#include "Backends/ClaudeCodeBackend.h"
#include "MyikaMCPChatLog.h"
#include "MyikaMCPChatSettings.h"
#include "Async/Async.h"
#include "Dom/JsonObject.h"
#include "HAL/PlatformProcess.h"
#include "HAL/RunnableThread.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

FClaudeCodeBackend::FClaudeCodeBackend() = default;

FClaudeCodeBackend::~FClaudeCodeBackend()
{
	Stop();
	if (Thread)
	{
		Thread->WaitForCompletion();
		delete Thread;
		Thread = nullptr;
	}
}

FString FClaudeCodeBackend::GetDisplayName() const
{
	return TEXT("Claude Code (local CLI)");
}

void FClaudeCodeBackend::NewChat()
{
	if (bBusy)
	{
		// Politely refuse — let the in-flight turn settle first.
		OnError.Broadcast(TEXT("Cannot start a new chat mid-turn; wait for the current response."));
		return;
	}
	History.Reset();
}

void FClaudeCodeBackend::SendUserMessage(const FString& Text)
{
	if (bBusy)
	{
		OnError.Broadcast(TEXT("Backend is busy with an in-flight turn."));
		return;
	}
	bBusy = true;
	PendingMessage = Text;

	// Spawn a fresh worker thread per turn so blocking pipe I/O doesn't block
	// the game thread. The runnable owns nothing except this — the next
	// SendUserMessage waits for the previous thread to finish (bBusy guard).
	if (Thread)
	{
		Thread->WaitForCompletion();
		delete Thread;
		Thread = nullptr;
	}
	Thread = FRunnableThread::Create(this, TEXT("MyikaClaudeCodeBackend"));
}

void FClaudeCodeBackend::Stop()
{
	bStopRequested = true;
}

void FClaudeCodeBackend::Exit()
{
}

void FClaudeCodeBackend::DispatchOnGameThread(TFunction<void()> Fn)
{
	TFunction<void()> F = MoveTemp(Fn);
	AsyncTask(ENamedThreads::GameThread, [F = MoveTemp(F)]() { F(); });
}

FString FClaudeCodeBackend::ResolveExePath() const
{
	const UMyikaMCPChatSettings* S = GetDefault<UMyikaMCPChatSettings>();
	if (S && !S->ClaudeCodeExePath.IsEmpty())
	{
		return S->ClaudeCodeExePath;
	}
	// Bare "claude" relies on PATH resolution.
	return TEXT("claude");
}

FString FClaudeCodeBackend::BuildPromptFromHistory(const FString& NewUserMessage) const
{
	if (History.Num() == 0)
	{
		return NewUserMessage;
	}
	FString Prompt = TEXT("Previous conversation:\n\n");
	for (const TPair<FString, FString>& Pair : History)
	{
		if (Pair.Key == TEXT("user"))
		{
			Prompt += FString::Printf(TEXT("User: %s\n\n"), *Pair.Value);
		}
		else if (Pair.Key == TEXT("assistant"))
		{
			Prompt += FString::Printf(TEXT("Assistant: %s\n\n"), *Pair.Value);
		}
	}
	Prompt += FString::Printf(TEXT("User: %s"), *NewUserMessage);
	return Prompt;
}

void FClaudeCodeBackend::ParseStreamLine(const FString& Line, FString& InOutAccumulatedText, int32& InOutEmittedLen)
{
	if (Line.TrimStartAndEnd().IsEmpty())
	{
		return;
	}

	TSharedPtr<FJsonObject> Event;
	TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(Line);
	if (!FJsonSerializer::Deserialize(Reader, Event) || !Event.IsValid())
	{
		return;
	}

	FString EventType;
	Event->TryGetStringField(TEXT("type"), EventType);

	if (EventType == TEXT("assistant"))
	{
		const TSharedPtr<FJsonObject>* Message = nullptr;
		if (!Event->TryGetObjectField(TEXT("message"), Message) || !Message || !(*Message).IsValid())
		{
			return;
		}
		const TArray<TSharedPtr<FJsonValue>>* Content = nullptr;
		if (!(*Message)->TryGetArrayField(TEXT("content"), Content) || !Content)
		{
			return;
		}

		FString EventText;
		for (const TSharedPtr<FJsonValue>& V : *Content)
		{
			const TSharedPtr<FJsonObject>* BlockObj = nullptr;
			if (!V->TryGetObject(BlockObj) || !BlockObj || !(*BlockObj).IsValid()) continue;
			const TSharedPtr<FJsonObject>& B = *BlockObj;

			FString Type;
			B->TryGetStringField(TEXT("type"), Type);

			if (Type == TEXT("text"))
			{
				FString T;
				B->TryGetStringField(TEXT("text"), T);
				EventText += T;
			}
			else if (Type == TEXT("tool_use"))
			{
				FMyikaChatToolCall Call;
				B->TryGetStringField(TEXT("id"), Call.Id);
				B->TryGetStringField(TEXT("name"), Call.Name);
				const TSharedPtr<FJsonObject>* InputObj = nullptr;
				if (B->TryGetObjectField(TEXT("input"), InputObj) && InputObj && (*InputObj).IsValid())
				{
					Call.Input = *InputObj;
				}
				else
				{
					Call.Input = MakeShared<FJsonObject>();
				}
				TSharedPtr<FClaudeCodeBackend> Self = AsShared();
				DispatchOnGameThread([Self, Call]()
				{
					Self->OnAssistantTool.Broadcast(Call);
				});
			}
			// "thinking" and unknown block kinds: ignore.
		}

		// New turn detected — reset delta tracking. (Mirrors claude.rs.)
		if (EventText.Len() < InOutEmittedLen)
		{
			InOutEmittedLen = 0;
		}
		if (EventText.Len() > InOutEmittedLen)
		{
			const FString Delta = EventText.Mid(InOutEmittedLen);
			InOutEmittedLen = EventText.Len();
			TSharedPtr<FClaudeCodeBackend> Self = AsShared();
			DispatchOnGameThread([Self, Delta]()
			{
				Self->OnAssistantText.Broadcast(Delta);
			});
		}
		InOutAccumulatedText = EventText;
	}
	else if (EventType == TEXT("result"))
	{
		// Fallback: some runs only deliver the final text via the `result` event.
		FString ResultText;
		if (Event->TryGetStringField(TEXT("result"), ResultText) && !ResultText.IsEmpty()
			&& InOutAccumulatedText.IsEmpty())
		{
			InOutAccumulatedText = ResultText;
			TSharedPtr<FClaudeCodeBackend> Self = AsShared();
			DispatchOnGameThread([Self, ResultText]()
			{
				Self->OnAssistantText.Broadcast(ResultText);
			});
		}
	}
	// Other event types ignored for MVP.
}

uint32 FClaudeCodeBackend::Run()
{
	const FString Message = PendingMessage;
	PendingMessage.Reset();

	// Append the user turn to history ahead of the call so it's reflected in
	// the prompt we build (the claude.rs pattern).
	History.Add(TPair<FString, FString>(TEXT("user"), Message));

	const FString System = UMyikaMCPChatSettings::GetEffectiveSystemPrompt();
	const FString PromptBody = BuildPromptFromHistory(Message);
	FString Prompt = System.IsEmpty()
		? PromptBody
		: FString::Printf(TEXT("[System instructions follow. Treat them as your operating context, then respond to the user's message.]\n\n%s\n\n---\n\n%s"), *System, *PromptBody);
	const FString ExePath = ResolveExePath();

	// We pass the prompt as the final positional CLI argument (NOT via stdin):
	// UE's stdin pipe + Node CLI doesn't work — claude reports "Input must be
	// provided either through stdin or as a prompt argument" even after a
	// successful WritePipe. Argv is reliable for Myika-sized prompts (short).
	// Windows command-line cap is ~32 KB — well above what we'll ever send.
	FString PromptArg = Prompt;
	PromptArg.ReplaceInline(TEXT("\\"), TEXT("\\\\"));
	PromptArg.ReplaceInline(TEXT("\""), TEXT("\\\""));

	FString Args = TEXT("--print --verbose --output-format stream-json --no-session-persistence --permission-mode bypassPermissions");
	Args += FString::Printf(TEXT(" \"%s\""), *PromptArg);

	void* StdOutRead = nullptr;  void* StdOutWrite = nullptr;
	void* StdErrRead = nullptr;  void* StdErrWrite = nullptr;
	if (!FPlatformProcess::CreatePipe(StdOutRead, StdOutWrite))
	{
		DispatchOnGameThread([Self = AsShared()]() {
			Self->OnError.Broadcast(TEXT("Failed to create stdout pipe."));
			Self->bBusy = false;
		});
		return 1;
	}
	if (!FPlatformProcess::CreatePipe(StdErrRead, StdErrWrite))
	{
		FPlatformProcess::ClosePipe(StdOutRead, StdOutWrite);
		DispatchOnGameThread([Self = AsShared()]() {
			Self->OnError.Broadcast(TEXT("Failed to create stderr pipe."));
			Self->bBusy = false;
		});
		return 1;
	}

	UE_LOG(LogMyikaMCPChat, Log, TEXT("Spawning claude with %d-char prompt (UTF-8 args)"), Prompt.Len());

	uint32 ProcessId = 0;
	FProcHandle Proc = FPlatformProcess::CreateProc(
		*ExePath,
		*Args,
		false,                  // bLaunchDetached
		true,                   // bLaunchHidden — the CREATE_NO_WINDOW analogue
		true,                   // bLaunchReallyHidden
		&ProcessId,
		0,                      // PriorityModifier
		nullptr,                // OptionalWorkingDirectory
		StdOutWrite,            // PipeWriteChild  (stdout)
		nullptr,                // PipeReadChild   (no stdin — prompt is in argv)
		StdErrWrite             // PipeWriteChildError (stderr)
	);

	if (!Proc.IsValid())
	{
		FPlatformProcess::ClosePipe(StdOutRead, StdOutWrite);
		FPlatformProcess::ClosePipe(StdErrRead, StdErrWrite);
		DispatchOnGameThread([Self = AsShared(), ExePath]() {
			Self->OnError.Broadcast(FString::Printf(TEXT("Failed to spawn claude (path: %s). Is it on PATH?"), *ExePath));
			Self->bBusy = false;
		});
		return 1;
	}

	// Read stdout line-by-line. ReadPipe returns whatever's available; we buffer
	// and split on '\n' to get NDJSON frames. Concurrently drain stderr so the
	// child doesn't block on a full pipe.
	FString StreamBuffer;
	FString StderrBuffer;
	FString AccumulatedText;
	int32 EmittedLen = 0;

	while (FPlatformProcess::IsProcRunning(Proc) && !bStopRequested)
	{
		const FString Chunk = FPlatformProcess::ReadPipe(StdOutRead);
		if (!Chunk.IsEmpty())
		{
			StreamBuffer += Chunk;
			int32 Newline;
			while (StreamBuffer.FindChar(TEXT('\n'), Newline))
			{
				const FString Line = StreamBuffer.Left(Newline);
				StreamBuffer = StreamBuffer.Mid(Newline + 1);
				ParseStreamLine(Line, AccumulatedText, EmittedLen);
			}
		}
		const FString ErrChunk = FPlatformProcess::ReadPipe(StdErrRead);
		if (!ErrChunk.IsEmpty())
		{
			StderrBuffer += ErrChunk;
		}
		if (Chunk.IsEmpty() && ErrChunk.IsEmpty())
		{
			FPlatformProcess::Sleep(0.01f);
		}
	}

	// Drain tails on both streams.
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		const FString Tail = FPlatformProcess::ReadPipe(StdOutRead);
		if (!Tail.IsEmpty()) StreamBuffer += Tail;
		const FString ErrTail = FPlatformProcess::ReadPipe(StdErrRead);
		if (!ErrTail.IsEmpty()) StderrBuffer += ErrTail;
	}
	{
		int32 Newline;
		while (StreamBuffer.FindChar(TEXT('\n'), Newline))
		{
			const FString Line = StreamBuffer.Left(Newline);
			StreamBuffer = StreamBuffer.Mid(Newline + 1);
			ParseStreamLine(Line, AccumulatedText, EmittedLen);
		}
		if (!StreamBuffer.IsEmpty())
		{
			ParseStreamLine(StreamBuffer, AccumulatedText, EmittedLen);
			StreamBuffer.Reset();
		}
	}

	int32 ReturnCode = 0;
	FPlatformProcess::GetProcReturnCode(Proc, &ReturnCode);
	FPlatformProcess::CloseProc(Proc);
	FPlatformProcess::ClosePipe(StdOutRead, StdOutWrite);
	FPlatformProcess::ClosePipe(StdErrRead, StdErrWrite);

	if (!StderrBuffer.IsEmpty())
	{
		UE_LOG(LogMyikaMCPChat, Warning, TEXT("claude stderr (%d bytes):\n%s"), StderrBuffer.Len(), *StderrBuffer);
	}

	// Append assistant turn to history.
	if (!AccumulatedText.IsEmpty())
	{
		History.Add(TPair<FString, FString>(TEXT("assistant"), AccumulatedText));
	}

	const FString FinalText = AccumulatedText;
	const bool bWasStopped = bStopRequested;
	const int32 Exit = ReturnCode;

	DispatchOnGameThread([Self = AsShared(), FinalText, bWasStopped, Exit]()
	{
		Self->bBusy = false;
		if (bWasStopped)
		{
			Self->OnError.Broadcast(TEXT("Claude run cancelled."));
		}
		else if (Exit != 0 && FinalText.IsEmpty())
		{
			Self->OnError.Broadcast(FString::Printf(TEXT("claude exited with code %d and produced no output."), Exit));
		}
		else
		{
			Self->OnTurnComplete.Broadcast(FinalText, Exit == 0 ? TEXT("end_turn") : TEXT("error"));
		}
	});

	bStopRequested = false;
	return 0;
}
