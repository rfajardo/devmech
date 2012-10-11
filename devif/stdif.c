/*
 * devif.c
 *
 *  Created on: May 16, 2011
 *      Author: rfajardo
 */

#include "regdata.h"

#include "getrules.h"
#include "setrules.h"

#include "adapt.h"
#include "regop.h"
#include "modify.h"
#include "comif.h"
#include "com.h"

#include "devif.h"
#include "helperif.h"

#include "devifdebug.h"


//
//	Standard interface
//
unsigned int getReg(Device * dev, Reg * reg)
{
	enum com_error comerror;

	if ( !isActive(reg) )
	{
		DEVIF_WARNING("This register is currently inactive. Its alternateGroup or registerField "
				"must be activated to use it.\n");
		setTransferError(NOTACTIVE);
		return RETURNERROR;
	}

	if ( !getRegAccess(reg) )
	{
		DEVIF_DEBUG_IF("Read access to register denied.\n");
		setTransferError(ACCESS_DENIED);
		return RETURNERROR;
	}

	if ( !getRegNeedTransfer(reg) )
	{
		DEVIF_INFO("Transfer not required to acquire register value, cached value returned.\n");
		return readReg(reg);
	}

	if ( !getRegAllowedContent(reg) )
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

	if ( getRegNeedModifyContent(reg) )
	{
		DEVIF_DEBUG_IF("Modifying register content to cope with read actions.\n");
		modifyValueRegister(reg, READ, 0);	/*readAction is never dependent on earlier value*/
											/*oneTo... or zeroTo... types don't exist*/
	}

	return readReg(reg);
}
EXPORT_SYMBOL_GPL(getReg);

unsigned int getField(Device * dev, RegField * regField)
{
	enum com_error comerror;

	if ( !isActive(regField->parent_register) )
	{
		DEVIF_WARNING("The parent register of this field is currently inactive. The group or registerField "
				"of its parent register must be activated to use this field.\n");
		setTransferError(NOTACTIVE);
		return RETURNERROR;
	}

	if ( !getFieldAccess(regField) )
	{
		DEVIF_DEBUG_IF("Read access to field denied.\n");
		setTransferError(ACCESS_DENIED);
		return RETURNERROR;
	}

	if ( !getFieldNeedTransfer(regField) )
	{
		DEVIF_INFO("Transfer not required to acquire register value, cached value returned.\n");
		return getCurrentFieldValue(regField);
	}

	if ( !getFieldAllowedContent(regField) )
	{
		DEVIF_DEBUG_IF("Bad fields on parent register, content not allowed.\n");
		setTransferError(CONTENT_UNALLOWED);
		return RETURNERROR;
	}

	recvReg(dev->handle, regField->parent_register);

	comerror = dev->handle->getComError();
	if ( comerror != COM_OK )
	{
		DEVIF_ERROR("Communication failed.\n");
		setTransferError(COMMUNICATION_ERROR);
		return RETURNERROR;
	}

	if ( getFieldNeedModifyContent(regField) )
	{
		DEVIF_DEBUG_IF("Modifying register content to cope with read actions.\n");
//		modifyValue(regField->field, regField->field->readAction, 0);
		//since fields have to always be synchronized, we have now to modify all read_action fields if register has been read
		modifyValueRegister(regField->parent_register, READ, 0);	/*readAction is never dependent on earlier value*/
																	/*oneTo... or zeroTo... types don't exist*/
	}

	return getCurrentFieldValue(regField);
}
EXPORT_SYMBOL_GPL(getField);



enum transfer_error setReg(Device * dev, Reg * reg, unsigned int value)
{
	enum com_error comerror;
	unsigned int previous_value;

	if ( !isActive(reg) )
	{
		DEVIF_WARNING("This register is currently inactive. Its alternateGroup or registerField "
				"must be activated to use it.\n");
		return NOTACTIVE;
	}

	if ( !setRegAccess(reg) )
	{
		DEVIF_DEBUG_IF("Write access to register denied.\n");
		return ACCESS_DENIED;
	}

	if ( !setRegNeedTransfer(reg) )
	{
		DEVIF_ERROR("No transfer required for setReg. By design, this should never happen.\n");
		return NO_TRANSFER;
	}

	if ( setRegNeedReadBeforeWrite(reg) )
	{
		DEVIF_WARNING("Some register's field(s) has to be read prior to writing to this register.\n");
		return NEEDGETSET;
	}

	previous_value = readReg(reg);
	writeReg(reg, value);

	if ( !setRegAllowedContent(reg) )
	{
		DEVIF_DEBUG_IF("Bad fields on register, content not allowed.\n");
		writeReg(reg, previous_value);
		return CONTENT_UNALLOWED;
	}

	comerror = sendReg(dev->handle, reg, value);
	if ( comerror != COM_OK )
	{
		DEVIF_ERROR("Communication failed.\n");
		writeReg(reg, previous_value);
		return COMMUNICATION_ERROR;
	}

	if ( setRegNeedModifyContent(reg) )
	{
		DEVIF_DEBUG_IF("Modifying register content to cope with modified write values.\n");
		modifyValueRegister(reg, WRITE, previous_value);
	}

	return TRANSFER_OK;
}
EXPORT_SYMBOL_GPL(setReg);

enum transfer_error setField(Device * dev, RegField * regField, unsigned int value)
{
	enum com_error comerror;
	unsigned int previous_field;
	unsigned int previous_reg;

	if ( !isActive(regField->parent_register) )
	{
		DEVIF_WARNING("The parent register of this field is currently inactive. The group or registerField "
				"of its parent register must be activated to use this field.\n");
		return NOTACTIVE;
	}

	if ( !setFieldAccess(regField) )
	{
		DEVIF_DEBUG_IF("Write access to field denied.\n");
		return ACCESS_DENIED;
	}

	if ( !setFieldNeedTransfer(regField) )
	{
		DEVIF_ERROR("No transfer required for setField. By design, this should never happen.\n");
		return NO_TRANSFER;
	}

	if ( setFieldNeedReadBeforeWrite(regField) )
	{
		DEVIF_WARNING("Some parent register's field(s) has to be read prior to writing to this field.\n");
		return NEEDGETSET;
	}

	previous_reg = readReg(regField->parent_register);
	previous_field = getCurrentFieldValue(regField);

	updateParentRegister(regField, value);

	if ( setFieldNeedAdaptContentBeforeWrite(regField) )
	{
		DEVIF_DEBUG_IF("Updating sibling fields of this field in order not to produce any action or modify register values.\n");
		adaptContentSiblings(regField);		//what about value? value is only for this bitfield, this updates
													//other fields of register
	}

	if ( !setFieldAllowedContent(regField) )
	{
		DEVIF_DEBUG_IF("Bad fields on parent register, content not allowed.\n");
		writeReg(regField->parent_register, previous_reg);
		return CONTENT_UNALLOWED;
	}

	comerror = sendReg(dev->handle, regField->parent_register, readReg(regField->parent_register));
	if ( comerror != COM_OK )
	{
		DEVIF_ERROR("Communication failed.\n");
		writeReg(regField->parent_register, previous_reg);
		return COMMUNICATION_ERROR;
	}

	if ( setFieldNeedModifyContent(regField) )
	{
		DEVIF_DEBUG_IF("Modifying field content to cope with its modified write value.\n");
		modifyValue(regField, regField->field->modifiedWriteValue, previous_field);
	}

	return TRANSFER_OK;
}
EXPORT_SYMBOL_GPL(setField);



enum transfer_error getSetReg(Device * dev, Reg * reg, unsigned int value)
{
	enum com_error comerror;
	enum transfer_error error = TRANSFER_OK;

	unsigned int recv_value;

	recv_value = getReg(dev, reg);
	error = getTransferError();
	if ( error != TRANSFER_OK )
		return error;

	if ( !setRegAccess(reg) )
	{
		DEVIF_DEBUG_IF("Write access to register denied.\n");
		return ACCESS_DENIED;
	}

	if ( !setRegNeedTransfer(reg) )
	{
		DEVIF_ERROR("No transfer required for getSetReg. By design, this should never happen.\n");
		return NO_TRANSFER;
	}

	writeReg(reg, value);

	if ( setRegNeedReadBeforeWrite(reg) )
	{
		DEVIF_DEBUG_IF("Adapting register's writeAsRead fields with read values.\n");
		//reg->value contains new data and recv_value is the old data
		adaptWriteAsReadFields(reg, recv_value);
	}
	else
		DEVIF_WARNING("It does not seem that getSetReg is required, you could try simply setReg instead.\n");

	if ( !setRegAllowedContent(reg) )
	{
		DEVIF_DEBUG_IF("Bad fields on register, content not allowed.\n");
		writeReg(reg, recv_value);
		return CONTENT_UNALLOWED;
	}

	comerror = sendReg(dev->handle, reg, value);
	if ( comerror != COM_OK )
	{
		DEVIF_ERROR("Communication failed.\n");
		writeReg(reg, recv_value);
		return COMMUNICATION_ERROR;
	}

	if ( setRegNeedModifyContent(reg) )
	{
		DEVIF_DEBUG_IF("Modifying register content to cope with modified write values.\n");
		modifyValueRegister(reg, WRITE, recv_value);
	}

	return TRANSFER_OK;
}
EXPORT_SYMBOL_GPL(getSetReg);

enum transfer_error getSetField(Device * dev, RegField * regField, unsigned int value)
{
	enum com_error comerror;
	enum transfer_error error = TRANSFER_OK;

	unsigned int recv_value;
	unsigned int previous_field;

	recv_value = getReg(dev, regField->parent_register);
	error = getTransferError();
	if ( error != TRANSFER_OK )
		return error;

	if ( !setFieldAccess(regField) )
	{
		DEVIF_DEBUG_IF("Write access to field denied.\n");
		return ACCESS_DENIED;
	}

	if ( !setFieldNeedTransfer(regField) )
	{
		DEVIF_ERROR("No transfer required for getSetField. By design, this should never happen.\n");
		return NO_TRANSFER;
	}

	previous_field = getCurrentFieldValue(regField);
	updateParentRegister(regField, value);

	if ( setFieldNeedReadBeforeWrite(regField) )
	{
		DEVIF_DEBUG_IF("Adapting field's writeAsRead siblings with read values.\n");
		//regField->parent_register->value contains new data and recv_value is the old data
		adaptWriteAsReadSiblings(regField, recv_value);
	}
	else
		DEVIF_WARNING("It does not seem that getSetField is required, you could try simply setField instead.\n");

	if ( setFieldNeedAdaptContentBeforeWrite(regField) )
	{
		DEVIF_DEBUG_IF("Adapting modified write value and strobe write siblings of this field not to change the parent register.\n");
		adaptContentSiblings(regField);		//what about value? value is only for this bitfield, this updates
													//other fields of register
	}

	if ( !setFieldAllowedContent(regField) )
	{
		DEVIF_DEBUG_IF("Bad fields on parent register, content not allowed.\n");
		writeReg(regField->parent_register, recv_value);
		return CONTENT_UNALLOWED;
	}

	comerror = sendReg(dev->handle, regField->parent_register, readReg(regField->parent_register));
	if ( comerror != COM_OK )
	{
		DEVIF_ERROR("Communication failed.\n");
		writeReg(regField->parent_register, recv_value);
		return COMMUNICATION_ERROR;
	}

	if ( setFieldNeedModifyContent(regField) )
	{
		DEVIF_DEBUG_IF("Modifying field content to cope with its modified write value.\n");
		modifyValue(regField, regField->field->modifiedWriteValue, previous_field);
	}

	return TRANSFER_OK;
}
EXPORT_SYMBOL_GPL(getSetField);



unsigned int getWideReg(Device * dev, Reg * reg, unsigned int index)
{
	enum com_error comerror;

	if ( !isActive(reg) )
	{
		DEVIF_WARNING("This register is currently inactive. Its alternateGroup or registerField "
				"must be activated to use it.\n");
		setTransferError(NOTACTIVE);
		return RETURNERROR;
	}

	if ( reg->dim == 1 )
	{
		DEVIF_WARNING("This register does not seem to be wide. Using getWideReg avoids rule checks you don't want to miss. Exiting.\n");
		setTransferError(NOTWIDE);
		return RETURNERROR;
	}

	if ( index >= reg->dim )
	{
		DEVIF_WARNING("The input index is beyond the dimension of this register. Exiting. \n");
		setTransferError(OUTBOUNDS);
		return RETURNERROR;
	}

	if ( !getRegAccess(reg) )
	{
		DEVIF_DEBUG_IF("Read access to register denied.\n");
		setTransferError(ACCESS_DENIED);
		return RETURNERROR;
	}

	if ( !getRegNeedTransfer(reg) )
	{
		DEVIF_INFO("Transfer not required to acquire register value, cached value returned.\n");
		return readWideReg(reg, index);
	}

	recvWideReg(dev->handle, reg, index);

	comerror = dev->handle->getComError();
	if ( comerror != COM_OK )
	{
		DEVIF_ERROR("Communication failed.\n");
		setTransferError(COMMUNICATION_ERROR);
		return RETURNERROR;
	}

	return readWideReg(reg, index);
}
EXPORT_SYMBOL_GPL(getWideReg);

enum transfer_error setWideReg(Device * dev, Reg * reg, unsigned int index, unsigned int value)
{
	enum com_error comerror;
	unsigned int previous_value;

	if ( !isActive(reg) )
	{
		DEVIF_WARNING("This register is currently inactive. Its alternateGroup or registerField "
				"must be activated to use it.\n");
		return NOTACTIVE;
	}

	if ( reg->dim == 1 )
	{
		DEVIF_WARNING("This register does not seem to be wide. Using setWideReg avoids rule checks you don't want to miss. Exiting.\n");
		return NOTWIDE;
	}

	if ( index >= reg->dim )
	{
		DEVIF_WARNING("The input index is beyond the dimension of this register. Exiting. \n");
		return OUTBOUNDS;
	}

	if ( !setRegAccess(reg) )
	{
		DEVIF_DEBUG_IF("Write access to register denied.\n");
		return ACCESS_DENIED;
	}

	if ( !setRegNeedTransfer(reg) )
	{
		DEVIF_ERROR("No transfer required for setReg. By design, this should never happen.\n");
		return NO_TRANSFER;
	}

	previous_value = readWideReg(reg, index);
	writeWideReg(reg, index, value);

	comerror = sendWideReg(dev->handle, reg, index, value);
	if ( comerror != COM_OK )
	{
		DEVIF_ERROR("Communication failed.\n");
		writeWideReg(reg, index, previous_value);
		return COMMUNICATION_ERROR;
	}

	return TRANSFER_OK;
}
EXPORT_SYMBOL_GPL(setWideReg);
