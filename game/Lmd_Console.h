

#define CT_N "^7"

#ifdef LMD_DYNAMICTHEME
//base
#define CT_B "^\x001"

//arg required
#define CT_AR "^\x002"
//arg optional
#define CT_AO "^\x003"

//value
#define CT_V "^\x004"

//prof bot
#define CT_PB "^\x1B"
//prof forceuser
#define CT_PF "^\x1C"
//prof merc
#define CT_PM "^\x1D"
//prof god
#define CT_PG "^\x1E"

//error
#define CT_E "^\x1F"

#else

//base, notify
#define CT_B "^3"

// base value
#define CT_B_V "^2"

//command
#define CT_C "^5"

//arg required
#define CT_AR "^5"
//arg optional
#define CT_AO "^5"

//success
#define CT_S "^2"
//success value
#define CT_SV "^3"

//value
#define CT_V "^2"
//primary value (for lists)
#define CT_VP "^3"

//list base
#define CT_LB "^5"
//list highlight
#define CT_LH "^3"
//list value
#define CT_LV "^2"

//prof bot
#define CT_PB "^5"
//prof forceuser
#define CT_PF "^4"
//prof merc
#define CT_PM "^1"
//prof god
#define CT_PG "^6"

//unable to perform, not applicable
#define CT_NA "^6"

//na value
#define CT_NAV "^3"

//error
#define CT_E "^1"

#endif


#define CONSOLE_LIST_MAXDISP 25
#define CONSOLE_LIST_ARGHELP CT_AO"<offset>"

#define CONSOLE_LIST_START_OFS(list_offset, list_max) \
	int list_count = -list_offset; \
	unsigned int list_total = 0; \
	unsigned int list_index; \
	for(list_index = list_offset; list_index < list_max; list_index++) {

#define CONSOLE_LIST_RUN_OFS(ent, str) \
		list_count++; \
		if(list_count >= 0 && list_count < CONSOLE_LIST_MAXDISP) \
			Disp(ent, str); \
		list_total++;

#define CONSOLE_LIST_END_OFS(ent) \
	} \
	Disp(ent, va(CT_B"Showing "CT_V"%i"CT_B" out of "CT_V"%u"CT_B".", list_count, list_total));

//TODO: CONSOLE_LIST_SEARCH() series.  Euka-style partial string search with highlighting.

