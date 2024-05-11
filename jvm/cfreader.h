#ifndef _CLASSFILE_READER_H_
#define _CLASSFILE_READER_H_

#include <stdint.h>
#include "consts.h"
#include <stdio.h>
#include "fm.h"

struct Attribute;
typedef struct Code {
	uint16_t max_stack;
	uint16_t max_locals;
	uint32_t ins_length;
	uint8_t* ins;
	uint16_t exception_table_len;
	struct {
		uint16_t start_pc;
		uint16_t end_pc;
		uint16_t handler_pc;
		uint16_t type;
	} *exception_table;
	uint16_t attributes_count;
	struct Attribute* attrs;
} Code;

typedef struct Attribute {
	const char* name;
	uint32_t length;
	union { // We only support Code & ConstantValue right now
		Code code;
	        uint16_t const_value;
		uint16_t source;
		struct {
			uint16_t num_bt_methods;
			struct bootstrap {
				uint16_t ref;
				uint16_t nargs;
				uint16_t* args;
			} *bt_methods;
		};
		struct {
			uint16_t num_exceptions;
			uint16_t* exc_table;
		};
	};
	struct Attribute (*FindAttribute) (struct Attribute*, int n, const char*);
} Attribute;

struct cpelem {
	uint8_t tag;
	union {
		uint16_t index; // Represents any of : CONSTANT_Class, CONSTANT_String, CONSTANT_MethodType
		struct {
			uint16_t cl_index;
			uint16_t nt_index;
		}; // Represents any of : CONSTANT_[Field|Method|Interface|NameAndType|InvokeDynamic] 
		int32_t int32; // From here everything is obvious from name
		float flt32;
		int64_t int64;
		double flt64;
		struct {
			uint16_t len;
			uint8_t* bytes;
		};
		struct {
			uint8_t ref_kind;
			uint16_t ref;
		};
	};
};

typedef struct ConstantPool {
	uint16_t size;
	struct cpelem *elements;
	int (*IsOfType) (struct ConstantPool*, uint8_t, uint16_t);
	struct cpelem (*Find) (struct ConstantPool*, uint16_t);
	uint8_t* (*GetString) (struct ConstantPool*, uint16_t);
} ConstantPool;

typedef struct FM { // May be field or method
	uint16_t flags;
	uint16_t name_idx;
	uint16_t desc_idx;
	uint16_t attribute_count;
	uint8_t deprecated;
	uint8_t checked; // Only for methods
	Attribute* attributes;
	struct FM* next;
} FM;

typedef struct ClassFile {
	uint16_t major;
	ConstantPool* cp;
	uint16_t flags;
	const char* this_class;
	uint16_t super_class;
	uint16_t interfaces_count;
	uint16_t* interfaces;
	uint16_t fields_count;
	FM* field;
	uint16_t method_count;
	FM* methods;
	uint16_t attributes_count;
	Attribute* attributes;
	FM* (*GetMethodOrField) (struct ClassFile*, const char*, const char*);
} ClassFile;

ClassFile* LoadClass(FileHandle*);
#endif

