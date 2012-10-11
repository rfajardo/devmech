/*
 * regtypes.h
 *
 *  Created on: May 19, 2011
 *      Author: rfajardo
 */

#ifndef REGTYPES_H_
#define REGTYPES_H_

#ifdef DEVIF
#include "types.h"
#include "listutil.h"
#else
#include <devif/types.h>
#include <devif/listutil.h>
#endif


//scope macros
#define _GETNUM(scope, cte) scope ## cte
#define GETNUM(scope, cte) _GETNUM(scope, cte)
#define SCOPENUM(cte) GETNUM(SCOPE, cte)

#define SCOPE foo


//regtype definitions
struct reg;
struct field_type;
struct p_reg_field;

struct active_list_data;
struct enum_list_data;
struct field_list_data;


struct mem_type;


typedef struct reg Reg;
typedef struct field_type Field;
typedef struct p_reg_field RegField;
typedef struct reg Altreg;

typedef struct active_list_data ActiveListData;
typedef struct enum_list_data EnumListData;
typedef struct field_list_data FieldListData;

typedef struct mem_type Mem;


//specific definitions
enum modifiedWriteValue_type;

//communication definition
struct devcom;
typedef struct devcom DevCom;

//device definition
#if defined(DEVIF) || defined(USBIF)
typedef struct dev Device;
#else
typedef void Device;
#endif


#endif /* REGTYPES_H_ */
