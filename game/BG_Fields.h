// RoboPhred

qboolean BG_ParseType(fieldtype_t type, const char *value, void *target) ;
qboolean BG_ParseField( BG_field_t *l_fields, const char *key, const char *value, void *target );

qboolean BG_GetField(BG_field_t *l_field, char *value, unsigned int sze, byte *ent);

qboolean BG_CompareFields(BG_field_t *f, byte *v1, byte *v2);
void BG_CopyField(BG_field_t *f, byte *dst, byte *src);

#ifdef QAGAME
void BG_FreeField(fieldtype_t type, void *target);
void BG_FreeFields(BG_field_t *fields, byte* structure);
#endif