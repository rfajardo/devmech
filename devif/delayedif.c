/*
 * delayedif.c
 *
 *  Created on: May 27, 2011
 *      Author: rfajardo
 */

#include "regdata.h"

#include "delayedlist.h"

#include "delayedsetrules.h"
#include "delayedgetrules.h"

#include "adapt.h"
#include "regop.h"
#include "modify.h"
#include "comif.h"
#include "com.h"

#include "devif.h"
#include "helperif.h"

#include "devifdebug.h"



static unsigned int delayedGetReg(Device * dev, Reg * reg)
{
	enum com_error comerror;

	if ( !isActive(reg) )
	{
		DEVIF_WARNING("This register is currently inactive. Its alternateGroup or registerField "
				"must be activated to use it.\n");
		setTransferError(NOTACTIVE);
		return RETURNERROR;
	}

	if ( !delayedGetRegAccess(reg) )
	{
		setTransferError(ACCESS_DENIED);
		DEVIF_DEBUG_IF("Read access to register denied.\n");
		return RETURNERROR;
	}

	if ( !delayedGetRegNeedTransfer(reg) )
	{
		DEVIF_INFO("Transfer not required to acquire register value, cached value returned.\n");
		return readReg(reg);
	}

	if ( !delayedGetRegAllowedContent(reg) )
	{
		DEVIF_DEBUG_IF("Bad fields on register, content not allowed.\n");
		setTransferError(CONTENT_UNALLOWED);
		return RETURNERROR;
	}

	recvReg(dev->handle, reg);

	comerror = dev->handle->getComError();
	if ( comerror != COM_OK )
	{
		DEVIF_ERROR("Communication failed.\n");
		setTransferError(COMMUNICATION_ERROR);
		return RETURNERROR;
	}

	if ( delayedGetRegNeedModifyContent(reg) )
	{
		DEVIF_DEBUG_IF("Modifying register content to cope with read actions.\n");
		modifyValueRegister(reg, READ, 0);			/*readAction is never dependent on earlier value*/
													/*oneTo... or zeroTo... types don't exist*/
	}

	return readReg(reg);
}


//
//	Delayed set interface for writing multiple fields in one transaction
//
enum transfer_error delayedSetReg(Device * dev, Reg * reg)
{
	enum com_error comerror;
	unsigned int previous_value;

	if ( !isActive(reg) )
	{
		DEVIF_WARNING("This register is currently inactive. Its alternateGroup or registerField "
				"must be activated to use it.\n");
		return NOTACTIVE;
	}

	if ( !delayedSetRegAccess(reg) )
	{
		DEVIF_DEBUG_IF("Write access to register denied.\n");
		return ACCESS_DENIED;
	}

	if ( !delayedSetRegNeedTransfer(reg) )
	{
		DEVIF_WARNING("No transfer required for delayedSetReg. No fields have been previously delayedSet.\n");
		return NO_TRANSFER;
	}

	if ( delayedSetRegNeedReadBeforeWrite(reg) )
	{
		DEVIF_WARNING("Some register's field(s) has to be read prior to writing to this register.\n");
		return NEEDGETSET;
	}

	previous_value = readReg(reg);
	updateDirtiesParentRegister(reg);			//update register with values of dirty fields

	if ( delayedSetRegNeedAdaptContentBeforeWrite(reg) )
	{
		DEVIF_DEBUG_IF("Updating non dirty fields of this register in order not to produce any action or modify register values.\n");
		adaptContentFieldList(reg->cleanFields);
	}

	if ( !delayedSetRegAllowedContent(reg) )
	{
		DEVIF_DEBUG_IF("Bad fields on register, content not allowed.\n");
		writeReg(reg, previous_value);
		return CONTENT_UNALLOWED;
	}

	comerror = sendReg(dev->handle, reg, readReg(reg));
	if ( comerror != COM_OK )
	{
		DEVIF_ERROR("Communication failed.\n");
		writeReg(reg, previous_value);
		return COMMUNICATION_ERROR;
	}

	if ( delayedSetRegNeedModifyContent(reg) )
	{
		DEVIF_DEBUG_IF("Modifying register content to cope with modified write values.\n");
		modifyValueRegister(reg, WRITE, previous_value);
	}

	clearDirtyFields(reg);

	return TRANSFER_OK;
}
EXPORT_SYMBOL_GPL(delayedSetReg);

enum transfer_error delayedSetField(RegField * regField, unsigned int value)
{
	if ( !isActive(regField->parent_register) )
	{
		DEVIF_WARNING("The parent register of this field is currently inactive. The group or registerField "
				"of its parent register must be activated to use this field.\n");
		return NOTACTIVE;
	}

	if ( !delayedSetFieldAccess(regField) )
	{
		DEVIF_DEBUG_IF("Write access to field denied.\n");
		return ACCESS_DENIED;
	}

	if ( delayedSetFieldNeedTransfer(regField) )
	{
		DEVIF_ERROR("Transfers should, by design, never be required for delayedSetField.\n");
		return TRANSFER;
	}

	setDirty(regField, value); 			//put to dirty list

	if ( !delayedSetFieldAllowedContent(regField) )
	{
		DEVIF_DEBUG_IF("Bad fields on parent register, content not allowed.\n");
		clearDirty(regField); 		//remove from dirty list
		return CONTENT_UNALLOWED;
	}

	return TRANSFER_OK;
}
EXPORT_SYMBOL_GPL(delayedSetField);



enum transfer_error delayedSetFieldResetValue(RegField * regField)
{
	unsigned int reset_value;

	if ( !regField->parent_register->reset.valid )
	{
		DEVIF_WARNING("Register does not have a reset value assigned. This failed, exiting.\n");
		return NORESET;
	}

	reset_value = genResetFieldValue(regField);

	return delayedSetField(regField, reset_value);
}
EXPORT_SYMBOL_GPL(delayedSetFieldResetValue);

void cancelDelayedTransfers(Reg * reg)
{
	clearDirtyFields(reg);
}
EXPORT_SYMBOL_GPL(cancelDelayedTransfers);



enum transfer_error delayedGetSetReg(Device * dev, Reg * reg)
{
	enum com_error comerror;
	enum transfer_error error = TRANSFER_OK;

	unsigned int recv_value;

	recv_value = delayedGetReg(dev, reg);
	error = getTransferError();
	if ( error != TRANSFER_OK )
		return error;

	if ( !delayedSetRegAccess(reg) )
	{
		DEVIF_DEBUG_IF("Write access to register denied.\n");
		writeReg(reg, recv_value);
		return ACCESS_DENIED;
	}

	if ( !delayedSetRegNeedTransfer(reg) )
	{
		DEVIF_ERROR("No transfer required for delayedGetSetReg. By design, this should never happen.\n");
		writeReg(reg, recv_value);
		return NO_TRANSFER;
	}

	updateDirtiesParentRegister(reg);			//update register with values of dirty fields

	if ( delayedSetRegNeedReadBeforeWrite(reg) )
	{
		DEVIF_DEBUG_IF("Adapting non dirty writeAsRead fields of register with read values.\n");
		adaptWriteAsReadFieldList(reg->cleanFields, recv_value);
	}
	else
		DEVIF_WARNING("It does not seem that delayedGetSetReg is required, you could try simply delayedSetReg instead.\n");

	if ( delayedSetRegNeedAdaptContentBeforeWrite(reg) )
	{
		DEVIF_DEBUG_IF("Adapting modified write value and strobe write non-dirty fields of this register not to change it.\n");
		adaptContentFieldList(reg->cleanFields);
	}

	if ( !delayedSetRegAllowedContent(reg) )
	{
		DEVIF_DEBUG_IF("Bad fields on register, content not allowed.\n");
		writeReg(reg, recv_value);
		return CONTENT_UNALLOWED;
	}

	comerror = sendReg(dev->handle, reg, readReg(reg));
	if ( comerror != COM_OK )
	{
		DEVIF_ERROR("Communication failed.\n");
		writeReg(reg, recv_value);
		return COMMUNICATION_ERROR;
	}

	if ( delayedSetRegNeedModifyContent(reg) )
	{
		DEVIF_DEBUG_IF("Modifying register content to cope with modified write values.\n");
		modifyValueRegister(reg, WRITE, recv_value);
	}

	clearDirtyFields(reg);

	return TRANSFER_OK;
}
EXPORT_SYMBOL_GPL(delayedGetSetReg);
