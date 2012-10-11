/*
 * delayedgetrules.c
 *
 *  Created on: May 18, 2011
 *      Author: rfajardo
 */

#include "delayedgetrules.h"
#include "getrules.h"
#include "regdata.h"
#include "quantify.h"

#include "devifdebug.h"

bool delayedGetRegAccess(Reg * reg)
{
	return getRegAccess(reg);
}

bool delayedGetRegNeedTransfer(Reg * reg)
{
	return getRegNeedTransfer(reg);
}

bool delayedGetRegAllowedContent(Reg * reg)
{
	return getRegAllowedContent(reg);
}

bool delayedGetRegNeedModifyContent(Reg * reg)
{
	return getRegNeedModifyContent(reg);
}
