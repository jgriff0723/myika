// Copyright (c) Myika AI. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Async/Async.h"
#include "Templates/Function.h"

namespace MyikaMCP
{
	/**
	 * Runs Lambda on the game thread and blocks the calling thread until it returns.
	 * Must NOT be called from the game thread itself (would deadlock).
	 *
	 * Times out after TimeoutSeconds and returns the default-constructed T if exceeded;
	 * caller is expected to know the worker thread is short-lived and the editor is responsive.
	 */
	template <typename T>
	T RunOnGameThread(TFunction<T()> Lambda, float TimeoutSeconds = 10.f)
	{
		check(!IsInGameThread());

		TSharedRef<TPromise<T>, ESPMode::ThreadSafe> Promise = MakeShared<TPromise<T>, ESPMode::ThreadSafe>();
		TFuture<T> Future = Promise->GetFuture();

		AsyncTask(ENamedThreads::GameThread, [Promise, Lambda]()
		{
			Promise->SetValue(Lambda());
		});

		const FTimespan Timeout = FTimespan::FromSeconds(TimeoutSeconds);
		if (Future.WaitFor(Timeout))
		{
			return Future.Get();
		}
		return T{};
	}

	/** Void specialization. */
	inline bool RunOnGameThreadVoid(TFunction<void()> Lambda, float TimeoutSeconds = 10.f)
	{
		check(!IsInGameThread());

		TSharedRef<TPromise<bool>, ESPMode::ThreadSafe> Promise = MakeShared<TPromise<bool>, ESPMode::ThreadSafe>();
		TFuture<bool> Future = Promise->GetFuture();

		AsyncTask(ENamedThreads::GameThread, [Promise, Lambda]()
		{
			Lambda();
			Promise->SetValue(true);
		});

		const FTimespan Timeout = FTimespan::FromSeconds(TimeoutSeconds);
		return Future.WaitFor(Timeout);
	}
}
