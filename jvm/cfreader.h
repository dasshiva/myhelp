#ifndef _CLASSFILE_READER_H_
#define _CLASSFILE_READER_H_

#include <stdint.h>
#include "consts.h"
#include <stdio.h>
#include "fm.h"

typedef struct Attribute {
	uint16_t name;
	uint32_t length;
	union {
	} attrs;
	struct Attribute* next; 
} Attribute;

typedef struct ConstantPool {
	struct cpelem {
		uint8_t tag;
		union {
			uint16_t index; // Represents any of : CONSTANT_Class, CONSTANT_String, CONSTANT_MethodType
			struct {
				uint16_t cl_index;
				uint16_t nt_index;
			} FMI_NT_INDY_ref; // Represents any of : CONSTANT_[Field|Method|Interface|NameAndType|InvokeDynamic] 
			int32_t int32; // From here everything is obvious from name
			float flt32;
			int64_t int64;
			double flt64;
			struct {
				uint16_t len;
				uint8_t* bytes;
			} string;
			struct {
				uint8_t ref_kind;
				uint16_t ref;
			} mhandle;
		} elem;
	} **elements;
	struct cpelem* (*Find) (struct ConstantPool*, uint16_t);
} ConstantPool;

typedef struct FM { // May be field or method
	uint16_t flags;
	uint16_t name_idx;
	uint16_t desc_idx;
	uint16_t attribute_count;
	Attribute* attributes;
	struct FM* next;
} FM;

typedef struct ClassFile {
	uint64_t major;
	uint16_t cp_size;
	ConstantPool* cp;
	uint16_t flags;
	uint16_t this_class;
	uint16_t super_class;
	uint16_t interfaces_count;
	uint16_t* interfaces;
	uint16_t fields_count;
	FM* field;
	uint16_t method_count;
	FM* methods;
	uint16_t attributes_count;
	Attribute* attributes;
} ClassFile;

ClassFile* LoadClass(FileHandle*);
#endif

