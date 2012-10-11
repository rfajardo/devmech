/*
 * userif.c
 *
 *  Created on: May 30, 2011
 *      Author: rfajardo
 */

#include "regdata.h"

#include "getrules.h"

#include "regop.h"
#include "modify.h"
#include "comif.h"
#include "com.h"

#include "devif.h"
#include "helperif.h"

#include "devifdebug.h"

//
//	User controlled get interface (force)
//
unsigned int cachedGetReg(Reg * reg)
{
	if ( !isActive(reg) )
	{
		DEVIF_WARNING("This register is currently inactive. Its alternateGroup or registerField "
				"must be activated to use it.\n");
		setTransferError(NOTACTIVE);
		return RETURNERROR;
	}

	if ( !getRegAccess(reg) )
	{
		DEVIF_WARNING("User control get interface: Rule surpassed, read access to register would be denied.\n");
	}

	if ( getRegNeedTransfer(reg) )
	{
		DEVIF_WARNING("User control get interface: Rule surpassed, a transfer seems to have been necessary.\n");
	}

	return readReg(reg);
}
EXPORT_SYMBOL_GPL(cachedGetReg);

unsigned int cachedGetField(RegField * regField)
{
	if ( !isActive(regField->parent_register) )
	{
		DEVIF_WARNING("The parent register of this field is currently inactive. The group or registerField "
				"of its parent register must be activated to use this field.\n");
		setTransferError(NOTACTIVE);
		return RETURNERROR;
	}

	if ( !getFieldAccess(regField) )
	{
		DEVIF_WARNING("User control get interface: Rule surpassed, read access to field would be denied.\n");
	}

	if ( getFieldNeedTransfer(regField) )
	{
		DEVIF_WARNING("User control get interface: Rule surpassed, a transfer seems to have been necessary.\n");
	}

	return getCurrentFieldValue(regField);
}
EXPORT_SYMBOL_GPL(cachedGetField);

unsigned int forceGetReg(Device * dev, Reg * reg)
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
		DEVIF_WARNING("User control get interface: Rule surpassed, read access to register would be denied.\n");
	}

	if ( !getRegNeedTransfer(reg) )
	{
		DEVIF_WARNING("User control get interface: Rule surpassed, a transfer does not seem to be necessary.\n");
	}

	if ( !getRegAllowedContent(reg) )
	{
		DEVIF_WARNING("User control get interface: Rule surpassed, a transfer reading from this register might "
				"trigger actions on your device or you are overwriting data prepared for the delayed interface.\n");
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
EXPORT_SYMBOL_GPL(forceGetReg);

unsigned int forceGetField(Device * dev, RegField * regField)
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
		DEVIF_WARNING("User control get interface: Rule surpassed, read access to field would be denied.\n");
	}

	if ( !getFieldNeedTransfer(regField) )
	{
		DEVIF_WARNING("User control get interface: Rule surpassed, a transfer does not seem to be necessary.\n");
	}

	if ( !getFieldAllowedContent(regField) )
	{
		DEVIF_WARNING("User control get interface: Rule surpassed, a transfer reading from this register's field might "
				"trigger actions on your device or you are overwriting data prepared for the delayed interface.\n");
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
EXPORT_SYMBOL_GPL(forceGetField);


unsigned int cachedGetWideReg(Reg * reg, unsigned int index)
{
	if ( !isActive(reg) )
	{
		DEVIF_WARNING("This register is currently inactive. Its alternateGroup or registerField "
				"must be activated to use it.\n");
		setTransferError(NOTACTIVE);
		return RETURNERROR;
	}

	if ( reg->dim == 1 )
	{
		DEVIF_WARNING("This register does not seem to be wide. Using cachedGetWideReg avoids data updates "
				"you don't want to miss. Use the cachedGetReg instead. Exiting.\n");
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
		DEVIF_WARNING("User control get interface: Rule surpassed, read access to register would be denied.\n");
	}

	if ( getRegNeedTransfer(reg) )
	{
		DEVIF_WARNING("User control get interface: Rule surpassed, a transfer seems to have been necessary.\n");
	}

	return readWideReg(reg, index);
}
EXPORT_SYMBOL_GPL(cachedGetWideReg);

unsigned int forceGetWideReg(Device * dev, Reg * reg, unsigned int index)
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
		DEVIF_WARNING("This register does not seem to be wide. Using forceGetWideReg avoids data updates "
				"you don't want to miss. Use the forceGetReg instead. Exiting.\n");
		return NOTWIDE;
	}

	if ( index >= reg->dim )
	{
		DEVIF_WARNING("The input index is beyond the dimension of this register. Exiting. \n");
		return OUTBOUNDS;
	}

	if ( !getRegAccess(reg) )
	{
		DEVIF_WARNING("User control get interface: Rule surpassed, read access to register would be denied.\n");
	}

	if ( !getRegNeedTransfer(reg) )
	{
		DEVIF_WARNING("User control get interface: Rule surpassed, a transfer does not seem to be necessary.\n");
	}

	if ( !getRegAllowedContent(reg) )
	{
		DEVIF_WARNING("User control get interface: Rule surpassed, a transfer reading from this register might "
				"trigger actions on your device or you are overwriting data prepared for the delayed interface.\n");
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
EXPORT_SYMBOL_GPL(forceGetWideReg);
