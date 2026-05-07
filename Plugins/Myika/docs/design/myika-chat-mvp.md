# Myika in-editor chat — MVP design

## Problem

Driving Myika global state from natural language today requires the user to launch a separate MCP client (Claude Desktop, Cursor, Claude Code) and have the editor + plugin running in parallel. That's a friction tax — the user is already in the editor when they want to say "make it 5pm and stormy". Building a chat panel inside the editor makes the round-trip one app instead of two.

## Goals (in scope for MVP)

1. Dockable Slate panel with a conversation history + input box, opened from toolbar or Window menu.
2. Direct calls to the Anthropic Messages API. The Myika tool surface is forwarded as Anthropic tool definitions; tool calls dispatch via the existing `FMyikaMCPCommandRegistry` in-process (no TCP).
3. API key from environment variable (`ANTHROPIC_API_KEY`) — never written to ini files. Settings UI shows status (set / unset) but cannot store the key.
4. Plaintext rendering of agent output. No markdown formatting yet.
5. Multi-turn conversation in memory; cleared on tab close. No persistence to disk.

## Non-goals (deferred)

- Streaming responses (typing-effect UX). Non-streaming for MVP — cleaner code, ships sooner; revisit when tool-call latency feels off.
- OpenRouter / multi-provider abstraction. Anthropic only.
- Hosted free-tier proxy (VibeUE / FlopMCP pattern).
- Markdown / code-block rendering. Use `SMultiLineEditableTextBox` read-only.
- Persistence (save/load conversations to disk).
- Speech I/O.
- Vision / screenshot tool feedback.

## Architecture

New editor-only module `MyikaMCPChat`, separate from `MyikaMCP` so users who only want the MCP server bridge don't pull in HTTP / Slate / Anthropic.

```
SMyikaChatPanel (Slate)         FMyikaChatSession                 FAnthropicClient
  ┌──────────────────┐          ┌─────────────────────┐           ┌────────────────┐
  │ history view     │  ◄──────►│ messages[]          │ ──────►   │ POST /v1/messages
  │ input box        │          │ tools_in_progress   │           │ (FHttpModule)  │
  │ status line      │          │ Step():             │  ◄────── │                │
  └──────────────────┘          │   send + handle     │           └────────────────┘
                                 │   tool_use blocks  │
                                 └──────────┬──────────┘
                                            │ Dispatch(tool, args)
                                            ▼
                                 FMyikaMCPCommandRegistry  (existing)
                                            │
                                            ▼
                                 UMyikaSubsystem  (existing)
```

**Why dispatch direct to the registry** instead of routing through TCP? The registry is the same process. TCP framing exists to support out-of-process MCP clients; in-process there's no benefit and a lot of latency cost. The chat panel runs on the editor's game thread, so handlers' `RunOnGameThreadVoid` calls become near-no-ops (the AsyncTask resolves on the same thread it was called from).

**Tool schemas.** Anthropic's API needs JSON Schema for each tool's `input_schema`. The Python side already has them (FastMCP derives from type hints); the C++ side does not. Adding a `schema` field to `FMyikaMCPCommandRegistry::Register` is the cleanest place: each `MakeXxxHandler()` factory now also returns a `TSharedPtr<FJsonObject>` schema. Two sources of truth (Python + C++), but each is small (~10 entries) and they describe the same thing.

## Tool-use loop

Anthropic's tool-use protocol:

1. Send messages + tool definitions.
2. Response is one of:
   - `stop_reason: end_turn` — normal completion, append assistant message, return.
   - `stop_reason: tool_use` — response includes `tool_use` content blocks. For each: dispatch via registry, append a `tool_result` user message with the JSON output (or the error code+message), then re-send.
3. Loop until `end_turn` or until we hit a max-iterations cap (default 10).

Errors propagate as `tool_result` content with `is_error: true` so the model can recover (e.g. retry `myika_set_weather` with a corrected enum string after `INVALID_ARGS`).

## Threading

- HTTP requests are async via `FHttpRequest::OnProcessRequestComplete` — game thread is never blocked.
- Tool dispatch happens on the game thread (same thread Slate runs on).
- The chat session is a `TSharedPtr` owned by the panel; cancelled when the tab closes.

## Settings (`UMyikaMCPChatSettings`)

| Field | Default | Notes |
|---|---|---|
| `Model` | `claude-opus-4-7` | Latest at time of writing |
| `MaxTokens` | `4096` | Per-response cap |
| `MaxToolIterations` | `10` | Safety net against infinite loops |
| `SystemPrompt` | (Myika-specific intro) | See below |
| (no API key field) | — | Read from `ANTHROPIC_API_KEY` env at runtime |

System prompt encodes the same heuristics as the cursor rule (preconditions, units, weather enum, Phase 2 stub guidance). Single source of truth: parse the cursor rule's body at startup, OR duplicate. For MVP we duplicate (keeps the chat module from depending on file IO).

## Risks

1. **API key leak.** Mitigation: env var only, never written to ini, never logged. Status UI says "set" / "unset", never the key value.
2. **Runaway tool calls.** Mitigation: `MaxToolIterations` hard cap, and the panel surfaces each tool call to the user so they can hit Stop.
3. **Game-thread stalls during tool dispatch.** Mitigation: each handler is fast (subsystem mutator). If a future Phase 2 tool is slow, dispatch needs to move to AsyncTask + future. Document.
4. **Schema drift between Python and C++.** Mitigation: a Python-side test that connects to the C++ TCP listener, calls `list_commands` (a meta-tool we should add later), and diffs against the FastMCP-registered names. Out of MVP.

## Open questions

- Should the panel survive across `/clear`-style operations on the agent? For MVP: clears together with the tab.
- Should we ship a "presets" dropdown (winter scene, golden hour, apocalypse) on the panel? Tempting but feature creep; keep MVP clean.
- Markdown rendering — VibeUE has a markdown→RichText converter. We can adopt theirs as a pattern when we re-do output rendering, but plaintext ships now.
