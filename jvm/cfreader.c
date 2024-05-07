#include "cfreader.h"
#include "log.h"

#define u1() get_u1(buf, &offset)
#define u2() get_u2(buf, &offset)
#define u4() get_u4(buf, &offset)
#define cp() cf->cp->elements[i]
inline static uint8_t get_u1(uint8_t* buf, uint64_t* offset) {
	*offset += 1;
	return buf[*offset - 1];
}

inline static uint16_t get_u2(uint8_t* buf, uint64_t* offset) {
	uint8_t r1 = get_u1(buf, offset);
	uint8_t r2 = get_u1(buf, offset);
	return ((uint16_t)r1) << 8 | r2;
}

inline static uint32_t get_u4(uint8_t* buf, uint64_t* offset) {
        uint8_t r1 = get_u1(buf, offset);
        uint8_t r2 = get_u1(buf, offset);
	uint8_t r3 = get_u1(buf, offset);
	uint8_t r4 = get_u1(buf, offset);
        return r1 << 24 | r2 << 16 | r3 << 8 | r4;
}

struct cpelem Find (ConstantPool* self, uint16_t index) {
	if (index == 0 || index > self->size) {
		warn("Invalid access to index %d", index);
		return self->elements[0];
	}
	return self->elements[index - 1];
}

uint8_t* GetString(ConstantPool* self, uint16_t index) {
	struct cpelem elem = self->Find(self, index);
	if (elem.tag == JVM_CONSTANT_Utf8)
		return elem.bytes;
	else if (elem.tag == JVM_CONSTANT_Class || elem.tag == JVM_CONSTANT_String) 
		return GetString(self, elem.index);
	else {
		warn("Index %d is not a UTF-8 element", index);
		return NULL;
	}
}

int IsOfType(ConstantPool* self, uint8_t type, uint16_t index) {
	if (self->Find(self, index).tag != type) {
		warn("Index %d is not of type %d", index, type);
		return 0;
	}
	return 1;
}

#include <string.h>
FM* GetMethod(struct ClassFile* self, const char* name, const char* desc) {
	FM* method = self->methods;
	while (method) {
		const char* n = (const char*) self->cp->GetString(self->cp, method->name_idx);
		const char* d = (const char*) self->cp->GetString(self->cp, method->desc_idx);
		if (!strcmp(name, n) && !strcmp(desc, d)) 
			return method;
		method = method->next;
	}

	warn("Method %s:%s not found", name, desc);
	return NULL;
}

uint64_t parse_methods_or_fields(ClassFile* cf, FM** _target, uint16_t number, int method, uint64_t offset, uint8_t* buf) {
	FM* target = *_target;
	for (int i = 0; i < number; i++) {
		target->flags = u2();
		target->name_idx = u2();
		// info("%d", method->name_idx);
		target->desc_idx = u2();
		target->attribute_count = u2();
		target->attributes = malloc(sizeof(Attribute) * target->attribute_count);
		for (int j = 0; j < target->attribute_count; j++) {
			target->attributes[j].name = (uint8_t*) cf->cp->GetString(cf->cp, u2());
			if (!target->attributes[j].name) {
				warn("Attribute name accesses invalid index");
				return 0;
			}
			target->attributes[j].length = u4();
			if (!strcmp(target->attributes[j].name, "Code") && method) {
#define mt_code() target->attributes[j].code
				 mt_code().max_stack = u2();
				 mt_code().max_locals = u2();
				 mt_code().ins_length = u4();
				 mt_code().ins = calloc(1, mt_code().ins_length);
				 for (int k = 0; k < mt_code().ins_length; k++) {
					mt_code().ins[k] = u1();
				 }
				 mt_code().exception_table_len = u2();
				 mt_code().attributes_count = u2();
				if (mt_code().exception_table_len != 0 || mt_code().attributes_count != 0) {
					warn("Exceptions and code attributes are not implemented yet");
					return 0;
				}
#undef mt_code
			}
			else if (!strcmp(target->attributes[j].name, "ConstantValue") && !method) {
				target->attributes[j].const_value = u2();
			}
			else {
				warn("Skipping unknown or unexpected attribute %s", target->attributes[j].name);
				offset += target->attributes[j].length;
			}
		}
	       	if (i + 1 != number) {
			target->next = malloc(sizeof(FM));
			target = target->next;
		}	
		else target->next = NULL;
	}
	return offset;
}

ClassFile* LoadClass(FileHandle* fh) {
	uint64_t offset = 0;
	uint64_t len = fh->Length(fh);
	uint8_t* buf = malloc(len);

	fh->Read(fh, buf, len);
	GetSystemFM()->Close(fh);
	if(u4() != 0xCAFEBABE) {
		warn("Cannot load class - Invalid Magic");
		return NULL;
	}
	u2(); // Skip minor version (it's zero these days anyway)
	
	ClassFile* cf = malloc(sizeof(ClassFile));
	cf->GetMethod = GetMethod;
	cf->major = u2();
	info("Class file version %d", cf->major);

	cf->cp = malloc(sizeof(ConstantPool));
	cf->cp->size = u2();
	cf->cp->Find = Find;
	cf->cp->GetString = GetString;
	cf->cp->IsOfType = IsOfType;
	info("Constant pool size = %d", cf->cp->size);
	cf->cp->elements = malloc(sizeof(struct cpelem) * cf->cp->size);
	for (int i = 0; i < cf->cp->size - 1; i++) {
		cp().tag = u1();
		// info("Found tag = %d %d", cp()->tag, i);
		switch (cp().tag) {
			case JVM_CONSTANT_Utf8: 
				cp().len = u2();
				cp().bytes = malloc(cp().len + 1);
				for (int j = 0; j < cp().len; j++) {
					cp().bytes[j] = u1(); 
				}
				cp().bytes[cp().len] = '\0';
				break;
			case JVM_CONSTANT_Class:
			case JVM_CONSTANT_String:
			case JVM_CONSTANT_MethodType:
				cp().index = u2(); 
				break;
			case JVM_CONSTANT_Fieldref:
			case JVM_CONSTANT_Methodref:
			case JVM_CONSTANT_NameAndType:
			case JVM_CONSTANT_InterfaceMethodref:
			case JVM_CONSTANT_InvokeDynamic:
				cp().cl_index = u2();
				cp().nt_index = u2();
				break;
			case JVM_CONSTANT_Integer:
				cp().int32 = u4();
				break;
			case JVM_CONSTANT_Long:
				uint32_t high = u4();
				uint32_t low = u4();
				cp().int64 = ((long) high << 32) | low;
				break;
			case JVM_CONSTANT_MethodHandle:
				cp().ref_kind = u1();
				cp().ref = u1();
				break;
			case JVM_CONSTANT_Float:
			case JVM_CONSTANT_Double:
				warn("Float and double is not implemented yet");
				return NULL;
			default:
				warn("Unrecognised constant pool tag %d", cp().tag);
				return NULL;
		}
	}
	
	cf->flags = u2();
	info("Classfile flags = %d", cf->flags);

	cf->this_class = u2();
	info("This class = %d", cf->this_class);

	cf->super_class = u2();
	info("Super class = %d", cf->super_class);

	cf->interfaces_count = u2();
	info("Implemented interfaces = %d", cf->interfaces_count);
	cf->interfaces = malloc(cf->interfaces_count * sizeof(uint16_t));
	for (int i = 0; i < cf->interfaces_count; i++) {
		cf->interfaces[i] = u2();
	}

	cf->fields_count = u2();
	info("Number of fields = %d", cf->fields_count);
	cf->field = malloc(sizeof(FM));
	FM* field = cf->field;
	uint64_t off = parse_methods_or_fields(cf, &field, cf->fields_count, 0, offset, buf);
	if (off == offset && cf->fields_count != 0) {
		warn("Failed to parse fields of class file");
		return NULL;
	}
	offset = off;

	cf->method_count = u2();
	info("Number of methods = %d", cf->method_count);
	cf->methods = malloc(sizeof(FM));
	FM* method = cf->methods;
	off = parse_methods_or_fields(cf, &method, cf->method_count, 1, offset, buf);
	if (off == offset && cf->method_count != 0) {
		warn("Failed to parse methods of class file");
		return NULL;
	}
	offset = off;

	return cf;
}

