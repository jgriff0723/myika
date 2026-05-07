// Copyright (c) Myika AI. All rights reserved.

#include "MyikaChatTypes.h"

FMyikaChatMessage FMyikaChatMessage::UserText(const FString& Text)
{
	FMyikaChatMessage M;
	M.Role = TEXT("user");
	FMyikaChatContentBlock B;
	B.Kind = FMyikaChatContentBlock::EKind::Text;
	B.Text = Text;
	M.Content.Add(MoveTemp(B));
	return M;
}
