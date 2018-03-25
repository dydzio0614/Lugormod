

typedef struct parserField_s parserField_t;

typedef enum parserType_e {
	PARSE_FIELD,
	PARSE_FUNCTION,
}parserType_t;

struct parserField_s {
	char *key;

	parserType_t type;
	union {
		BG_field_t
	}handler;

	struct {
		parserField_t *fields;
		unsigned int count;
	}group;
};