#pragma once
#include "VsMessage.h"

class VsKeybdMessage : public VsMessage
{
private:
	BYTE shiftFlags;
	BYTE keyCodes[6];
	INT imeKey;
public:
	VsKeybdMessage(void);
	~VsKeybdMessage(void);
	BYTE GetShiftFlags(void);
	BYTE *GetKeyCodes(void);
	INT GetImeKey(void);
	INT Parse(CHAR*) override;
};

