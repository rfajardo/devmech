
#ifndef REGDATA_H
#define REGDATA_H

#ifdef DEVIF
#include "regtypes.h"
#else
#include <devif/regtypes.h>
#endif


enum access_type
{
	read_write = 0,
	read_only,
	write_only,
	writeOnce,
	read_writeOnce
};

enum modifiedWriteValue_type
{
	none = 0,
	clear,
	set,
	modify,
	oneToClear,
	oneToSet,
	oneToToggle,
	zeroToClear,
	zeroToSet,
	zeroToToggle
};

struct writeValueConstraint_type
{
	bool valid;
	bool writeAsRead;
	bool useEnumeratedValues;
	unsigned int minimum;
	unsigned int maximum;
};

typedef enum modifiedWriteValue_type readAction_type;		//only uses first 4 enumerations

struct strobe_type
{
    bool read;
    bool write;
    unsigned int noAction;
};

struct vendorExtensions_type
{
	struct strobe_type strobe;
};

struct reset_type
{
	bool valid;
	unsigned int value;
	unsigned int mask;
};


struct enum_list_data
{
	unsigned int value;
};

struct active_list_data
{
	bool * active;
};

struct field_list_data
{
	RegField * regField;
	unsigned int cachedValue;
};


struct mem_type
{
	bool once_written;
	bool once_read;
	unsigned int value;
	unsigned int if_addr;
};


struct field_type
{
	bool dirty;							//only used to calculate number of dirty fields
										//among all fields of a register (delayedsetrules.c + quantify.c)
	unsigned int bitOffset;
	unsigned int bitWidth;
	bool is_volatile;
	enum access_type access;
	enum modifiedWriteValue_type modifiedWriteValue;
	struct writeValueConstraint_type writeValueConstraint;
	readAction_type readAction;
	bool testable;
	struct vendorExtensions_type vendorExtensions;

	List * enumeratedValues;
};


struct reg
{
	List * regFilesActive;
	List * altGroupsActive;

	unsigned int dim;
	unsigned int addressOffset;
	unsigned int size;

	bool is_volatile;
	enum access_type access;
	struct reset_type reset;

	List * allFields;		//list of all fields

	//all fields divided between dirty and clean fields
	List * dirtyFields;
	List * cleanFields;

	struct vendorExtensions_type vendorExtensions;

	Mem * pcache;

	void * if_complex_data;
};


struct p_reg_field
{
	Field * field;
	Reg * parent_register;
};


Reg * alloc_register(void);
void free_register(Reg * reg);				//also frees all internal fields
											//does not free associated memory

void add_register_file_active(Reg * reg, bool * register_file_active);		//register_file_active has to be allocated
void add_alternate_group_active(Reg * reg, bool * alternate_group_active);	//alternate_group_active has to be allocated

Field * alloc_field(void);
void add_field(Reg * reg, Field * field);	//field has to be allocated
void add_enumvalue(Field * field, unsigned int enumvalue);

RegField * alloc_regfield(void);
void associate_regfield(RegField * regField, Reg * reg, Field * field);
void free_regfield(RegField * regField);

Mem * alloc_memory(unsigned int size);
void associate_memory(Reg * reg, Mem * pmem);
void free_memory(Mem * pmem);



#endif //REGDATA_H
